#include "ApexDeathmatchGameMode.h"

#include "ApexDeathmatchGameState.h"
#include "ApexDeathmatchPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "JunGame/JunDeathmatchGameState.h"
#include "JunGame/JunDeathmatchPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Weapon/BulletPoolManager.h"

AApexDeathmatchGameMode::AApexDeathmatchGameMode()
{
	GameStateClass = AApexDeathmatchGameState::StaticClass();
	PlayerStateClass = AApexDeathmatchPlayerState::StaticClass();

	BulletPoolManagerClass = nullptr;
	SpawnedBulletPool = nullptr;

	bAllowRespawn = false;          // 부모 시스템 비활성화 (이중 리스폰 차단)
	bApexAllowRespawn = true;       // Apex 자체 리스폰 시스템 활성화
	ApexRespawnDelay = 5.0f;
}

void AApexDeathmatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnBulletPool();
	}
}

void AApexDeathmatchGameMode::HandleApexPawnKilled(AController* Killer, AController* Victim)
{
	if (!HasAuthority())
	{
		return;
	}

	const bool bSuicide = IsValid(Killer) && (Killer == Victim);
	const bool bEnvKill = !IsValid(Killer);

	if (bSuicide || bEnvKill)
	{
		UE_LOG(LogTemp, Log, TEXT("[ApexGM] Kill not credited (Suicide=%d EnvKill=%d) Victim=%s"),
			bSuicide ? 1 : 0,
			bEnvKill ? 1 : 0,
			Victim ? *Victim->GetName() : TEXT("None"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[ApexGM] Kill registered: Killer=%s Victim=%s"),
		*Killer->GetName(),
		*Victim->GetName());

	Super::RegisterKill(Killer, Victim);

	if (IsMatchInProgress() && bApexAllowRespawn && IsValid(Victim))
	{
		RequestRespawn(Victim);
	}
}

void AApexDeathmatchGameMode::RequestRespawn(AController* EliminatedController)
{
	if (!HasAuthority() || !IsValid(EliminatedController) || !bApexAllowRespawn)
	{
		return;
	}
	if (!IsMatchInProgress())
	{
		return;
	}

	TWeakObjectPtr<AController> WeakCtrl(EliminatedController);
	FTimerHandle& Handle = RespawnTimers.FindOrAdd(WeakCtrl);
	GetWorldTimerManager().ClearTimer(Handle); // 중복 방지

	FTimerDelegate Del;
	Del.BindUObject(this, &AApexDeathmatchGameMode::RespawnController, EliminatedController);
	GetWorldTimerManager().SetTimer(Handle, Del, FMath::Max(0.1f, ApexRespawnDelay), false);
}

void AApexDeathmatchGameMode::RespawnController(AController* EliminatedController)
{
	RespawnTimers.Remove(TWeakObjectPtr<AController>(EliminatedController));

	if (!HasAuthority() || !IsValid(EliminatedController))
	{
		return;
	}
	if (!IsMatchInProgress() || !bApexAllowRespawn)
	{
		return;
	}

	RestartPlayer(EliminatedController);
}

void AApexDeathmatchGameMode::ClearAllRespawnTimers()
{
	if (UWorld* W = GetWorld())
	{
		for (auto& Pair : RespawnTimers)
		{
			W->GetTimerManager().ClearTimer(Pair.Value);
		}
	}
	RespawnTimers.Empty();
}

void AApexDeathmatchGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	if (!HasAuthority())
	{
		return;
	}
	bMatchTimeExpired = false;
	GetWorldTimerManager().SetTimer(
		MatchTimerHandle, this,
		&AApexDeathmatchGameMode::OnMatchTimeUp,
		FMath::Max(1.f, MatchDuration), false);
	UE_LOG(LogTemp, Log, TEXT("[ApexGM] Match timer started: %.1fs"), MatchDuration);
}

void AApexDeathmatchGameMode::OnMatchTimeUp()
{
	if (!HasAuthority() || bMatchTimeExpired)
	{
		return;
	}
	bMatchTimeExpired = true;
	bApexAllowRespawn = false;
	ClearAllRespawnTimers();

	AController* Winner = nullptr;
	int32 WinnerKills = 0;

	// GameState/PlayerState 에 GetCurrentLeader 가 없으므로 PlayerArray 직접 순회.
	// 매치당 1회 호출이라 비용 무시 가능.
	if (AGameStateBase* GS = GetGameState<AGameStateBase>())
	{
		AJunDeathmatchPlayerState* TopPS = nullptr;
		for (APlayerState* PS : GS->PlayerArray)
		{
			AJunDeathmatchPlayerState* JunPS = Cast<AJunDeathmatchPlayerState>(PS);
			if (!JunPS)
			{
				continue;
			}
			const int32 Kills = JunPS->GetEliminations();
			if (!TopPS || Kills > WinnerKills)
			{
				TopPS = JunPS;
				WinnerKills = Kills;
			}
		}
		if (TopPS)
		{
			Winner = TopPS->GetOwner<AController>();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[ApexGM] Match time up — Winner=%s Kills=%d"),
		Winner ? *Winner->GetName() : TEXT("None"), WinnerKills);

	BP_OnMatchEnded(Winner, WinnerKills);
	EndMatch();
}

void AApexDeathmatchGameMode::HandleMatchHasEnded()
{
	GetWorldTimerManager().ClearTimer(MatchTimerHandle);
	ClearAllRespawnTimers();
	Super::HandleMatchHasEnded();
}

void AApexDeathmatchGameMode::SpawnBulletPool()
{
	if (!BulletPoolManagerClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApexGM] BulletPoolManagerClass not set"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (AActor* Existing = UGameplayStatics::GetActorOfClass(World, ABulletPoolManager::StaticClass()))
	{
		SpawnedBulletPool = Cast<ABulletPoolManager>(Existing);
		UE_LOG(LogTemp, Log, TEXT("[ApexGM] BulletPool already exists"));
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	SpawnedBulletPool = World->SpawnActor<ABulletPoolManager>(
		BulletPoolManagerClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams);

	if (SpawnedBulletPool)
	{
		UE_LOG(LogTemp, Log, TEXT("[ApexGM] BulletPool spawned"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ApexGM] BulletPool spawn FAILED"));
	}
}
