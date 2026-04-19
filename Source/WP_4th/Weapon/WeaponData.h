// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "WeaponTypes.h"
#include "WeaponData.generated.h"

class AProjectileBase;
class USkeletalMesh;
class UTexture2D;
class UParticleSystem;
class USoundBase;

UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
	Firearm,
	Throwable
};

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	// ===== 기본 정보 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Info")
	FName DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Info")
	EWeaponCategory Category = EWeaponCategory::Firearm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Info")
	UTexture2D* WeaponIcon = nullptr;

	// ===== 메시 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	USkeletalMesh* WeaponMesh1P = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	USkeletalMesh* WeaponMesh3P = nullptr;

	// ===== 공통 스탯 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float BaseDamage = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float HeadshotMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float LegMultiplier = 0.75f;

	// ===== 총기 전용 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm")
	float FireRate = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm")
	float WeaponRange = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm")
	int32 MaxAmmo = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm")
	float ReloadTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm")
	EFireMode FireMode = EFireMode::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm")
	EAmmoType AmmoType = EAmmoType::Light;

	// ===== 투사체 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	float BulletSpeed = 30000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile")
	float BulletGravityScale = 0.3f;

	// ===== 반동 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil")
	float RecoilPitchMin = -0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil")
	float RecoilPitchMax = -0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil")
	float RecoilYawMin = -0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil")
	float RecoilYawMax = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Recoil")
	float RecoilRecoverySpeed = 5.0f;

	// ===== ADS =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ADS")
	float ADSFOVMultiplier = 0.7f;

	// ===== 산탄총 전용 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shotgun")
	bool bIsShotgun = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shotgun", meta=(EditCondition="bIsShotgun"))
	int32 PelletCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shotgun", meta=(EditCondition="bIsShotgun"))
	float SpreadAngle = 5.0f;

	// ===== 투척 전용 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	float ThrowForce = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	float FuseTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	float ExplosionDamage = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	float ExplosionRadius = 500.f;

	// ===== 이펙트/사운드 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	UParticleSystem* MuzzleFlashFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	USoundBase* FireSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	USoundBase* ReloadSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	UParticleSystem* ExplosionFX = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	USoundBase* ExplosionSound = nullptr;
};
