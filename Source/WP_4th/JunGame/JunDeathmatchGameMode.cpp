#include "JunGame/JunDeathmatchGameMode.h"

#include "Character/Components/HPComp/HealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "JunGame/JunDeathmatchGameState.h"
#include "JunGame/JunDeathmatchPlayerState.h"
#include "JunGame/JunPawnDeathListener.h"
#include "JunGame/JunRingActor.h"
#include "JunGame/JunRingComponent.h"
#include "TimerManager.h"
#include "EngineUtils.h"

AJunDeathmatchGameMode::AJunDeathmatchGameMode()
{
	TargetKillCount = 10;
	MatchStartDelay = 3.f;
	PostMatchDelay = 5.f;
	RespawnDelay = 3.f;
	bAllowRespawn = true;
	DeathmatchSettingsDataTable = nullptr;
	DeathmatchSettingsRowName = TEXT("Default");
	RingActorClass = AJunRingActor::StaticClass();
	bStartRingOnBeginPlay = true;
	RingActorInstance = nullptr;
	GameStateClass = AJunDeathmatchGameState::StaticClass();
	PlayerStateClass = AJunDeathmatchPlayerState::StaticClass();
}

void AJunDeathmatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	LoadDeathmatchSettings();
	SyncGameStateFromConfig();
	SpawnRingActor();

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			RegisterObservedPawn(*It);
		}
	}

	ActorSpawnedHandle = GetWorld()->AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &AJunDeathmatchGameMode::HandleSpawnedActor));

	GetWorldTimerManager().SetTimer(
		StartMatchTimerHandle,
		this,
		&AJunDeathmatchGameMode::StartDeathmatch,
		FMath::Max(0.f, MatchStartDelay),
		false);
}

void AJunDeathmatchGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AJunDeathmatchGameMode::RegisterKill(AController* KillerController, AController* VictimController)
{
	(void)VictimController;

	if (!HasAuthority() || !IsValid(KillerController) || !IsMatchInProgress())
	{
		return;
	}

	int32& KillCount = KillCounts.FindOrAdd(KillerController);
	++KillCount;

	if (AJunDeathmatchPlayerState* KillerPlayerState = KillerController->GetPlayerState<AJunDeathmatchPlayerState>())
	{
		KillerPlayerState->RegisterElimination();
	}

	RefreshLeaderState();

	if (KillCount >= TargetKillCount)
	{
		FinishMatch(KillerController);
	}
}

void AJunDeathmatchGameMode::HandleObservedPawnDeath(APawn* EliminatedPawn)
{
	if (!HasAuthority() || !IsValid(EliminatedPawn))
	{
		return;
	}

	AController* EliminatedController = EliminatedPawn->GetController();
	if (!IsValid(EliminatedController))
	{
		EliminatedController = PawnControllerCache.FindRef(EliminatedPawn);
	}

	if (IsValid(EliminatedController))
	{
		if (AJunDeathmatchPlayerState* VictimPlayerState = EliminatedController->GetPlayerState<AJunDeathmatchPlayerState>())
		{
			VictimPlayerState->RegisterDeath();
		}

		if (IsMatchInProgress() && bAllowRespawn)
		{
			RequestRespawn(EliminatedController);
		}
	}
}

void AJunDeathmatchGameMode::LoadDeathmatchSettings()
{
	if (!DeathmatchSettingsDataTable)
	{
		return;
	}

	const FJunDeathmatchSettingsRow* SettingsRow =
		DeathmatchSettingsDataTable->FindRow<FJunDeathmatchSettingsRow>(DeathmatchSettingsRowName, TEXT("JunDeathmatchSettingsLoad"));

	if (!SettingsRow)
	{
		return;
	}

	TargetKillCount = SettingsRow->TargetKillCount;
	MatchStartDelay = SettingsRow->MatchStartDelay;
	PostMatchDelay = SettingsRow->PostMatchDelay;
	RespawnDelay = SettingsRow->RespawnDelay;
	bStartRingOnBeginPlay = SettingsRow->bStartRingOnBeginPlay;
	bAllowRespawn = SettingsRow->bAllowRespawn;
}

void AJunDeathmatchGameMode::RequestRespawn(AController* EliminatedController)
{
	if (!HasAuthority() || !IsValid(EliminatedController) || !bAllowRespawn)
	{
		return;
	}

	FTimerDelegate RespawnDelegate;
	RespawnDelegate.BindUObject(this, &AJunDeathmatchGameMode::RespawnController, EliminatedController);

	FTimerHandle RespawnTimerHandle;
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, RespawnDelegate, RespawnDelay, false);
}

