// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon_AR.h"

AWeapon_AR::AWeapon_AR()
{
	// 돌격소총: 풀오토, 데미지14, 탄창20, 히트스캔
	BaseDamage = 14.f;
	FireRate = 0.1f;		// 600 RPM
	MaxAmmo = 20;
	CurrentAmmo = MaxAmmo;
	WeaponRange = 10000.f;
	ReloadTime = 2.0f;
	FireMode = EFireMode::Auto;
	AmmoType = EAmmoType::Light;

	// Recoil
	RecoilPitchMin = -0.3f;
	RecoilPitchMax = -0.6f;
	RecoilYawMin = -0.15f;
	RecoilYawMax = 0.15f;
	RecoilRecoverySpeed = 5.f;

	// ADS
	ADSFOVMultiplier = 0.7f;
}
