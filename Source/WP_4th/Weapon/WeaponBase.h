// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "WeaponData.h"
#include "WeaponBase.generated.h"

class ABulletPoolManager;
class UDataTable;

UCLASS(Abstract)
class WP_4TH_API AWeaponBase : public AItemBase
{
	GENERATED_BODY()

public:
	AWeaponBase();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ========== Components ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	USkeletalMeshComponent* WeaponMesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	USkeletalMeshComponent* WeaponMesh3P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	USceneComponent* MuzzlePoint;

	// ========== Stats ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float BaseDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float HeadshotMultiplier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float LegMultiplier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float FireRate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float WeaponRange;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	int32 MaxAmmo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	float ReloadTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	EFireMode FireMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Stats")
	EAmmoType AmmoType;

	// ========== Projectile ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile")
	float BulletSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile")
	float BulletGravityScale;

	UPROPERTY()
	ABulletPoolManager* BulletPool;

	// ========== Recoil ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilPitchMin;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilPitchMax;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilYawMin;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilYawMax;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Recoil")
	float RecoilRecoverySpeed;

	// ========== ADS ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|ADS")
	float ADSFOVMultiplier;

	// ========== Data ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Data")
	UDataTable* WeaponDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_WeaponID, Category = "Weapon|Data")
	FName WeaponID;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Data")
	FWeaponData CurrentWeaponData;

	// ========== Runtime ==========
	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo, BlueprintReadOnly, Category = "Weapon|Runtime")
	int32 CurrentAmmo;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Runtime")
	bool bIsReloading;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Runtime")
	bool bIsFiring;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|ADS")
	bool bIsAiming = false;

	// ========== Burst ==========
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Burst")
	bool bIsBursting = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Burst")
	int32 CurrentBurstCount = 0;

	// ========== ADS ==========
	UFUNCTION(BlueprintCallable, Category = "Weapon|ADS")
	void StartAiming();

	UFUNCTION(BlueprintCallable, Category = "Weapon|ADS")
	void StopAiming();

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

	UFUNCTION(BlueprintPure, Category = "Weapon|ADS")
	float GetADSFOVMultiplier() const;

	// ========== Data ==========
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void InitFromDataTable(FName InWeaponID);

	UFUNCTION()
	void OnRep_WeaponID();

	virtual void BeginPlay() override;

	// ========== Muzzle ==========
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FVector GetMuzzleLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FVector GetMuzzleForward() const;

	/** 카메라 ray로 LineTrace해서 조준 목표 지점(크로스헤어 끝점) 산출 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FVector CalculateAimTarget() const;

	// ========== Fire ==========
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Fire")
	float LastFireTime;

	UFUNCTION(BlueprintPure, Category = "Weapon|Fire")
	bool CanFireNow() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StopFire();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(FVector MuzzleLocation, FVector AimDirection);

	virtual void ProcessHit(const FVector& MuzzleLocation, const FVector& AimDirection);

	// ========== Burst (서버 권한) ==========
	void StartBurstFire(const FVector& MuzzleLocation, const FVector& AimDirection);
	void FireBurstShot();
	void EndBurstFire();

	// ========== Reload ==========
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartReload();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStartReload();

	// ========== Equip ==========
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void OnEquipped();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void OnUnequipped();

	// ========== Multicast ==========
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFireEffects(FVector MuzzleLocation, FVector TraceEnd);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSpawnMuzzleFlash();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayEquipSound();

protected:
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	FTimerHandle BurstTimerHandle;

	// Burst 시작 시점의 총구 / 에임 캐시 (서버에서만 사용)
	FVector CachedBurstMuzzle = FVector::ZeroVector;
	FVector CachedBurstDir = FVector::ForwardVector;

	// Recoil accumulation (local)
	float CurrentRecoilPitch;
	float CurrentRecoilYaw;

	void FireShot();
	void FinishReload();
	void PerformLineTrace(const FVector& Start, const FVector& Direction, FHitResult& OutHit) const;
	void ApplyDamage(const FHitResult& HitResult, float Damage);
	void ApplyRecoil();
	void RecoverRecoil(float DeltaTime);
	void ApplyWeaponData(const FWeaponData& Data);
	void FireProjectile(const FVector& MuzzleLocation, const FVector& Direction);

	// WP4-43: 샷건 스프레드 패턴별 방향 계산
	FVector CalculateSpreadDirection(int32 Index, int32 Total, const FVector& AimDir, float SpreadRad) const;

	UFUNCTION()
	void OnRep_CurrentAmmo();
};