void AJunDeathmatchGameMode::SpawnRingActor()
{
	if (!HasAuthority() || !RingActorClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	RingActorInstance = GetWorld()->SpawnActor<AJunRingActor>(
		RingActorClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams);

	if (IsValid(RingActorInstance))
	{
		RingActorInstance->OnRingDamageApplied.AddUObject(this, &AJunDeathmatchGameMode::HandleRingDamageApplied);
		GetWorldTimerManager().SetTimer(
			RingStateSyncTimerHandle,
			this,
			&AJunDeathmatchGameMode::SyncRingStateToGameState,
			0.2f,
			true);
	}
}

void AJunDeathmatchGameMode::FinishMatch(AController* WinningController)
{
	if (!HasAuthority())
	{
		return;
	}

	bAllowRespawn = false;

	if (AJunDeathmatchGameState* JunGameState = GetGameState<AJunDeathmatchGameState>())
	{
		JunGameState->SetWinningPlayerState(WinningController ? WinningController->PlayerState : nullptr);
	}

	EndMatch();
}

void AJunDeathmatchGameMode::RespawnController(AController* EliminatedController)
{
	if (!HasAuthority() || !IsValid(EliminatedController))
	{
		return;
	}

	RestartPlayer(EliminatedController);

	if (AJunDeathmatchPlayerState* RespawningPlayerState = EliminatedController->GetPlayerState<AJunDeathmatchPlayerState>())
	{
		RespawningPlayerState->RegisterRespawn();
	}

	if (APawn* RespawnedPawn = EliminatedController->GetPawn())
	{
		PawnControllerCache.Add(RespawnedPawn, EliminatedController);
		RegisterObservedPawn(RespawnedPawn);
	}
}

void AJunDeathmatchGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	if (AJunDeathmatchGameState* JunGameState = GetGameState<AJunDeathmatchGameState>())
	{
		JunGameState->SetMatchPhase(EJunMatchPhase::InProgress);
	}
}

void AJunDeathmatchGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	if (AJunDeathmatchGameState* JunGameState = GetGameState<AJunDeathmatchGameState>())
	{
		JunGameState->SetMatchPhase(EJunMatchPhase::Completed);
	}

	GetWorldTimerManager().SetTimer(
		PostMatchCleanupTimerHandle,
		this,
		&AJunDeathmatchGameMode::HandlePostMatchCleanup,
		FMath::Max(0.f, PostMatchDelay),
		false);
}

void AJunDeathmatchGameMode::StartDeathmatch()
{
	if (!HasAuthority() || IsMatchInProgress())
	{
		return;
	}

	StartMatch();

	if (bStartRingOnBeginPlay && IsValid(RingActorInstance))
	{
		RingActorInstance->StartRing();
	}
}

void AJunDeathmatchGameMode::HandleSpawnedActor(AActor* SpawnedActor)
{
	if (APawn* SpawnedPawn = Cast<APawn>(SpawnedActor))
	{
		RegisterObservedPawn(SpawnedPawn);
	}
}

void AJunDeathmatchGameMode::RegisterObservedPawn(APawn* Pawn)
{
	if (!HasAuthority() || !IsValid(Pawn) || RegisteredPawns.Contains(Pawn))
	{
		return;
	}

	UHealthComponent* HealthComponent = Pawn->FindComponentByClass<UHealthComponent>();
	if (!IsValid(HealthComponent))
	{
		return;
	}

	RegisteredPawns.Add(Pawn);
	PawnControllerCache.Add(Pawn, Pawn->GetController());

	UJunPawnDeathListener* Listener = NewObject<UJunPawnDeathListener>(this);
	Listener->Initialize(this, Pawn);
	HealthComponent->OnDeath.AddDynamic(Listener, &UJunPawnDeathListener::HandleObservedDeath);
	DeathListeners.Add(Listener);
}

void AJunDeathmatchGameMode::RefreshLeaderState()
{
	AJunDeathmatchPlayerState* BestPlayerState = nullptr;
	int32 BestScore = INDEX_NONE;

	for (const TPair<TObjectPtr<AController>, int32>& Pair : KillCounts)
	{
		AController* Controller = Pair.Key.Get();
		if (!IsValid(Controller))
		{
			continue;
		}

		if (Pair.Value > BestScore)
		{
			BestScore = Pair.Value;
			BestPlayerState = Controller->GetPlayerState<AJunDeathmatchPlayerState>();
		}
	}

	if (AJunDeathmatchGameState* JunGameState = GetGameState<AJunDeathmatchGameState>())
	{
		JunGameState->SetCurrentLeader(BestPlayerState);
	}
}

void AJunDeathmatchGameMode::SyncGameStateFromConfig()
{
	if (AJunDeathmatchGameState* JunGameState = GetGameState<AJunDeathmatchGameState>())
	{
		JunGameState->SetTargetKillCount(TargetKillCount);
		JunGameState->SetMatchPhase(EJunMatchPhase::Warmup);
	}
}

void AJunDeathmatchGameMode::SyncRingStateToGameState()
{
	if (!HasAuthority() || !IsValid(RingActorInstance))
	{
		return;
	}

	if (AJunDeathmatchGameState* JunGameState = GetGameState<AJunDeathmatchGameState>())
	{
		JunGameState->SetRingState(
			RingActorInstance->GetCurrentPhaseIndex(),
			RingActorInstance->GetCurrentRadius(),
			RingActorInstance->IsRingShrinking(),
			RingActorInstance->GetRingPhaseState(),
			RingActorInstance->GetPhaseTimeRemaining(),
			RingActorInstance->GetRingCenter());
	}
}

void AJunDeathmatchGameMode::HandleRingDamageApplied(APawn* DamagedPawn)
{
	if (!HasAuthority() || !IsValid(DamagedPawn) || !GetWorld())
	{
		return;
	}

	LastRingDamageTimestamps.Add(DamagedPawn, GetWorld()->GetTimeSeconds());
}

void AJunDeathmatchGameMode::HandlePostMatchCleanup()
{
	GetWorldTimerManager().ClearTimer(RingStateSyncTimerHandle);
}
