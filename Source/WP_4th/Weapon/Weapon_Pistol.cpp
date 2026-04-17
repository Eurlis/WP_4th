// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon_Pistol.h"

AWeapon_Pistol::AWeapon_Pistol()
{
	// 권총: 반자동, 데미지18, 탄창12, 히트스캔
	BaseDamage = 18.f;
	FireRate = 0.2f;		// 300 RPM
	MaxAmmo = 12;
	CurrentAmmo = MaxAmmo;
	WeaponRange = 8000.f;
	ReloadTime = 1.5f;
	FireMode = EFireMode::Semi;
	AmmoType = EAmmoType::Light;

	// Projectile (250m/s)
	BulletSpeed = 25000.f;
	BulletGravityScale = 0.4f;

	// Recoil
	RecoilPitchMin = -0.5f;
	RecoilPitchMax = -0.8f;
	RecoilYawMin = -0.1f;
	RecoilYawMax = 0.1f;
	RecoilRecoverySpeed = 8.f;

	// ADS
	ADSFOVMultiplier = 0.8f;
}
