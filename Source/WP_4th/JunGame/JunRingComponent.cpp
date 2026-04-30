#include "JunGame/JunRingComponent.h"

#include "Algo/Sort.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "JunGame/JunRingDamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UJunRingComponent::UJunRingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	bStartAutomatically = true;
	bUseOwnerLocationAsCenter = true;
	RingCenter = FVector::ZeroVector;
	InitialRadius = 10000.f;
	RingPhaseDataTable = nullptr;

	FJunRingPhaseRow OpeningPhase;
	OpeningPhase.PhaseIndex = 0;
	OpeningPhase.TargetRadius = 8000.f;
	OpeningPhase.WaitTime = 3.f;
	OpeningPhase.ShrinkTime = 8.f;
	OpeningPhase.DamageInterval = 1.f;
	OpeningPhase.DamagePerTick = 2.f;
	RingPhases.Add(OpeningPhase);

	FJunRingPhaseRow MidPhase;
	MidPhase.PhaseIndex = 1;
	MidPhase.TargetRadius = 4000.f;
	MidPhase.WaitTime = 3.f;
	MidPhase.ShrinkTime = 6.f;
	MidPhase.DamageInterval = 0.75f;
	MidPhase.DamagePerTick = 5.f;
	RingPhases.Add(MidPhase);

	FJunRingPhaseRow FinalPhase;
	FinalPhase.PhaseIndex = 2;
	FinalPhase.TargetRadius = 1500.f;
	FinalPhase.WaitTime = 2.f;
	FinalPhase.ShrinkTime = 5.f;
	FinalPhase.DamageInterval = 0.5f;
	FinalPhase.DamagePerTick = 8.f;
	RingPhases.Add(FinalPhase);

	CurrentRadius = InitialRadius;
	CurrentPhaseIndex = INDEX_NONE;
	bRingStarted = false;
	bIsShrinking = false;
	bIsPaused = false;
	PhaseState = EJunRingPhaseState::Inactive;
	bEnableDebugDraw = true;
	DebugDrawDuration = 0.f;
	bLogValidationDetails = true;
	RingDamageType = UJunRingDamageType::StaticClass();
	PhaseStartRadius = InitialRadius;
	PhaseTargetRadius = InitialRadius;
	TargetRingCenter = RingCenter;
	ShrinkStartTime = 0.f;
	ShrinkEndTime = 0.f;
	PhaseStateEndTime = 0.f;
	PausedPhaseTimeRemaining = 0.f;
	PausedShrinkTimeRemaining = 0.f;
}

void UJunRingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority() && !Owner->GetIsReplicated())
		{
			Owner->SetReplicates(true);
			Owner->SetNetUpdateFrequency(FMath::Max(Owner->GetNetUpdateFrequency(), 15.f));
			UE_LOG(LogTemp, Log, TEXT("JunRingComponent: enabled replication on owner %s for client ring sync."), *GetNameSafe(Owner));
		}
	}

	CurrentRadius = InitialRadius;
	PhaseStartRadius = InitialRadius;
	PhaseTargetRadius = InitialRadius;
	TargetRingCenter = RingCenter;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (bUseOwnerLocationAsCenter)
		{
			RingCenter = GetOwner()->GetActorLocation();
		}

		LoadRingPhasesFromDataTable();

		if (bStartAutomatically)
		{
			StartRing();
		}
	}
}

void UJunRingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !bRingStarted || !bIsShrinking)
	{
		if (bEnableDebugDraw && GetWorld())
		{
			DrawDebugSphere(GetWorld(), RingCenter, CurrentRadius, 64, FColor::Cyan, false, DebugDrawDuration, 0, 4.f);
		}
		return;
	}

	UpdateCurrentRadiusFromShrinkTime();

	if (bEnableDebugDraw)
	{
		DrawDebugSphere(GetWorld(), RingCenter, CurrentRadius, 64, FColor::Cyan, false, DebugDrawDuration, 0, 4.f);
	}
}

void UJunRingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UJunRingComponent, CurrentRadius);
	DOREPLIFETIME(UJunRingComponent, CurrentPhaseIndex);
	DOREPLIFETIME(UJunRingComponent, bRingStarted);
	DOREPLIFETIME(UJunRingComponent, bIsShrinking);
	DOREPLIFETIME(UJunRingComponent, bIsPaused);
	DOREPLIFETIME(UJunRingComponent, PhaseState);
	DOREPLIFETIME(UJunRingComponent, RingCenter);
	DOREPLIFETIME(UJunRingComponent, TargetRingCenter);
	DOREPLIFETIME(UJunRingComponent, PhaseStartRadius);
	DOREPLIFETIME(UJunRingComponent, PhaseTargetRadius);
	DOREPLIFETIME(UJunRingComponent, ShrinkStartTime);
	DOREPLIFETIME(UJunRingComponent, ShrinkEndTime);
	DOREPLIFETIME(UJunRingComponent, PhaseStateEndTime);
}

void UJunRingComponent::StartRing()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bRingStarted)
	{
		return;
	}

	if (!ReloadRingData())
	{
		UE_LOG(LogTemp, Error, TEXT("JunRingComponent: StartRing failed because ring phase data is invalid. Owner=%s"), *GetNameSafe(GetOwner()));
		return;
	}

	bRingStarted = true;
	bIsPaused = false;
	UE_LOG(LogTemp, Log, TEXT("JunRingComponent: StartRing. Owner=%s InitialRadius=%.2f PhaseCount=%d"),
		*GetNameSafe(GetOwner()),
		InitialRadius,
		RingPhases.Num());
	BeginPhase(0);
}

bool UJunRingComponent::InitFromDataTable(UDataTable* InTable)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	RingPhaseDataTable = InTable;
	return ReloadRingData();
}

bool UJunRingComponent::InitFromPhaseRows(const TArray<FJunRingPhaseRow>& InRows)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	TArray<FJunRingPhaseRow> CandidateRows = InRows;
	Algo::Sort(CandidateRows, [](const FJunRingPhaseRow& Left, const FJunRingPhaseRow& Right)
	{
		return Left.PhaseIndex < Right.PhaseIndex;
	});

	FString ValidationError;
	if (!ValidateRingPhases(CandidateRows, ValidationError))
	{
		UE_LOG(LogTemp, Error, TEXT("JunRingComponent: InitFromPhaseRows failed. %s"), *ValidationError);
		return false;
	}

	RingPhases = CandidateRows;
	return true;
}

void UJunRingComponent::StopRing()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ClearRingTimers();
	bRingStarted = false;
	bIsShrinking = false;
	bIsPaused = false;
	CurrentPhaseIndex = INDEX_NONE;
	PhaseState = EJunRingPhaseState::Inactive;
	PhaseStateEndTime = 0.f;
	PausedPhaseTimeRemaining = 0.f;
	PausedShrinkTimeRemaining = 0.f;
}

void UJunRingComponent::PauseRing()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bRingStarted || bIsPaused)
	{
		return;
	}

	UpdateCurrentRadiusFromShrinkTime();
	const float CurrentTime = GetRingWorldTime();
	PausedPhaseTimeRemaining = FMath::Max(0.f, PhaseStateEndTime - CurrentTime);
	PausedShrinkTimeRemaining = FMath::Max(0.f, ShrinkEndTime - CurrentTime);
	ClearRingTimers();
	bIsPaused = true;
	bIsShrinking = false;
	PhaseState = EJunRingPhaseState::Paused;
}

