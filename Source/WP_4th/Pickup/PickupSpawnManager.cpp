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

	// 3. 각 SpawnPoint마다 픽업 스폰 + 무기 옆 탄창 그룹 스폰
	int32 WeaponSpawned = 0;
	int32 AmmoSpawned = 0;
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
			++WeaponSpawned;

			// 그룹 스폰: 실제 총기(Throwable/Ammo/None 제외) 옆에 호환 탄창 1~2개
			const FWeaponData* Data = WeaponDataTable->FindRow<FWeaponData>(
				SelectedID, TEXT("PickupSpawnManager::SpawnAllPickups GroupAmmo"));
			if (Data)
			{
				const bool bIsActualWeapon =
					(Data->Category != EWeaponType::Throwable
					 && Data->Category != EWeaponType::Ammo
					 && Data->Category != EWeaponType::None);
				if (bIsActualWeapon)
				{
					AmmoSpawned += SpawnAmmoNearWeapon(Pickup->GetActorLocation(), Rot, Data->AmmoType);
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[SpawnManager] Spawned %d weapons + %d ammo pickups (group spawn) from %d points"),
		WeaponSpawned, AmmoSpawned, SpawnPointActors.Num());
}

FName APickupSpawnManager::GetAmmoPickupRowNameFromAmmoType(EAmmoType Type)
{
	switch (Type)
	{
	case EAmmoType::Light:   return TEXT("AmmoLight");
	case EAmmoType::Heavy:   return TEXT("AmmoHeavy");
	case EAmmoType::Energy:  return TEXT("AmmoEnergy");
	case EAmmoType::Shotgun: return TEXT("AmmoShotgun");
	case EAmmoType::Sniper:  return TEXT("AmmoHeavy"); // S16+ Heavy로 통합
	default:                 return NAME_None;
	}
}

int32 APickupSpawnManager::SpawnAmmoNearWeapon(const FVector& WeaponLocation, const FRotator& WeaponRotation, EAmmoType AmmoType)
{
	if (!WeaponDataTable || !PickupClass)
	{
		return 0;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	const FName AmmoRowName = GetAmmoPickupRowNameFromAmmoType(AmmoType);
	if (AmmoRowName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnManager] No ammo mapping for AmmoType %d"), (int32)AmmoType);
		return 0;
	}

	const FWeaponData* AmmoData = WeaponDataTable->FindRow<FWeaponData>(
		AmmoRowName, TEXT("SpawnAmmoNearWeapon"));
	if (!AmmoData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnManager] DT row not found: %s"), *AmmoRowName.ToString());
		return 0;
	}

	const int32 RequestedCount = FMath::RandRange(1, 2);

	UE_LOG(LogTemp, Log, TEXT("[SpawnManager] Spawning %d ammo pickups (%s) near weapon"),
		RequestedCount, *AmmoRowName.ToString());

	// 무기 우측 방향 (회전 반영). 회전 0 이면 World +Y
	const FVector RightDir = WeaponRotation.RotateVector(FVector::RightVector);

	int32 SpawnedThisGroup = 0;
	for (int32 i = 0; i < RequestedCount; ++i)
	{
		const float OffsetDist = AmmoDistanceFromWeapon + i * AmmoSpacingBetween;
		const FVector AmmoLocation = WeaponLocation + RightDir * OffsetDist;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APickupBase* AmmoPickup = World->SpawnActor<APickupBase>(
			PickupClass, AmmoLocation, WeaponRotation, SpawnParams);

		if (AmmoPickup)
		{
			AmmoPickup->PickupWeaponID = AmmoRowName;
			AmmoPickup->RefreshFromDataTable();
			SpawnedPickups.Add(AmmoPickup);
			++SpawnedThisGroup;

			UE_LOG(LogTemp, Verbose, TEXT("[SpawnManager] Ammo spawned at %s"), *AmmoLocation.ToString());
		}
	}

	return SpawnedThisGroup;
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
