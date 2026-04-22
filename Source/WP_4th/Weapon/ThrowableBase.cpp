// Fill out your copyright notice in the Description page of Project Settings.

#include "ThrowableBase.h"
#include "FireZone.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Engine/DataTable.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"

AThrowableBase::AThrowableBase()
{
	PrimaryActorTick.bCanEverTick = true; // Arc Star 시각적 회전용
	bReplicates = true;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = PickupMesh;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bAutoActivate = false;

	ThrowForce = 2000.f;
	FuseTime = 2.0f;
	ExplosionDamage = 80.f;
	ExplosionRadius = 500.f; // 5m = 500 UU (1m ≈ 100 UU)
}

void AThrowableBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AThrowableBase, WeaponID);
	DOREPLIFETIME(AThrowableBase, bIsStuck);
}

void AThrowableBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && !WeaponID.IsNone() && WeaponDataTable)
	{
		InitFromDataTable(WeaponID);
	}

	// Arc Star만 OnComponentHit 바인딩 (bIsSticky 체크는 콜백 안에서도 재확인)
	if (CurrentWeaponData.bIsSticky && PickupMesh)
	{
		PickupMesh->OnComponentHit.AddDynamic(this, &AThrowableBase::OnProjectileHit);
		UE_LOG(LogTemp, Warning, TEXT("[ArcStar] OnComponentHit bound"));
	}
}

void AThrowableBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Arc Star 표창 회전 (부착 전까지) - Pitch축 (앞구르기)
	if (bIsVisualSpinning && !bIsStuck && PickupMesh)
	{
		PickupMesh->AddLocalRotation(FRotator(1500.0f * DeltaTime, 0.0f, 0.0f));
	}
}

// ==================== Data ====================

void AThrowableBase::InitFromDataTable(FName InWeaponID)
{
	if (!WeaponDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[ThrowableBase] WeaponDataTable not set!"));
		return;
	}

	FWeaponData* Data = WeaponDataTable->FindRow<FWeaponData>(InWeaponID, TEXT("ThrowableBase InitFromDataTable"));
	if (!Data)
	{
		UE_LOG(LogTemp, Error, TEXT("[ThrowableBase] WeaponID '%s' not found!"), *InWeaponID.ToString());
		return;
	}

	WeaponID = InWeaponID;
	CurrentWeaponData = *Data;
	ApplyThrowableData(*Data);

	UE_LOG(LogTemp, Warning, TEXT("[ThrowableBase] Init: %s (Dmg:%.1f Radius:%.1f Fuse:%.2f)"),
		*Data->DisplayName.ToString(), Data->ExplosionDamage, Data->ExplosionRadius, Data->FuseTime);
}

void AThrowableBase::ApplyThrowableData(const FWeaponData& Data)
{
	ThrowForce = Data.ThrowForce;
	FuseTime = Data.FuseTime;
	ExplosionDamage = Data.ExplosionDamage;
	ExplosionRadius = Data.ExplosionRadius;

	if (Data.ThrowableMesh && PickupMesh)
	{
		PickupMesh->SetStaticMesh(Data.ThrowableMesh);
		PickupMesh->SetRelativeScale3D(Data.ThrowableMeshScale);
		PickupMesh->SetRelativeRotation(Data.ThrowableMeshRotation);
	}
}

void AThrowableBase::OnRep_WeaponID()
{
	if (!WeaponID.IsNone())
	{
		InitFromDataTable(WeaponID);
	}
}

bool AThrowableBase::ServerThrow_Validate(FVector ThrowDirection)
{
	return true;
}

