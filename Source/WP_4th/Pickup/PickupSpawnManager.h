#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon/WeaponTypes.h"
#include "PickupSpawnManager.generated.h"

class UDataTable;
class APickupBase;

UCLASS()
class WP_4TH_API APickupSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnManager();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	UDataTable* WeaponDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TSubclassOf<APickupBase> PickupClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	float SpawnZOffset = 50.0f;

	// 무기 옆 탄창 그룹 스폰 시 사용하는 XY 반경 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	float AmmoGroupSpawnRadius = 150.0f;

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void SpawnAllPickups();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void ClearAllPickups();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TArray<APickupBase*> SpawnedPickups;

private:
	// AmmoType → 탄창 픽업 RowName 매핑 (Sniper는 Heavy로 통합)
	static FName GetAmmoPickupRowNameFromAmmoType(EAmmoType Type);

	// 무기 위치 옆에 호환 탄창 1~2개 스폰. 실제 스폰된 개수 반환.
	int32 SpawnAmmoNearWeapon(const FVector& WeaponLocation, EAmmoType AmmoType);
};
