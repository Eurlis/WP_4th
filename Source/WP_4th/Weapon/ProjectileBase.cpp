// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"

AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Collision (root)
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(2.0f);
	CollisionComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->OnComponentHit.AddDynamic(this, &AProjectileBase::OnHit);
	RootComponent = CollisionComp;

	// Visual mesh (optional)
	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	BulletMesh->SetupAttachment(CollisionComp);
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Projectile movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 30000.f;
	ProjectileMovement->MaxSpeed = 60000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.3f;

	// Stats defaults
	Damage = 14.f;
	BulletSpeed = 30000.f;
	GravityScale = 0.3f;
	LifeSpan = 3.0f;
	HeadshotMultiplier = 2.0f;
	LegMultiplier = 0.75f;

	bIsActive = false;
	OwnerCharacter = nullptr;
	OwnerController = nullptr;
}

void AProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AProjectileBase::Activate(FVector SpawnLocation, FVector Direction, float InDamage, float InSpeed, float InGravity, ACharacter* Shooter)
{
	Damage = InDamage;
	BulletSpeed = InSpeed;
	GravityScale = InGravity;
	OwnerCharacter = Shooter;
	OwnerController = Shooter ? Shooter->GetController() : nullptr;

	FVector NormalizedDir = Direction.GetSafeNormal();

	SetActorLocation(SpawnLocation);
	SetActorRotation(NormalizedDir.Rotation());

	if (ProjectileMovement)
	{
		ProjectileMovement->ProjectileGravityScale = GravityScale;
		ProjectileMovement->Velocity = NormalizedDir * BulletSpeed;
		ProjectileMovement->SetUpdatedComponent(CollisionComp);
		ProjectileMovement->Activate(true);
	}

	bIsActive = true;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);
	GetWorldTimerManager().SetTimer(LifeSpanTimerHandle, this, &AProjectileBase::Deactivate, LifeSpan, false);

	DrawDebugLine(GetWorld(), SpawnLocation, SpawnLocation + NormalizedDir * 500.f, FColor::Yellow, false, 1.0f, 0, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("[Projectile] Activated: Speed=%.1f, Direction=%s"), BulletSpeed, *NormalizedDir.ToString());
}

void AProjectileBase::Deactivate()
{
	bIsActive = false;

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	SetActorLocation(FVector(0.f, 0.f, -10000.f));

	GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);
}

void AProjectileBase::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bIsActive) return;
	if (OtherActor == this || OtherActor == OwnerCharacter) return;

	float FinalDamage = Damage;
	FName BoneName = Hit.BoneName;

	ACharacter* HitChar = Cast<ACharacter>(OtherActor);
	if (HitChar)
	{
		if (BoneName == FName("head"))
		{
			FinalDamage *= HeadshotMultiplier;
		}
		else if (BoneName == FName("thigh_l") || BoneName == FName("thigh_r") ||
		         BoneName == FName("calf_l") || BoneName == FName("calf_r") ||
		         BoneName == FName("foot_l") || BoneName == FName("foot_r"))
		{
			FinalDamage *= LegMultiplier;
		}

		// TODO: ServerApplyDamage(FinalDamage, OwnerCharacter, Hit) — 캐릭터 베이스에서 구현 예정
	}

	DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.f, 12, FColor::Red, false, 2.0f);
	UE_LOG(LogTemp, Log, TEXT("[Projectile] Hit: %s, Damage=%.1f, Bone=%s"),
		OtherActor ? *OtherActor->GetName() : TEXT("None"), FinalDamage, *BoneName.ToString());

	// TODO: 탄흔 이펙트 스폰

	Deactivate();
}
