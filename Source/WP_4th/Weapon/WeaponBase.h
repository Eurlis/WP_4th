// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "WeaponBase.generated.h"

class ABulletPoolManager;

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	Auto	UMETA(DisplayName = "Full Auto"),
	Semi	UMETA(DisplayName = "Semi Auto"),
	Pump	UMETA(DisplayName = "Pump Action")
};

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	Light		UMETA(DisplayName = "Light"),
	Shotgun		UMETA(DisplayName = "Shotgun")
};

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

	// ========== Runtime ==========
	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo, BlueprintReadOnly, Category = "Weapon|Runtime")
	int32 CurrentAmmo;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Runtime")
	bool bIsReloading;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Runtime")
	bool bIsFiring;

	// ========== Fire ==========
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StartFire();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void StopFire();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(FVector MuzzleLocation, FVector AimDirection);

	virtual void ProcessHit(const FVector& MuzzleLocation, const FVector& AimDirection);

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

protected:
	FTimerHandle FireTimerHandle;
	FTimerHandle ReloadTimerHandle;
	float LastFireTime;

	// Recoil accumulation (local)
	float CurrentRecoilPitch;
	float CurrentRecoilYaw;

	void FireShot();
	void FinishReload();
	void PerformLineTrace(const FVector& Start, const FVector& Direction, FHitResult& OutHit) const;
	void ApplyDamage(const FHitResult& HitResult, float Damage);
	void ApplyRecoil();
	void RecoverRecoil(float DeltaTime);

	UFUNCTION()
	void OnRep_CurrentAmmo();
};
