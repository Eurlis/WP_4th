#include "JunGame/JunRingComponent.h"

#include "Algo/Sort.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "JunGame/JunRingDamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UJunRingComponent::UJunRingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	bStartAutomatically = false;
	bUseOwnerLocationAsCenter = true;
	RingCenter = FVector::ZeroVector;
	InitialRadius = 10000.f;
	RingPhaseDataTable = nullptr;

	FJunRingPhaseRow OpeningPhase;
	OpeningPhase.PhaseIndex = 0;
	OpeningPhase.TargetRadius = 8000.f;
	OpeningPhase.WaitTime = 30.f;
	OpeningPhase.ShrinkTime = 20.f;
	OpeningPhase.DamageInterval = 1.f;
	OpeningPhase.DamagePerTick = 2.f;
	RingPhases.Add(OpeningPhase);

	FJunRingPhaseRow MidPhase;
	MidPhase.PhaseIndex = 1;
	MidPhase.TargetRadius = 4000.f;
	MidPhase.WaitTime = 20.f;
	MidPhase.ShrinkTime = 15.f;
	MidPhase.DamageInterval = 0.75f;
	MidPhase.DamagePerTick = 5.f;
	RingPhases.Add(MidPhase);

	FJunRingPhaseRow FinalPhase;
	FinalPhase.PhaseIndex = 2;
	FinalPhase.TargetRadius = 1500.f;
	FinalPhase.WaitTime = 12.f;
	FinalPhase.ShrinkTime = 10.f;
	FinalPhase.DamageInterval = 0.5f;
	FinalPhase.DamagePerTick = 8.f;
	RingPhases.Add(FinalPhase);

	CurrentRadius = InitialRadius;
	CurrentPhaseIndex = INDEX_NONE;
	bRingStarted = false;
	bIsShrinking = false;
	PhaseState = EJunRingPhaseState::Inactive;
	bEnableDebugDraw = false;
	DebugDrawDuration = 0.f;
	RingDamageType = UJunRingDamageType::StaticClass();
	PhaseStartRadius = InitialRadius;
	PhaseTargetRadius = InitialRadius;
	ShrinkStartTime = 0.f;
	ShrinkEndTime = 0.f;
	PhaseStateEndTime = 0.f;
}

void UJunRingComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentRadius = InitialRadius;
	PhaseStartRadius = InitialRadius;
	PhaseTargetRadius = InitialRadius;

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

	if (!GetOwner() || !GetOwner()->HasAuthority() || !bRingStarted || !bIsShrinking)
	{
		if (bEnableDebugDraw && GetWorld())
		{
			DrawDebugSphere(GetWorld(), RingCenter, CurrentRadius, 64, FColor::Cyan, false, DebugDrawDuration, 0, 4.f);
		}
		return;
	}

	if (ShrinkEndTime <= ShrinkStartTime)
	{
		CurrentRadius = PhaseTargetRadius;
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float Alpha = FMath::Clamp((CurrentTime - ShrinkStartTime) / (ShrinkEndTime - ShrinkStartTime), 0.f, 1.f);
	CurrentRadius = FMath::Lerp(PhaseStartRadius, PhaseTargetRadius, Alpha);

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
	DOREPLIFETIME(UJunRingComponent, PhaseState);
}

void UJunRingComponent::StartRing()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bRingStarted || RingPhases.IsEmpty())
	{
		return;
	}

	bRingStarted = true;
	BeginPhase(0);
}

void UJunRingComponent::SetRingCenter(const FVector& NewRingCenter)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	RingCenter = NewRingCenter;
}

float UJunRingComponent::GetPhaseTimeRemaining() const
{
	if (!bRingStarted || PhaseState == EJunRingPhaseState::Inactive || PhaseState == EJunRingPhaseState::Completed)
	{
		return 0.f;
	}

	if (const UWorld* World = GetWorld())
	{
		return FMath::Max(0.f, PhaseStateEndTime - World->GetTimeSeconds());
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

void UJunRingComponent::BeginPhase(int32 PhaseIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !RingPhases.IsValidIndex(PhaseIndex))
	{
		return;
	}

	CurrentPhaseIndex = PhaseIndex;
	bIsShrinking = false;
	PhaseState = EJunRingPhaseState::Waiting;

	const FJunRingPhaseRow& Phase = RingPhases[PhaseIndex];
	GetWorld()->GetTimerManager().ClearTimer(PhaseStartTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(PhaseEndTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(DamageTickTimerHandle);

	PhaseStateEndTime = GetWorld()->GetTimeSeconds() + Phase.WaitTime;

	GetWorld()->GetTimerManager().SetTimer(
		DamageTickTimerHandle,
		this,
		&UJunRingComponent::ApplyRingDamage,
		FMath::Max(KINDA_SMALL_NUMBER, Phase.DamageInterval),
		true);

	GetWorld()->GetTimerManager().SetTimer(
		PhaseStartTimerHandle,
		this,
		&UJunRingComponent::StartShrinkForCurrentPhase,
		Phase.WaitTime,
		false);
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
	ShrinkStartTime = GetWorld()->GetTimeSeconds();
	ShrinkEndTime = ShrinkStartTime + Phase.ShrinkTime;
	PhaseStateEndTime = ShrinkEndTime;
	bIsShrinking = true;
	PhaseState = EJunRingPhaseState::Shrinking;

	GetWorld()->GetTimerManager().SetTimer(
		PhaseEndTimerHandle,
		this,
		&UJunRingComponent::CompletePhase,
		Phase.ShrinkTime,
		false);
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

	const float DamageAmount = RingPhases[CurrentPhaseIndex].DamagePerTick;

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

bool UJunRingComponent::IsOutsideRing(const FVector& TargetLocation) const
{
	const FVector Target2D(TargetLocation.X, TargetLocation.Y, 0.f);
	const FVector Center2D(RingCenter.X, RingCenter.Y, 0.f);
	return FVector::DistSquared(Target2D, Center2D) > FMath::Square(CurrentRadius);
}

void UJunRingComponent::OnRep_CurrentRadius()
{
}
