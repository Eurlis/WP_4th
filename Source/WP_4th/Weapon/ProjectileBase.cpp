// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Kismet/GameplayStatics.h"

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
	UE_LOG(LogTemp, Warning, TEXT("[Bullet] Activate: Loc=%s, Dir=%s, Speed=%.1f"),
		*SpawnLocation.ToString(), *Direction.ToString(), InSpeed);

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

		UE_LOG(LogTemp, Log, TEXT("[Bullet] ProjectileMovement activated, Velocity=%s"),
			*ProjectileMovement->Velocity.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Bullet] ProjectileMovement is NULL!"));
	}

	bIsActive = true;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	if (BulletMesh)
	{
		UE_LOG(LogTemp, Log, TEXT("[Bullet] Mesh visible: %s, StaticMesh=%s"),
			BulletMesh->IsVisible() ? TEXT("YES") : TEXT("NO"),
			BulletMesh->GetStaticMesh() ? *BulletMesh->GetStaticMesh()->GetName() : TEXT("NULL"));
	}

	GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);
	GetWorldTimerManager().SetTimer(LifeSpanTimerHandle, this, &AProjectileBase::Deactivate, LifeSpan, false);

	// 디버그: 총알 시작 위치 (노란 구, 3초) + 초기 방향 라인
	if (UWorld* World = GetWorld())
	{
		DrawDebugSphere(World, SpawnLocation, 15.0f, 12, FColor::Yellow, false, 3.0f);
		DrawDebugLine(World, SpawnLocation, SpawnLocation + NormalizedDir * 500.f, FColor::Yellow, false, 1.0f, 0, 1.0f);
	}

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

	// 다른 투사체와의 충돌 무시 (산탄총 펠릿끼리 폭발 방지)
	if (Cast<AProjectileBase>(OtherActor)) return;

	float FinalDamage = Damage;
	FName BoneName = Hit.BoneName;

	ACharacter* HitChar = Cast<ACharacter>(OtherActor);
	const bool bHitCharacter = (HitChar != nullptr);

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

	// 임팩트 이펙트 브로드캐스트 (모든 클라이언트 동기화) - Deactivate 전에 호출
	MulticastSpawnImpactEffects(Hit.ImpactPoint, Hit.ImpactNormal, OtherComp, bHitCharacter);

#if !UE_BUILD_SHIPPING
	DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.f, 12, FColor::Red, false, 2.0f);
#endif
	UE_LOG(LogTemp, Log, TEXT("[Projectile] Hit: %s, Damage=%.1f, Bone=%s"),
		OtherActor ? *OtherActor->GetName() : TEXT("None"), FinalDamage, *BoneName.ToString());

	Deactivate();
}

void AProjectileBase::SetWeaponData(const FWeaponData& InWeaponData)
{
	CachedWeaponData = InWeaponData;
}

void AProjectileBase::MulticastSpawnImpactEffects_Implementation(FVector ImpactLocation, FVector ImpactNormal, UPrimitiveComponent* HitComp, bool bHitCharacter)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (bHitCharacter)
	{
		// === 캐릭터 피격: 피 파티클 + 사운드 ===
		if (CachedWeaponData.BloodImpactFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				CachedWeaponData.BloodImpactFX,
				ImpactLocation,
				ImpactNormal.Rotation()
			);
		}

		if (CachedWeaponData.BloodImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				World,
				CachedWeaponData.BloodImpactSound,
				ImpactLocation
			);
		}

		UE_LOG(LogTemp, Log, TEXT("[Impact] Blood effect at %s"), *ImpactLocation.ToString());
	}
	else
	{
		// === 벽/바닥: 파티클 + 데칼 + 사운드 ===
		if (CachedWeaponData.BulletImpactFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				CachedWeaponData.BulletImpactFX,
				ImpactLocation,
				ImpactNormal.Rotation()
			);
		}

		// 데칼은 HitComp에 부착 (움직이는 오브젝트 대응)
		if (CachedWeaponData.BulletImpactDecal && HitComp)
		{
			UDecalComponent* Decal = UGameplayStatics::SpawnDecalAttached(
				CachedWeaponData.BulletImpactDecal,
				CachedWeaponData.BulletDecalSize,
				HitComp,
				NAME_None,
				ImpactLocation,
				ImpactNormal.Rotation(),
				EAttachLocation::KeepWorldPosition,
				CachedWeaponData.BulletDecalLifeSpan
			);

			if (Decal)
			{
				UE_LOG(LogTemp, Log, TEXT("[Impact] Decal spawned (life: %.1fs)"), CachedWeaponData.BulletDecalLifeSpan);
			}
		}

		if (CachedWeaponData.BulletImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				World,
				CachedWeaponData.BulletImpactSound,
				ImpactLocation
			);
		}

		UE_LOG(LogTemp, Log, TEXT("[Impact] Wall effects at %s"), *ImpactLocation.ToString());
	}
}