void UJunRingComponent::ResumeRing()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bRingStarted || !bIsPaused || !RingPhases.IsValidIndex(CurrentPhaseIndex))
	{
		return;
	}

	bIsPaused = false;

	if (PausedShrinkTimeRemaining > 0.f)
	{
		PhaseState = EJunRingPhaseState::Shrinking;
		bIsShrinking = true;
		PhaseStartRadius = CurrentRadius;
		ShrinkStartTime = GetRingWorldTime();
		ShrinkEndTime = ShrinkStartTime + PausedShrinkTimeRemaining;
		PhaseStateEndTime = ShrinkEndTime;
		GetWorld()->GetTimerManager().SetTimer(PhaseEndTimerHandle, this, &UJunRingComponent::CompletePhase, PausedShrinkTimeRemaining, false);
	}
	else
	{
		PhaseState = EJunRingPhaseState::Waiting;
		bIsShrinking = false;
		PhaseStateEndTime = GetRingWorldTime() + PausedPhaseTimeRemaining;
		GetWorld()->GetTimerManager().SetTimer(PhaseStartTimerHandle, this, &UJunRingComponent::StartShrinkForCurrentPhase, PausedPhaseTimeRemaining, false);
	}

	const FJunRingPhaseRow& Phase = RingPhases[CurrentPhaseIndex];
	GetWorld()->GetTimerManager().SetTimer(DamageTickTimerHandle, this, &UJunRingComponent::ApplyRingDamage, GetPhaseDamageInterval(Phase), true);
	PausedPhaseTimeRemaining = 0.f;
	PausedShrinkTimeRemaining = 0.f;
}

void UJunRingComponent::ResetRing()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	StopRing();
	CurrentRadius = InitialRadius;
	PhaseStartRadius = InitialRadius;
	PhaseTargetRadius = InitialRadius;
	TargetRingCenter = RingCenter;
}

void UJunRingComponent::ResetForRound()
{
	ResetRing();
}

bool UJunRingComponent::AdvanceToPhase(int32 PhaseIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !RingPhases.IsValidIndex(PhaseIndex))
	{
		return false;
	}

	bRingStarted = true;
	bIsPaused = false;
	BeginPhase(PhaseIndex);
	return true;
}

bool UJunRingComponent::ReloadRingData()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	LoadRingPhasesFromDataTable();
	if (!RingPhaseDataTable)
	{
		NormalizeDefaultPhasesForInitialRadius();
	}

	FString ValidationError;
	if (!ValidateRingPhases(RingPhases, ValidationError))
	{
		UE_LOG(LogTemp, Error, TEXT("JunRingComponent: invalid ring phase data. %s"), *ValidationError);
		return false;
	}

	return true;
}

void UJunRingComponent::SetRingCenter(const FVector& NewRingCenter)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	RingCenter = NewRingCenter;
	TargetRingCenter = NewRingCenter;
}

float UJunRingComponent::GetPhaseTimeRemaining() const
{
	if (!bRingStarted || PhaseState == EJunRingPhaseState::Inactive || PhaseState == EJunRingPhaseState::Completed)
	{
		return 0.f;
	}

	if (const UWorld* World = GetWorld())
	{
		return FMath::Max(0.f, PhaseStateEndTime - GetRingWorldTime());
	}

	return 0.f;
}

void UJunRingComponent::LoadRingPhasesFromDataTable()
{
	if (!RingPhaseDataTable)
	{
		return;
	}

	TArray<FJunRingPhaseRow*> Rows;
	RingPhaseDataTable->GetAllRows(TEXT("JunRingPhaseDataLoad"), Rows);
	if (Rows.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("JunRingComponent: RingPhaseDataTable has no rows. Table=%s"), *GetNameSafe(RingPhaseDataTable));
		return;
	}

	RingPhases.Reset();
	for (const FJunRingPhaseRow* Row : Rows)
	{
		if (Row)
		{
			RingPhases.Add(*Row);
		}
	}

	Algo::Sort(RingPhases, [](const FJunRingPhaseRow& Left, const FJunRingPhaseRow& Right)
	{
		return Left.PhaseIndex < Right.PhaseIndex;
	});
}

