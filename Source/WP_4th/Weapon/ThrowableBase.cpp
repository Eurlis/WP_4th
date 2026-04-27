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
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

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

	// 부착 1초 뒤 ShockFX/사운드 멀티캐스트 (구체가 보이고 들리는 시간 확보)
	const float StickEffectDelay = 1.0f;
	const FVector StickLoc = PickupMesh
		? PickupMesh->GetComponentLocation()
		: GetActorLocation();
	TWeakObjectPtr<AActor> StuckActorWeak(StuckTarget);

	GetWorldTimerManager().SetTimer(
		StickEffectTimerHandle,
		FTimerDelegate::CreateLambda([this, StickLoc, StuckActorWeak]()
		{
			if (!IsValid(this)) return;
			MulticastStickEffects(StickLoc, StuckActorWeak.Get());
		}),
		StickEffectDelay,
		false
	);

	// 부착 이후 퓨즈 시작 (ServerThrow에서 시작 안 함)
	GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AThrowableBase::Explode, FuseTime, false);

	UE_LOG(LogTemp, Warning, TEXT("[ArcStar] Fuse started: %.2fs"), FuseTime);
}

void AThrowableBase::Explode()
{
	if (!HasAuthority()) return;

	FVector ExplosionLocation = GetActorLocation();

	// 던진 방향 추출 (ProjectileMovement->Velocity 수평 성분, 양 분기 공통)
	FVector ThrowDir = FVector::ZeroVector;
	if (ProjectileMovement)
	{
		FVector HorizontalVel = ProjectileMovement->Velocity;
		HorizontalVel.Z = 0.0f;
		if (!HorizontalVel.IsNearlyZero())
		{
			ThrowDir = HorizontalVel.GetSafeNormal();
		}
	}

	// === 소이탄 (bIsIncendiary=true, Thermite 등): FireZone 생성 ===
	if (CurrentWeaponData.bIsIncendiary)
	{
		// FireZoneClass 결정 (DataTable 값 우선, 없으면 기본 AFireZone)
		TSubclassOf<AFireZone> ClassToSpawn = AFireZone::StaticClass();
		if (CurrentWeaponData.FireZoneClass)
		{
			ClassToSpawn = CurrentWeaponData.FireZoneClass;
		}

		// 투척 방향 → FireZone 회전 (수평 박스 정렬)
		FRotator SpawnRotation = ThrowDir.IsNearlyZero()
			? FRotator::ZeroRotator
			: ThrowDir.Rotation();

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
				CurrentWeaponData.FireZoneDamagePerTick,	// DamagePerTick (DT, 별도 필드)
				CurrentWeaponData.FireZoneExtent,			// BoxExtent (DT, 직접 지정)
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

		MulticastExplosionEffects(ExplosionLocation, ThrowDir);
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

	MulticastExplosionEffects(ExplosionLocation, ThrowDir);

	// Arc Star가 캐릭터에 붙어있던 경우, Destroy 전에 Detach (캐릭터 변형 방지)
	if (bIsStuck)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	Destroy();
}

void AThrowableBase::MulticastExplosionEffects_Implementation(FVector ExplosionLocation, FVector ThrowDir)
{
	UWorld* World = GetWorld();

	// 1. ExplosionFX (Niagara) — 모든 수류탄 공통 폭발 이펙트
	if (CurrentWeaponData.ExplosionFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			CurrentWeaponData.ExplosionFX,
			ExplosionLocation,
			FRotator::ZeroRotator,
			FVector(1.0f),
			true,   // bAutoDestroy
			true    // bAutoActivate
		);
	}

	// 2. ExplosionSound — ArcStar(bIsSticky)는 부착 시점에 이미 재생됨, 폭발 시점 스킵
	if (CurrentWeaponData.ExplosionSound && !CurrentWeaponData.bIsSticky)
	{
		UGameplayStatics::PlaySoundAtLocation(
			World,
			CurrentWeaponData.ExplosionSound,
			ExplosionLocation
		);
	}

	// 3. FireFX — Thermite 소이 화염 (bIsIncendiary=true 전용)
	// 던진 방향에 수직으로 라인 N개 스폰 (Apex 화염 라인 효과)
	if (CurrentWeaponData.bIsIncendiary && CurrentWeaponData.FireFX)
	{
		const FVector ThrowDir2D = ThrowDir.IsNearlyZero()
			? FVector::ForwardVector
			: ThrowDir.GetSafeNormal2D();
		// 던진 방향과 수직 (90도 회전): (X,Y) → (-Y,X)
		const FVector PerpDir = FVector(-ThrowDir2D.Y, ThrowDir2D.X, 0.0f);
		const FRotator FlameRot = PerpDir.Rotation();

		constexpr int32 FlameCount = 7;
		constexpr float FlameSpacing = 100.0f;	// 100cm 간격
		const int32 Half = FlameCount / 2;

		for (int32 i = -Half; i <= Half; i++)
		{
			FVector FlameLocation = ExplosionLocation + PerpDir * (i * FlameSpacing);
			FlameLocation.Z += 50.0f;	// 데칼이 바닥에 투영되도록 50cm 띄움
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				CurrentWeaponData.FireFX,
				FlameLocation,
				FlameRot,
				FVector(1.0f),
				true,	// bAutoDestroy
				true	// bAutoActivate
			);
		}
	}

	// 4. ShockFX 정리 — 부착 시점에 스폰된 감전 이펙트 종료
	if (ActiveShockFXComponent)
	{
		ActiveShockFXComponent->Deactivate();
		ActiveShockFXComponent = nullptr;
	}
}

void AThrowableBase::MulticastStickEffects_Implementation(FVector StickLocation, AActor* StuckActor)
{
	if (!CurrentWeaponData.bIsSticky)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// ShockFX (전기 구체) — StuckActor 무관하게 World Location에 고정 스폰
	// 표창 메시는 곧 숨겨지고, ShockFX는 그 위치에 그대로 머무름
	if (CurrentWeaponData.ShockFX)
	{
		ActiveShockFXComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			CurrentWeaponData.ShockFX,
			StickLocation,
			FRotator::ZeroRotator,
			FVector(1.0f),
			false,  // bAutoDestroy = false (Explode에서 수동 정리)
			true    // bAutoActivate
		);
	}

	// ArcStar 부착 사운드 (ExplosionSound 재사용 — 폭발 시점에는 스킵됨)
	if (CurrentWeaponData.bIsSticky && CurrentWeaponData.ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			World,
			CurrentWeaponData.ExplosionSound,
			StickLocation
		);
	}

	// 표창 메시 숨김 (구체 등장과 동시에) — 모든 클라이언트에 반영
	if (PickupMesh)
	{
		PickupMesh->SetVisibility(false);
	}
}

void AThrowableBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FuseTimerHandle);
	GetWorldTimerManager().ClearTimer(MaxLifetimeHandle);
	GetWorldTimerManager().ClearTimer(StickEffectTimerHandle);

	if (ActiveShockFXComponent)
	{
		ActiveShockFXComponent->DestroyComponent();
		ActiveShockFXComponent = nullptr;
	}

	Super::EndPlay(EndPlayReason);
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