void AThrowableBase::ServerThrow_Implementation(FVector ThrowDirection)
{
	// === 진단 로그 ===
	UE_LOG(LogTemp, Warning, TEXT("=== ServerThrow: %s ==="), *WeaponID.ToString());
	UE_LOG(LogTemp, Warning, TEXT("  bIsIncendiary=%s  bIsSticky=%s"),
	       CurrentWeaponData.bIsIncendiary ? TEXT("YES") : TEXT("NO"),
	       CurrentWeaponData.bIsSticky ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("  FuseTime=%.2f  ExplosionDmg=%.1f  ThrowForce=%.1f"),
	       FuseTime, ExplosionDamage, ThrowForce);
	UE_LOG(LogTemp, Warning, TEXT("  PickupMesh Profile=%s  Enabled=%d  HasMesh=%s"),
	       *PickupMesh->GetCollisionProfileName().ToString(),
	       (int32)PickupMesh->GetCollisionEnabled(),
	       PickupMesh->GetStaticMesh() ? TEXT("YES") : TEXT("NO"));

	// Detach from owner
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetItemState(EItemState::Dropped);
	PickupMesh->SetVisibility(true);
	PickupMesh->SetSimulatePhysics(false);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// === 모든 수류탄 공통: Block 프로파일 (벽 통과 방지) ===
	// PickupMesh 기본은 NoCollision (ItemBase 설정). 던진 후엔 Sweep 을 위해 Block 필요.
	PickupMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PickupMesh->SetNotifyRigidBodyCollision(true);

	// 시전자 셀프 충돌 원천 차단 (모든 타입)
	if (AActor* OwnerActor = GetOwner())
	{
		PickupMesh->IgnoreActorWhenMoving(OwnerActor, true);
	}

	// === Physics 파라미터 DataTable 주입 (Apex 스타일 궤적) ===
	if (ProjectileMovement)
	{
		ProjectileMovement->ProjectileGravityScale = CurrentWeaponData.ThrowableGravityScale;
		ProjectileMovement->Bounciness = CurrentWeaponData.ThrowableBounciness;
	}
	UE_LOG(LogTemp, Warning, TEXT("[Throw] %s - Gravity:%.2f Bounciness:%.2f"),
	       *WeaponID.ToString(),
	       CurrentWeaponData.ThrowableGravityScale,
	       CurrentWeaponData.ThrowableBounciness);

	// === Arc Star 전용 설정 (bIsSticky=true) ===
	if (CurrentWeaponData.bIsSticky)
	{
		// 모든 채널 Block 응답 강화 (이미 BlockAllDynamic 이지만 명시)
		PickupMesh->SetCollisionResponseToAllChannels(ECR_Block);

		// ProjectileMovement: 바운스/마찰 없음 (박히는 느낌)
		ProjectileMovement->bShouldBounce = false;
		ProjectileMovement->Friction = 0.0f;
		ProjectileMovement->Bounciness = 0.0f;

		// OnProjectileStop 바인딩 (OnComponentHit 놓친 경우 보조)
		if (!ProjectileMovement->OnProjectileStop.IsAlreadyBound(this, &AThrowableBase::OnProjectileStopped))
		{
			ProjectileMovement->OnProjectileStop.AddDynamic(this, &AThrowableBase::OnProjectileStopped);
		}

		bIsVisualSpinning = true;

		// 최후의 보루: 10초 후 강제 폭발 (공중 정지 방지)
		GetWorldTimerManager().SetTimer(MaxLifetimeHandle, this, &AThrowableBase::ForceExplode, 10.0f, false);

		UE_LOG(LogTemp, Warning, TEXT("[ArcStar] Thrown with sticky mode"));
	}

	// === Thermite 전용 설정 (bIsIncendiary=true): 충돌 즉시 폭발 ===
	if (CurrentWeaponData.bIsIncendiary)
	{
		PickupMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
		PickupMesh->SetNotifyRigidBodyCollision(true);
		ProjectileMovement->bShouldBounce = false;

		// Owner 물리 충돌 무시 (던진 직후 본인 캡슐과 충돌 방지)
		if (GetOwner())
		{
			PickupMesh->IgnoreActorWhenMoving(GetOwner(), true);
		}

		if (!PickupMesh->OnComponentHit.IsAlreadyBound(this, &AThrowableBase::OnImpactExplode))
		{
			PickupMesh->OnComponentHit.AddDynamic(this, &AThrowableBase::OnImpactExplode);
		}

		UE_LOG(LogTemp, Warning, TEXT("[Incendiary] Thrown with impact-explode mode"));
	}

	// === FragGrenade (bIsSticky=false, bIsIncendiary=false): 바운스 유지 ===
	// 공통 BlockAllDynamic 으로 벽은 감지, ProjectileMovement 기본값(bShouldBounce=true)으로 튕김

	// Activate projectile movement
	FVector LaunchVelocity = ThrowDirection.GetSafeNormal() * ThrowForce;
	ProjectileMovement->Velocity = LaunchVelocity;
	ProjectileMovement->Activate();

	// === 던지기 사운드 (Multicast) ===
	MulticastPlayThrowSound();

	// === 퓨즈 시작 분기 ===
	if (!CurrentWeaponData.bIsSticky && !CurrentWeaponData.bIsIncendiary)
	{
		// FragGrenade: 즉시 퓨즈 시작
		GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AThrowableBase::Explode, FuseTime, false);
	}
	// Arc Star: 부착 후 StickToTarget에서 퓨즈 시작
	// Thermite: 퓨즈 없음, OnImpactExplode에서 즉시 Explode
}

