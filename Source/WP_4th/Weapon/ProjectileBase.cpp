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
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Collision (root)
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(2.0f);

	// 펠릿끼리 Block되어 총구에 뭉쳐 정지하던 버그(WP4-42) 방지:
	// Projectile Custom Channel(ECC_GameTraceChannel1) 사용 — 다른 Projectile은 Ignore.
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetCollisionObjectType(ECC_GameTraceChannel1);
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComp->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);

	CollisionComp->SetNotifyRigidBodyCollision(true);
	CollisionComp->OnComponentHit.AddDynamic(this, &AProjectileBase::OnHit);
	RootComponent = CollisionComp;

	// Visual mesh (optional)
	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	BulletMesh->SetupAttachment(CollisionComp);
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Tracer (Niagara Component) - 발사 시 활성화
	TracerComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TracerComponent"));
	TracerComponent->SetupAttachment(CollisionComp);
	TracerComponent->bAutoActivate = false;

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

	// Tracer 활성화 — 풀 재사용 시 이전 ribbon strand 잔재 차단
	if (TracerComponent)
	{
		if (CachedWeaponData.BulletTracerFX)
		{
			TracerComponent->DeactivateImmediate();  // 잔류 strand 강제 클리어 (동기)
			TracerComponent->SetAsset(CachedWeaponData.BulletTracerFX);
			TracerComponent->SetVisibility(true);
			TracerComponent->ResetSystem();
			TracerComponent->Activate(true);
		}
		else
		{
			TracerComponent->DeactivateImmediate();
		}
	}

	if (BulletMesh)
	{
		UE_LOG(LogTemp, Log, TEXT("[Bullet] Mesh visible: %s, StaticMesh=%s"),
			BulletMesh->IsVisible() ? TEXT("YES") : TEXT("NO"),
			BulletMesh->GetStaticMesh() ? *BulletMesh->GetStaticMesh()->GetName() : TEXT("NULL"));
	}

	GetWorldTimerManager().ClearTimer(LifeSpanTimerHandle);
	GetWorldTimerManager().SetTimer(LifeSpanTimerHandle, this, &AProjectileBase::Deactivate, LifeSpan, false);

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

	if (TracerComponent)
	{
		TracerComponent->DeactivateImmediate();  // lazy → 동기 정리, 잔류 strand 즉시 제거
		TracerComponent->SetVisibility(false);   // 렌더 즉시 차단
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

		if (HasAuthority())
		{
			UGameplayStatics::ApplyPointDamage(
				HitChar,
				FinalDamage,
				(Hit.ImpactPoint - OwnerCharacter->GetActorLocation()).GetSafeNormal(),
				Hit,
				OwnerController,
				this,
				nullptr);
		}
	}

	// 임팩트 이펙트 브로드캐스트 (모든 클라이언트 동기화) - Deactivate 전에 호출
	// 클라 투사체는 풀 한정으로 CachedWeaponData가 비어있으므로, 서버가 가진 에셋 포인터를 RPC 인자로 동봉.
	MulticastSpawnImpactEffects(
		Hit.ImpactPoint,
		Hit.ImpactNormal,
		OtherComp,
		bHitCharacter,
		CachedWeaponData.BulletImpactFX,
		CachedWeaponData.BulletImpactSound,
		CachedWeaponData.BulletImpactDecal,
		CachedWeaponData.BulletDecalSize,
		CachedWeaponData.BulletDecalLifeSpan,
		CachedWeaponData.BloodImpactFX,
		CachedWeaponData.BloodImpactSound
	);

	UE_LOG(LogTemp, Log, TEXT("[Projectile] Hit: %s, Damage=%.1f, Bone=%s"),
		OtherActor ? *OtherActor->GetName() : TEXT("None"), FinalDamage, *BoneName.ToString());

	Deactivate();
}

void AProjectileBase::SetWeaponData(const FWeaponData& InWeaponData)
{
	CachedWeaponData = InWeaponData;
}

void AProjectileBase::MulticastSpawnImpactEffects_Implementation(
	FVector ImpactLocation,
	FVector ImpactNormal,
	UPrimitiveComponent* HitComp,
	bool bHitCharacter,
	UNiagaraSystem* ImpactFX,
	USoundBase* ImpactSound,
	UMaterialInterface* DecalMaterial,
	FVector DecalSize,
	float DecalLifeSpan,
	UNiagaraSystem* BloodFX,
	USoundBase* BloodSound)
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (bHitCharacter)
	{
		// === 캐릭터 피격: 피 파티클 + 사운드 ===
		if (BloodFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				BloodFX,
				ImpactLocation,
				ImpactNormal.Rotation()
			);
		}

		if (BloodSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				World,
				BloodSound,
				ImpactLocation
			);
		}

		UE_LOG(LogTemp, Log, TEXT("[Impact] Blood effect at %s"), *ImpactLocation.ToString());
	}
	else
	{
		// === 벽/바닥: 파티클 + 데칼 + 사운드 ===
		if (ImpactFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				ImpactFX,
				ImpactLocation,
				ImpactNormal.Rotation()
			);
		}

		// 데칼은 HitComp에 부착 (움직이는 오브젝트 대응)
		if (DecalMaterial && HitComp)
		{
			UDecalComponent* Decal = UGameplayStatics::SpawnDecalAttached(
				DecalMaterial,
				DecalSize,
				HitComp,
				NAME_None,
				ImpactLocation,
				ImpactNormal.Rotation(),
				EAttachLocation::KeepWorldPosition,
				DecalLifeSpan
			);

			if (Decal)
			{
				UE_LOG(LogTemp, Log, TEXT("[Impact] Decal spawned (life: %.1fs)"), DecalLifeSpan);
			}
		}

		if (ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				World,
				ImpactSound,
				ImpactLocation
			);
		}

		UE_LOG(LogTemp, Log, TEXT("[Impact] Wall effects at %s"), *ImpactLocation.ToString());
	}
}
