#include "ApexDeathmatchGameMode.h"

#include "ApexDeathmatchGameState.h"
#include "ApexDeathmatchPlayerState.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/BulletPoolManager.h"

AApexDeathmatchGameMode::AApexDeathmatchGameMode()
{
	GameStateClass = AApexDeathmatchGameState::StaticClass();
	PlayerStateClass = AApexDeathmatchPlayerState::StaticClass();

	BulletPoolManagerClass = nullptr;
	SpawnedBulletPool = nullptr;
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
