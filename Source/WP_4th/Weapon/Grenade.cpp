// Fill out your copyright notice in the Description page of Project Settings.

#include "Grenade.h"

AGrenade::AGrenade()
{
	// 수류탄: 2초 퓨즈, 데미지80, 반경5m(500UU)
	FuseTime = 2.0f;
	ExplosionDamage = 80.f;
	ExplosionRadius = 500.f;
	ThrowForce = 2000.f;
}
