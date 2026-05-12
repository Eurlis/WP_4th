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
