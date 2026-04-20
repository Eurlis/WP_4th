// Fill out your copyright notice in the Description page of Project Settings.

#include "BulletProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"

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

	// BulletMesh가 비어있으면 기본 Engine Sphere로 대체 (BP에서 덮어쓸 수 있음)
	if (BulletMesh && !BulletMesh->GetStaticMesh())
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultSphere(TEXT("/Engine/BasicShapes/Sphere"));
		if (DefaultSphere.Succeeded())
		{
			BulletMesh->SetStaticMesh(DefaultSphere.Object);
			BulletMesh->SetRelativeScale3D(FVector(0.05f));
			BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}
