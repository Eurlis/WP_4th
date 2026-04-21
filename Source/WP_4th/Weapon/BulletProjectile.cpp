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

	// BulletMesh가 비어있으면 기본 Engine Cylinder로 대체 (길쭉한 탄환 느낌, BP에서 덮어쓸 수 있음)
	if (BulletMesh && !BulletMesh->GetStaticMesh())
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultCylinder(TEXT("/Engine/BasicShapes/Cylinder"));
		if (DefaultCylinder.Succeeded())
		{
			BulletMesh->SetStaticMesh(DefaultCylinder.Object);
			// X/Y: 얇게, Z: 길게 (탄환 비율)
			BulletMesh->SetRelativeScale3D(FVector(0.03f, 0.03f, 0.08f));
			// Cylinder 기본 축은 Z(위쪽)이지만, bRotationFollowsVelocity 로 X축(Forward) 회전되므로
			// 메시를 90도 Pitch 회전시켜 Z축이 Forward 를 향하도록 보정
			BulletMesh->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
			BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}
