// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NiagaraSystem.h"
#include "WeaponTypes.h"
#include "WeaponData.generated.h"

class AProjectileBase;
class AFireZone;
class USkeletalMesh;
class UStaticMesh;
class UTexture2D;
class UParticleSystem;
class USoundBase;
class UNiagaraSystem;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
	Firearm,
	Throwable
};

UENUM(BlueprintType)
enum class EShotgunSpreadPattern : uint8
{
	Random      UMETA(DisplayName = "Random (VRandCone)"),
	Circular    UMETA(DisplayName = "Circular (Peacekeeper, EVA-8)"),
	Horizontal  UMETA(DisplayName = "Horizontal (Mastiff)"),
	Vertical    UMETA(DisplayName = "Vertical (확장용)"),
	Cross       UMETA(DisplayName = "Cross (확장용)")
};

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	// ===== 기본 정보 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Info")
	FName DisplayName;

	/** CSV 주도 무기 카테고리 (Rifle / Pistol / Shotgun / Sniper / Throwable) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Info")
	FString Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Info")
	UTexture2D* WeaponIcon = nullptr;

	/** 탄종 아이콘 (HUD 표시용 - Light/Heavy/Energy/Sniper/Shotgun) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Info")
	UTexture2D* AmmoIcon = nullptr;

	// ===== 메시 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	USkeletalMesh* WeaponMesh1P = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	USkeletalMesh* WeaponMesh3P = nullptr;

	// ===== Mesh Transform (각 무기별 크기/회전 다름) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	FVector MeshScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	FRotator MeshRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Mesh")
	FVector MeshLocationOffset = FVector::ZeroVector;

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

	// ===== Burst 사격 (FireMode == Burst 일 때만 사용) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm|Burst")
	int32 BurstShotCount = 3;  // Hemlok=3, Nemesis=4

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm|Burst")
	float BurstInterval = 0.06f;  // 버스트 내 발사 간격(초)

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shotgun", meta=(EditCondition="bIsShotgun"))
	EShotgunSpreadPattern SpreadPattern = EShotgunSpreadPattern::Random;

	// ===== 투척 전용 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	float ThrowForce = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	float FuseTime = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	float ExplosionDamage = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	float ExplosionRadius = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	bool bIsSticky = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable", meta=(EditCondition="bIsSticky"))
	float StickyDamage = 10.f;

	// ===== 투척 궤적 커스터마이징 (Apex 스타일) =====
	// 권장값:
	//   FragGrenade: ThrowForce=2800, GravityScale=2.0, Bounciness=0.3
	//   Thermite:    ThrowForce=2800, GravityScale=2.0, Bounciness=0.0
	//   ArcStar:     ThrowForce=3200, GravityScale=1.5, Bounciness=0.0
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Physics")
	float ThrowableGravityScale = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Physics")
	float ThrowableBounciness = 0.3f;

	// ===== 소이탄 (Incendiary) - Thermite 등 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Incendiary")
	bool bIsIncendiary = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Incendiary", meta=(EditCondition="bIsIncendiary"))
	float FireZoneDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Incendiary", meta=(EditCondition="bIsIncendiary"))
	float FireZoneTickInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Incendiary", meta=(EditCondition="bIsIncendiary"))
	float FireZoneDamagePerTick = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Incendiary", meta=(EditCondition="bIsIncendiary"))
	FVector FireZoneExtent = FVector(200.0f, 800.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Incendiary", meta=(EditCondition="bIsIncendiary"))
	TSubclassOf<AFireZone> FireZoneClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	UStaticMesh* ThrowableMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	FVector ThrowableMeshScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable")
	FRotator ThrowableMeshRotation = FRotator::ZeroRotator;

	// ===== 이펙트/사운드 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	TObjectPtr<UNiagaraSystem> MuzzleFlashFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	TObjectPtr<UNiagaraSystem> BulletTracerFX;

	// ===== 장착 사운드 (모든 무기 공통) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sound")
	TObjectPtr<USoundBase> EquipSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	USoundBase* FireSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	USoundBase* ReloadSound = nullptr;

	// ===== 수류탄 던지기 사운드 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Sound")
	TObjectPtr<USoundBase> ThrowSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	TObjectPtr<UNiagaraSystem> ExplosionFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FX")
	USoundBase* ExplosionSound = nullptr;

	// ===== Thermite 소이 화염 (지속 이펙트) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Incendiary")
	TObjectPtr<UNiagaraSystem> FireFX;

	// ===== ArcStar 전기/감전 이펙트 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Throwable|Electric")
	TObjectPtr<UNiagaraSystem> ShockFX;

	// ===== 임팩트 이펙트 (벽/바닥) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm|Impact")
	TObjectPtr<UNiagaraSystem> BulletImpactFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm|Impact")
	TObjectPtr<UMaterialInterface> BulletImpactDecal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm|Impact")
	TObjectPtr<USoundBase> BulletImpactSound;

	// ===== 피격 이펙트 (캐릭터) =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm|Impact")
	TObjectPtr<UNiagaraSystem> BloodImpactFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm|Impact")
	TObjectPtr<USoundBase> BloodImpactSound;

	// ===== 데칼 설정 =====
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm|Impact")
	FVector BulletDecalSize = FVector(8.0f, 8.0f, 8.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Firearm|Impact")
	float BulletDecalLifeSpan = 15.0f;
};