void UJunRingComponent::NormalizeDefaultPhasesForInitialRadius()
{
	constexpr float AuthoredDefaultInitialRadius = 10000.f;
	if (InitialRadius <= 0.f || RingPhases.IsEmpty())
	{
		return;
	}

	const bool bNeedsRadiusScale = !FMath::IsNearlyEqual(InitialRadius, AuthoredDefaultInitialRadius) && RingPhases[0].TargetRadius > InitialRadius;
	if (!bNeedsRadiusScale)
	{
		for (FJunRingPhaseRow& Phase : RingPhases)
		{
			Phase.WaitTime = FMath::Min(Phase.WaitTime, 3.f);
			Phase.ShrinkTime = FMath::Min(Phase.ShrinkTime, 8.f);
		}
		return;
	}

	const float RadiusScale = InitialRadius / AuthoredDefaultInitialRadius;
	for (FJunRingPhaseRow& Phase : RingPhases)
	{
		if (Phase.Radius > 0.f)
		{
			Phase.Radius *= RadiusScale;
		}

		Phase.TargetRadius *= RadiusScale;
		Phase.WaitTime = FMath::Min(Phase.WaitTime, 3.f);
		Phase.ShrinkTime = FMath::Min(Phase.ShrinkTime, 8.f);
	}

	if (bLogValidationDetails)
	{
		UE_LOG(LogTemp, Log, TEXT("JunRingComponent: scaled built-in ring phases for InitialRadius %.2f with scale %.3f. Owner=%s"),
			InitialRadius,
			RadiusScale,
			*GetNameSafe(GetOwner()));
	}
}

bool UJunRingComponent::ValidateRingPhases(const TArray<FJunRingPhaseRow>& CandidatePhases, FString& OutReason) const
{
	if (CandidatePhases.IsEmpty())
	{
		OutReason = TEXT("RingPhases is empty.");
		return false;
	}

	int32 ExpectedPhaseIndex = 0;
	float PreviousTargetRadius = InitialRadius;
	for (const FJunRingPhaseRow& Phase : CandidatePhases)
	{
		if (Phase.PhaseIndex != ExpectedPhaseIndex)
		{
			OutReason = FString::Printf(TEXT("PhaseIndex must be sequential. Expected=%d Actual=%d"), ExpectedPhaseIndex, Phase.PhaseIndex);
			return false;
		}

		if (Phase.TargetRadius < 0.f)
		{
			OutReason = FString::Printf(TEXT("Phase %d TargetRadius must be >= 0."), Phase.PhaseIndex);
			return false;
		}

		if (Phase.TargetRadius > PreviousTargetRadius && Phase.Radius <= 0.f)
		{
			OutReason = FString::Printf(TEXT("Phase %d TargetRadius grows from previous radius without explicit Radius."), Phase.PhaseIndex);
			return false;
		}

		if (GetPhaseWaitTime(Phase) < 0.f || GetPhaseShrinkTime(Phase) < 0.f)
		{
			OutReason = FString::Printf(TEXT("Phase %d timing values must be >= 0."), Phase.PhaseIndex);
			return false;
		}

		if (GetPhaseDamageInterval(Phase) <= 0.f)
		{
			OutReason = FString::Printf(TEXT("Phase %d DamageTickInterval/DamageInterval must be > 0."), Phase.PhaseIndex);
			return false;
		}

		PreviousTargetRadius = Phase.TargetRadius;
		++ExpectedPhaseIndex;
	}

	if (bLogValidationDetails)
	{
		UE_LOG(LogTemp, Log, TEXT("JunRingComponent: validated %d ring phases for %s."), CandidatePhases.Num(), *GetNameSafe(GetOwner()));
	}

	return true;
}

