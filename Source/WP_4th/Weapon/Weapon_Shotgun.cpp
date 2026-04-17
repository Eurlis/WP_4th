// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon_Shotgun.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "BulletPoolManager.h"
#include "ProjectileBase.h"

AWeapon_Shotgun::AWeapon_Shotgun()
{
	// 산탄총: 펌프액션, 펠릿8발 x 11데미지, 탄창6, 히트스캔
	BaseDamage = 11.f;
	FireRate = 0.8f;		// 펌프 간격
	MaxAmmo = 6;
	CurrentAmmo = MaxAmmo;
	WeaponRange = 5000.f;
	ReloadTime = 2.5f;
	FireMode = EFireMode::Pump;
	AmmoType = EAmmoType::Shotgun;

	PelletCount = 8;
	SpreadAngle = 5.f;

	// Projectile (산탄은 느리고 탄낙차 큼)
	BulletSpeed = 20000.f;
	BulletGravityScale = 0.5f;

	// Recoil
	RecoilPitchMin = -1.0f;
	RecoilPitchMax = -1.5f;
	RecoilYawMin = -0.3f;
	RecoilYawMax = 0.3f;
	RecoilRecoverySpeed = 3.f;

	// ADS
	ADSFOVMultiplier = 0.85f;
}

void AWeapon_Shotgun::ProcessHit(const FVector& MuzzleLocation, const FVector& AimDirection)
{
	// Pool 매니저 지연 바인딩
	if (!BulletPool)
	{
		BulletPool = Cast<ABulletPoolManager>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ABulletPoolManager::StaticClass()));
	}

	const float SpreadAngleRad = FMath::DegreesToRadians(SpreadAngle);
	bool bAnyFiredAsProjectile = false;

	for (int32 i = 0; i < PelletCount; i++)
	{
		FVector PelletDirection = FMath::VRandCone(AimDirection, SpreadAngleRad);

		if (BulletPool)
		{
			AProjectileBase* Bullet = BulletPool->GetProjectile();
			if (Bullet)
			{
				Bullet->Activate(MuzzleLocation, PelletDirection, BaseDamage, BulletSpeed, BulletGravityScale, OwningCharacter);
				bAnyFiredAsProjectile = true;
				continue;
			}
		}

		// 폴백: 풀 없거나 고갈 시 히트스캔 펠릿
		FHitResult HitResult;
		PerformLineTrace(MuzzleLocation, PelletDirection, HitResult);

		FVector TraceEnd = MuzzleLocation + PelletDirection * WeaponRange;
		if (HitResult.bBlockingHit)
		{
			TraceEnd = HitResult.ImpactPoint;
			ApplyDamage(HitResult, BaseDamage);
		}

		DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::Orange, false, 1.0f, 0, 0.5f);
	}

	FVector RepresentativeEnd = MuzzleLocation + AimDirection * WeaponRange;
	MulticastFireEffects(MuzzleLocation, RepresentativeEnd);
}