void AThrowableBase::OnProjectileStopped(const FHitResult& ImpactResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[ArcStar] OnProjectileStop triggered at %s"),
	       *ImpactResult.ImpactPoint.ToString());

	if (!HasAuthority()) return;
	if (bIsStuck) return;
	if (!CurrentWeaponData.bIsSticky) return;

	AActor* HitActor = ImpactResult.GetActor();

	// 자기 자신이나 시전자 무시 (시전자 셀프 부착 방지)
	if (HitActor == this)
	{
		UE_LOG(LogTemp, Log, TEXT("[ArcStar] Self-collision ignored"));
		return;
	}
	if (HitActor && (HitActor == OwningCharacter || HitActor == GetOwner()))
	{
		UE_LOG(LogTemp, Log, TEXT("[ArcStar] Owner collision ignored: %s"), *HitActor->GetName());
		return;
	}

	if (ImpactResult.bBlockingHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArcStar] Stopping -> sticking to: %s"),
		       HitActor ? *HitActor->GetName() : TEXT("WALL"));
		StickToTarget(ImpactResult);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArcStar] Stopped but no blocking hit - force stick at current location"));

		FHitResult DummyHit;
		DummyHit.ImpactPoint = GetActorLocation();
		DummyHit.ImpactNormal = FVector(0, 0, 1);
		StickToTarget(DummyHit);
	}
}

void AThrowableBase::ForceExplode()
{
	UE_LOG(LogTemp, Warning, TEXT("[ArcStar] Force explode (max lifetime reached)"));
	Explode();
}

void AThrowableBase::OnImpactExplode(UPrimitiveComponent* HitComp, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp,
                                     FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) return;
	if (OtherActor == this) return;

	// 시전자 자신과의 충돌은 무시 (던진 직후 본인 캡슐 충돌로 즉시 터지는 것 방지)
	if (OtherActor == GetOwner())
	{
		return;
	}

	if (!CurrentWeaponData.bIsIncendiary) return;

	UE_LOG(LogTemp, Warning,
	       TEXT("[Incendiary] Impact: %s at %s -> Explode"),
	       OtherActor ? *OtherActor->GetName() : TEXT("WALL"),
	       *Hit.ImpactPoint.ToString());

	// 충돌 지점에서 FireZone이 스폰되도록 위치 이동
	SetActorLocation(Hit.ImpactPoint);

	// 중복 트리거 방지: Hit delegate 언바인딩
	PickupMesh->OnComponentHit.RemoveDynamic(this, &AThrowableBase::OnImpactExplode);

	Explode();
}

void AThrowableBase::OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
                                     UPrimitiveComponent* OtherComp,
                                     FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) return;
	if (bIsStuck) return;
	if (!CurrentWeaponData.bIsSticky) return;
	if (OtherActor == this) return;
	if (OtherActor == OwningCharacter) return;			// ItemBase.OwningCharacter
	if (OtherActor == GetOwner()) return;				// SpawnParams.Owner 기반 시전자

	UE_LOG(LogTemp, Warning, TEXT("[ArcStar] Hit: %s at %s"),
	       OtherActor ? *OtherActor->GetName() : TEXT("WALL"),
	       *Hit.ImpactPoint.ToString());

	StickToTarget(Hit);
}

void AThrowableBase::StickToTarget(const FHitResult& Hit)
{
	bIsStuck = true;
	bIsVisualSpinning = false;
	StuckTarget = Hit.GetActor();

	// 최후 보루 타이머 해제 (정상 부착됐으니 필요 없음)
	GetWorldTimerManager().ClearTimer(MaxLifetimeHandle);

	UE_LOG(LogTemp, Warning, TEXT("[ArcStar] Stuck to %s"),
	       StuckTarget ? *StuckTarget->GetName() : TEXT("WALL"));

	// ProjectileMovement 완전 정지
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	// 충돌 비활성화 (부착 후 추가 충돌 방지)
	PickupMesh->SetCollisionProfileName(TEXT("NoCollision"));
	PickupMesh->SetNotifyRigidBodyCollision(false);

	// 위치 고정 (충돌 지점)
	SetActorLocation(Hit.ImpactPoint);

	// 캐릭터에 부착된 경우 → AttachTo + 즉발 스티키 데미지
	if (StuckTarget && StuckTarget->IsA<ACharacter>())
	{
		AttachToActor(StuckTarget, FAttachmentTransformRules::KeepWorldTransform);

		AController* InstigatorController =
			OwningCharacter ? OwningCharacter->GetInstigatorController() : nullptr;

		UGameplayStatics::ApplyDamage(
			StuckTarget,
			CurrentWeaponData.StickyDamage,
			InstigatorController,
			OwningCharacter,
			nullptr
		);

		UE_LOG(LogTemp, Warning,
		       TEXT("[ArcStar] Sticky damage %.1f to character %s"),
		       CurrentWeaponData.StickyDamage,
		       *StuckTarget->GetName());
	}

	// 부착 이후 퓨즈 시작 (ServerThrow에서 시작 안 함)
	GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AThrowableBase::Explode, FuseTime, false);

	UE_LOG(LogTemp, Warning, TEXT("[ArcStar] Fuse started: %.2fs"), FuseTime);
}

