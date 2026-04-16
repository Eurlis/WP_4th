// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon_Shotgun.h"
#include "DrawDebugHelpers.h"

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
	float SpreadAngleRad = FMath::DegreesToRadians(SpreadAngle);

	for (int32 i = 0; i < PelletCount; i++)
	{
		// 각 펠릿에 랜덤 스프레드 적용
		FVector PelletDirection = FMath::VRandCone(AimDirection, SpreadAngleRad);

		FHitResult HitResult;
		PerformLineTrace(MuzzleLocation, PelletDirection, HitResult);

		FVector TraceEnd = MuzzleLocation + PelletDirection * WeaponRange;

		if (HitResult.bBlockingHit)
		{
			TraceEnd = HitResult.ImpactPoint;
			ApplyDamage(HitResult, BaseDamage);
		}

		// 각 펠릿에 대해 디버그 라인
		DrawDebugLine(GetWorld(), MuzzleLocation, TraceEnd, FColor::Orange, false, 1.0f, 0, 0.5f);
	}

	// 마지막 펠릿의 end 위치로 이펙트 (대표)
	FVector RepresentativeEnd = MuzzleLocation + AimDirection * WeaponRange;
	MulticastFireEffects(MuzzleLocation, RepresentativeEnd);
}