void UJunRingComponent::BeginPhase(int32 PhaseIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !RingPhases.IsValidIndex(PhaseIndex))
	{
		return;
	}

	ClearRingTimers();
	CurrentPhaseIndex = PhaseIndex;
	bIsShrinking = false;
	bIsPaused = false;
	PhaseState = EJunRingPhaseState::Waiting;

	const FJunRingPhaseRow& Phase = RingPhases[PhaseIndex];
	ApplyPhaseCenterPolicy(Phase);

	if (Phase.Radius > 0.f)
	{
		CurrentRadius = Phase.Radius;
	}

	PhaseStartRadius = CurrentRadius;
	PhaseTargetRadius = Phase.TargetRadius;
	const float WaitTime = GetPhaseWaitTime(Phase);
	PhaseStateEndTime = GetRingWorldTime() + WaitTime;
	UE_LOG(LogTemp, Log, TEXT("JunRingComponent: BeginPhase %d. CurrentRadius=%.2f TargetRadius=%.2f Wait=%.2f Shrink=%.2f Owner=%s"),
		CurrentPhaseIndex,
		CurrentRadius,
		PhaseTargetRadius,
		WaitTime,
		GetPhaseShrinkTime(Phase),
		*GetNameSafe(GetOwner()));
	GetOwner()->ForceNetUpdate();

	GetWorld()->GetTimerManager().SetTimer(
		DamageTickTimerHandle,
		this,
		&UJunRingComponent::ApplyRingDamage,
		GetPhaseDamageInterval(Phase),
		true);

	if (WaitTime <= 0.f)
	{
		StartShrinkForCurrentPhase();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			PhaseStartTimerHandle,
			this,
			&UJunRingComponent::StartShrinkForCurrentPhase,
			WaitTime,
			false);
	}
}

void UJunRingComponent::StartShrinkForCurrentPhase()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !RingPhases.IsValidIndex(CurrentPhaseIndex))
	{
		return;
	}

	const FJunRingPhaseRow& Phase = RingPhases[CurrentPhaseIndex];

	PhaseStartRadius = CurrentRadius;
	PhaseTargetRadius = Phase.TargetRadius;
	ShrinkStartTime = GetRingWorldTime();
	const float ShrinkTime = GetPhaseShrinkTime(Phase);
	ShrinkEndTime = ShrinkStartTime + ShrinkTime;
	PhaseStateEndTime = ShrinkEndTime;
	bIsShrinking = true;
	PhaseState = EJunRingPhaseState::Shrinking;
	UE_LOG(LogTemp, Log, TEXT("JunRingComponent: StartShrink phase %d from %.2f to %.2f over %.2f seconds. Owner=%s"),
		CurrentPhaseIndex,
		PhaseStartRadius,
		PhaseTargetRadius,
		ShrinkTime,
		*GetNameSafe(GetOwner()));
	GetOwner()->ForceNetUpdate();

	if (ShrinkTime <= 0.f)
	{
		CompletePhase();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			PhaseEndTimerHandle,
			this,
			&UJunRingComponent::CompletePhase,
			ShrinkTime,
			false);
	}
}

void UJunRingComponent::CompletePhase()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !RingPhases.IsValidIndex(CurrentPhaseIndex))
	{
		return;
	}

	CurrentRadius = RingPhases[CurrentPhaseIndex].TargetRadius;
	bIsShrinking = false;
	PhaseState = EJunRingPhaseState::Completed;
	UE_LOG(LogTemp, Log, TEXT("JunRingComponent: CompletePhase %d. Radius=%.2f Owner=%s"),
		CurrentPhaseIndex,
		CurrentRadius,
		*GetNameSafe(GetOwner()));
	GetOwner()->ForceNetUpdate();

	const int32 NextPhaseIndex = CurrentPhaseIndex + 1;
	if (RingPhases.IsValidIndex(NextPhaseIndex))
	{
		BeginPhase(NextPhaseIndex);
	}
	else
	{
		PhaseStateEndTime = 0.f;
	}
}