void AThrowableBase::Explode()
{
	if (!HasAuthority()) return;

	FVector ExplosionLocation = GetActorLocation();

	// === 소이탄 (bIsIncendiary=true, Thermite 등): FireZone 생성 ===
	if (CurrentWeaponData.bIsIncendiary)
	{
		// FireZoneClass 결정 (DataTable 값 우선, 없으면 기본 AFireZone)
		TSubclassOf<AFireZone> ClassToSpawn = AFireZone::StaticClass();
		if (CurrentWeaponData.FireZoneClass)
		{
			ClassToSpawn = CurrentWeaponData.FireZoneClass;
		}

		// 투척 방향 계산 (Velocity 수평 성분 → FireZone 회전)
		FRotator SpawnRotation = FRotator::ZeroRotator;
		if (ProjectileMovement)
		{
			FVector HorizontalVel = ProjectileMovement->Velocity;
			HorizontalVel.Z = 0.0f;
			if (!HorizontalVel.IsNearlyZero())
			{
				SpawnRotation = HorizontalVel.Rotation();
			}
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwningCharacter;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AFireZone* FireZone = GetWorld()->SpawnActor<AFireZone>(
			ClassToSpawn,
			ExplosionLocation,
			SpawnRotation,								// 투척 방향으로 수평 확장
			SpawnParams
		);

		if (FireZone)
		{
			FireZone->InitializeFireZone(
				CurrentWeaponData.FireZoneDuration,			// Duration (DT)
				CurrentWeaponData.FireZoneTickInterval,		// TickInterval (DT)
				CurrentWeaponData.ExplosionDamage,			// DamagePerTick (DT)
				CurrentWeaponData.ExplosionRadius,			// Radius (DT)
				OwningCharacter								// Instigator (시전자도 피해)
			);

			UE_LOG(LogTemp, Warning,
			       TEXT("[Thermite] FireZone spawned at %s, rotation=%s (class: %s)"),
			       *ExplosionLocation.ToString(),
			       *SpawnRotation.ToString(),
			       *ClassToSpawn->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[Incendiary] Failed to spawn FireZone!"));
		}

		MulticastExplosionEffects(ExplosionLocation);
		Destroy();
		return;
	}

	// === 기존 폭발 로직 (FragGrenade, ArcStar 등) ===
	TArray<AActor*> IgnoredActors;
	UGameplayStatics::ApplyRadialDamageWithFalloff(
		GetWorld(),
		ExplosionDamage,								// BaseDamage
		ExplosionDamage * 0.1f,							// MinimumDamage
		ExplosionLocation,								// Origin
		ExplosionRadius * 0.3f,							// DamageInnerRadius
		ExplosionRadius,								// DamageOuterRadius
		1.f,											// DamageFalloff
		nullptr,										// DamageTypeClass
		IgnoredActors,									// IgnoreActors
		this,											// DamageCauser
		OwningCharacter ? OwningCharacter->GetInstigatorController() : nullptr	// InstigatedBy
	);

	MulticastExplosionEffects(ExplosionLocation);

	// Debug sphere
	DrawDebugSphere(GetWorld(), ExplosionLocation, ExplosionRadius, 16, FColor::Yellow, false, 2.0f);

	// Arc Star가 캐릭터에 붙어있던 경우, Destroy 전에 Detach (캐릭터 변형 방지)
	if (bIsStuck)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	Destroy();
}

void AThrowableBase::MulticastExplosionEffects_Implementation(FVector ExplosionLocation)
{
	if (CurrentWeaponData.ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			CurrentWeaponData.ExplosionSound,
			ExplosionLocation
		);
	}

	// TODO: Explosion particle
	DrawDebugSphere(GetWorld(), ExplosionLocation, ExplosionRadius, 16, FColor::Yellow, false, 2.0f);
}

void AThrowableBase::MulticastPlayThrowSound_Implementation()
{
	if (CurrentWeaponData.ThrowSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			CurrentWeaponData.ThrowSound,
			GetActorLocation()
		);
	}
}
