#include "PickupSpawnManager.h"
#include "PickupBase.h"
#include "PickupSpawnPoint.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/WeaponData.h"
#include "Weapon/WeaponTypes.h"

APickupSpawnManager::APickupSpawnManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
}

void APickupSpawnManager::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnAllPickups();
	}
}

void APickupSpawnManager::SpawnAllPickups()
{
	if (!WeaponDataTable || !PickupClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnManager] WeaponDataTable or PickupClass not set"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. 월드의 모든 SpawnPoint 수집
	TArray<AActor*> SpawnPointActors;
	UGameplayStatics::GetAllActorsOfClass(World, APickupSpawnPoint::StaticClass(), SpawnPointActors);

	if (SpawnPointActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnManager] No PickupSpawnPoint actors in level"));
		return;
	}

	// 2. DT_Weapons 카테고리별 ID 풀 수집
	TArray<FName> WeaponIDs;
	TArray<FName> ThrowableIDs;
	WeaponDataTable->ForeachRow<FWeaponData>(TEXT("PickupSpawnManager::SpawnAllPickups"),
		[&WeaponIDs, &ThrowableIDs](const FName& Key, const FWeaponData& Row)
		{
			if (Row.Category == EWeaponType::Throwable)
			{
				ThrowableIDs.Add(Key);
			}
			else if (Row.Category != EWeaponType::None)
			{
				WeaponIDs.Add(Key);
			}
		});

	// 3. 각 SpawnPoint마다 픽업 스폰
	int32 SpawnedCount = 0;
	for (AActor* PointActor : SpawnPointActors)
	{
		APickupSpawnPoint* SP = Cast<APickupSpawnPoint>(PointActor);
		if (!SP)
		{
			continue;
		}

		// 가능한 풀 결정
		TArray<FName> AvailablePool;
		if (SP->AllowedWeaponIDs.Num() > 0)
		{
			AvailablePool = SP->AllowedWeaponIDs;
		}
		else
		{
			if (SP->bAllowWeapons)
			{
				AvailablePool.Append(WeaponIDs);
			}
			if (SP->bAllowThrowables)
			{
				AvailablePool.Append(ThrowableIDs);
			}
		}

		if (AvailablePool.Num() == 0)
		{
			continue;
		}

		const FName SelectedID = AvailablePool[FMath::RandRange(0, AvailablePool.Num() - 1)];

		const FVector Loc = SP->GetActorLocation() + FVector(0.f, 0.f, SpawnZOffset);
		const FRotator Rot = SP->GetActorRotation();

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APickupBase* Pickup = World->SpawnActor<APickupBase>(PickupClass, Loc, Rot, Params);
		if (Pickup)
		{
			Pickup->PickupWeaponID = SelectedID;
			Pickup->RefreshFromDataTable();
			SpawnedPickups.Add(Pickup);
			++SpawnedCount;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[SpawnManager] Spawned %d pickups from %d points"),
		SpawnedCount, SpawnPointActors.Num());
}

void APickupSpawnManager::ClearAllPickups()
{
	if (!HasAuthority())
	{
		return;
	}

	for (APickupBase* P : SpawnedPickups)
	{
		if (IsValid(P))
		{
			P->Destroy();
		}
	}
	SpawnedPickups.Reset();
}