void UJunRingComponent::ApplyRingDamage()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !RingPhases.IsValidIndex(CurrentPhaseIndex))
	{
		return;
	}

	const float DamageAmount = GetPhaseDamageAmount(RingPhases[CurrentPhaseIndex]);

	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		APawn* Pawn = *It;
		if (!IsValid(Pawn) || Pawn->IsActorBeingDestroyed())
		{
			continue;
		}

		if (IsOutsideRing(Pawn->GetActorLocation()))
		{
			UGameplayStatics::ApplyDamage(Pawn, DamageAmount, nullptr, GetOwner(), RingDamageType);
			OnRingDamageApplied.Broadcast(Pawn);
		}
	}
}

void UJunRingComponent::ClearRingTimers()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(PhaseStartTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(PhaseEndTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DamageTickTimerHandle);
}

void UJunRingComponent::ApplyPhaseCenterPolicy(const FJunRingPhaseRow& Phase)
{
	switch (Phase.CenterMode)
	{
	case EJunRingCenterMode::OwnerLocation:
		if (GetOwner())
		{
			RingCenter = GetOwner()->GetActorLocation();
		}
		break;
	case EJunRingCenterMode::FixedLocation:
		RingCenter = Phase.FixedCenter;
		break;
	case EJunRingCenterMode::KeepCurrent:
	default:
		break;
	}

	TargetRingCenter = RingCenter;
}

void UJunRingComponent::UpdateCurrentRadiusFromShrinkTime()
{
	if (!GetWorld() || !bIsShrinking)
	{
		return;
	}

	if (ShrinkEndTime <= ShrinkStartTime)
	{
		CurrentRadius = PhaseTargetRadius;
		return;
	}

	const float CurrentTime = GetRingWorldTime();
	const float Alpha = FMath::Clamp((CurrentTime - ShrinkStartTime) / (ShrinkEndTime - ShrinkStartTime), 0.f, 1.f);
	CurrentRadius = FMath::Lerp(PhaseStartRadius, PhaseTargetRadius, Alpha);
}

float UJunRingComponent::GetPhaseWaitTime(const FJunRingPhaseRow& Phase) const
{
	return Phase.DelayBeforeShrink >= 0.f ? Phase.DelayBeforeShrink : Phase.WaitTime;
}

float UJunRingComponent::GetPhaseShrinkTime(const FJunRingPhaseRow& Phase) const
{
	return Phase.ShrinkDuration >= 0.f ? Phase.ShrinkDuration : Phase.ShrinkTime;
}

float UJunRingComponent::GetPhaseDamageInterval(const FJunRingPhaseRow& Phase) const
{
	return FMath::Max(KINDA_SMALL_NUMBER, Phase.DamageTickInterval > 0.f ? Phase.DamageTickInterval : Phase.DamageInterval);
}

float UJunRingComponent::GetPhaseDamageAmount(const FJunRingPhaseRow& Phase) const
{
	if (Phase.DamagePerSecond >= 0.f)
	{
		return Phase.DamagePerSecond * GetPhaseDamageInterval(Phase);
	}

	return Phase.DamagePerTick;
}

float UJunRingComponent::GetRingWorldTime() const
{
	if (!GetWorld())
	{
		return 0.f;
	}

	if (const AGameStateBase* GameState = GetWorld()->GetGameState())
	{
		return GameState->GetServerWorldTimeSeconds();
	}

	return GetWorld()->GetTimeSeconds();
}

bool UJunRingComponent::IsOutsideRing(const FVector& TargetLocation) const
{
	const FVector Target2D(TargetLocation.X, TargetLocation.Y, 0.f);
	const FVector Center2D(RingCenter.X, RingCenter.Y, 0.f);
	return FVector::DistSquared(Target2D, Center2D) > FMath::Square(CurrentRadius);
}

void UJunRingComponent::OnRep_CurrentRadius()
{
}
