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

	// PickupBase::SnapToGround 가 BeginPlay 에서 바닥으로 자동 보정함. 기본 0 권장.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	float SpawnZOffset = 0.0f;

	// 무기 우측 첫 탄창까지 거리 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn|Ammo")
	float AmmoDistanceFromWeapon = 50.0f;

	// 탄창 간 간격 (i 번째 탄창은 AmmoDistanceFromWeapon + i * AmmoSpacingBetween 만큼 떨어짐)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn|Ammo")
	float AmmoSpacingBetween = 30.0f;

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
	int32 SpawnAmmoNearWeapon(const FVector& WeaponLocation, const FRotator& WeaponRotation, EAmmoType AmmoType);
};
