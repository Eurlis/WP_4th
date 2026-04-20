// Fill out your copyright notice in the Description page of Project Settings.

#include "BulletProjectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

ABulletProjectile::ABulletProjectile()
{
	// 총알: AR 기준 탄속 300m/s, 약간의 탄낙차
	BulletSpeed = 30000.f;
	GravityScale = 0.3f;
	LifeSpan = 3.0f;

	if (CollisionComp)
	{
		CollisionComp->InitSphereRadius(2.0f);
	}
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = BulletSpeed;
		ProjectileMovement->MaxSpeed = BulletSpeed * 2.f;
		ProjectileMovement->ProjectileGravityScale = GravityScale;
	}
}
