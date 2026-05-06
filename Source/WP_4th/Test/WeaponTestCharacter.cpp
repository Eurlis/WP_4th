// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponTestCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/DataTable.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WeaponBase.h"
#include "WeaponData.h"
#include "ThrowableBase.h"
#include "TrajectoryHelper.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractableInterface.h"

AWeaponTestCharacter::AWeaponTestCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	FirstPersonCamera->bUsePawnControlRotation = true;

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));
	Mesh1P->SetupAttachment(FirstPersonCamera);
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;

	// === WP4-37: 수류탄 궤적 프리뷰 ===
	TrajectorySpline = CreateDefaultSubobject<USplineComponent>(TEXT("TrajectorySpline"));
	TrajectorySpline->SetupAttachment(GetCapsuleComponent());
	TrajectorySpline->SetMobility(EComponentMobility::Movable);
	TrajectorySpline->ClearSplinePoints(false);

	// === WP4-38: 착탄 마커 ===
	TargetMarkerDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TargetMarkerDecal"));
	TargetMarkerDecal->SetupAttachment(GetCapsuleComponent());
	TargetMarkerDecal->SetVisibility(false);
	TargetMarkerDecal->DecalSize = FVector(50.f, 50.f, 50.f);

	CurrentWeapon = nullptr;

	InteractionComp = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComp"));
}

void AWeaponTestCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// 기본 무기 AR로 시작
	SwitchWeaponByID(ARWeaponID);

	// 카메라 기본 FOV 적용
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetFieldOfView(DefaultFOV);
	}

	// 이동 속도 원본 저장
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		SavedDefaultWalkSpeed = MoveComp->MaxWalkSpeed;
	}

	// 마커 머터리얼 적용 (BP에서 TargetMarkerMaterial 세팅 시)
	if (TargetMarkerDecal && TargetMarkerMaterial)
	{
		TargetMarkerDecal->SetDecalMaterial(TargetMarkerMaterial);
	}
}

void AWeaponTestCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!FirstPersonCamera) return;

	const bool bAiming = CurrentWeapon && CurrentWeapon->bIsAiming && CurrentSlot == EEquippedSlot::Weapon;
	const float Multiplier = bAiming ? CurrentWeapon->GetADSFOVMultiplier() : 1.0f;
	const float TargetFOV = DefaultFOV * Multiplier;

	const float CurrentFOV = FirstPersonCamera->FieldOfView;
	const float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, ADSInterpSpeed);
	FirstPersonCamera->SetFieldOfView(NewFOV);

	if (bIsAimingThrowable && IsCurrentWeaponThrowable())
	{
		UpdateThrowableAimPreview();
	}
}

void AWeaponTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogTemp, Warning, TEXT("[Init] SetupPlayerInputComponent called"));

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Init] EnhancedInputComponent CAST FAILED"));
		return;
	}

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::Move);
	}
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::Look);
	}
	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (FireAction)
	{
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::StartFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &AWeaponTestCharacter::StopFire);
	}
	if (ReloadAction)
	{
		EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::Reload);
	}
	if (SwitchARAction)
	{
		EnhancedInput->BindAction(SwitchARAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::SwitchToAR);
	}
	if (SwitchPistolAction)
	{
		EnhancedInput->BindAction(SwitchPistolAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::SwitchToPistol);
	}
	if (SwitchShotgunAction)
	{
		EnhancedInput->BindAction(SwitchShotgunAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::SwitchToShotgun);
	}
	if (SwitchGrenadeAction)
	{
		EnhancedInput->BindAction(SwitchGrenadeAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::SwitchToGrenade);
	}
	if (TurnAction)
	{
		EnhancedInput->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::Turn);
	}
	if (LookUpAction)
	{
		EnhancedInput->BindAction(LookUpAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::LookUp);
	}
	if (AimAction)
	{
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::OnAimStarted);
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Completed, this, &AWeaponTestCharacter::OnAimStopped);
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Canceled, this, &AWeaponTestCharacter::OnAimStopped);
	}
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::OnInteractInput);
		UE_LOG(LogTemp, Warning, TEXT("[Init] InteractAction BOUND successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Init] InteractAction is NULL - check BP assignment"));
	}
}

void AWeaponTestCharacter::OnInteractInput(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Verbose, TEXT("[Interact] OnInteractInput called! HasAuthority: %d"), HasAuthority());

	if (!InteractionComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interact] InteractionComp NULL"));
		return;
	}

	if (!InteractionComp->CurrentInteractable)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Interact] CurrentInteractable NULL"));
		return;
	}

	UE_LOG(LogTemp, Verbose, TEXT("[Interact] Target: %s"),
		*InteractionComp->CurrentInteractable->GetName());

	AActor* Target = InteractionComp->CurrentInteractable;
	IInteractableInterface* Iface = Cast<IInteractableInterface>(Target);
	if (!Iface)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interact] Cast<IInteractableInterface> FAILED on %s"), *Target->GetName());
		return;
	}
	if (!Iface->CanInteract(this))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interact] CanInteract returned FALSE on %s"), *Target->GetName());
		return;
	}

	if (HasAuthority())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Interact] Calling OnInteract directly (server)"));
		Iface->OnInteract(this);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Interact] Calling ServerInteract RPC (client)"));
		ServerInteract(Target);
	}
}

void AWeaponTestCharacter::ServerInteract_Implementation(AActor* TargetInteractable)
{
	if (!TargetInteractable)
	{
		return;
	}

	// 보안: 클라이언트가 보낸 타깃 액터의 거리 재검증 (조작 방지)
	const float MaxAllowedDist = 500.f;
	if (GetDistanceTo(TargetInteractable) > MaxAllowedDist)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interact] Target too far: %.1f"), GetDistanceTo(TargetInteractable));
		return;
	}

	IInteractableInterface* Iface = Cast<IInteractableInterface>(TargetInteractable);
	if (Iface && Iface->CanInteract(this))
	{
		Iface->OnInteract(this);
	}
}

void AWeaponTestCharacter::OnAimStarted()
{
	if (CurrentSlot != EEquippedSlot::Weapon || !CurrentWeapon) return;

	CurrentWeapon->StartAiming();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (SavedDefaultWalkSpeed <= 0.f)
		{
			SavedDefaultWalkSpeed = MoveComp->MaxWalkSpeed;
		}
		MoveComp->MaxWalkSpeed = SavedDefaultWalkSpeed * ADSWalkSpeedMultiplier;
	}
}

void AWeaponTestCharacter::OnAimStopped()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopAiming();
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (SavedDefaultWalkSpeed > 0.f)
		{
			MoveComp->MaxWalkSpeed = SavedDefaultWalkSpeed;
		}
	}
}

void AWeaponTestCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AWeaponTestCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AWeaponTestCharacter::StartFire()
{
	if (CurrentSlot == EEquippedSlot::Grenade)
	{
		StartThrowableAim();
	}
	else if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void AWeaponTestCharacter::StopFire()
{
	if (CurrentSlot == EEquippedSlot::Grenade && bIsAimingThrowable)
	{
		StopThrowableAim();
		ThrowGrenade();

		if (GrenadeCount <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Slot] No grenades left, switching back to weapon"));
			SwitchWeaponByID(!LastWeaponID.IsNone() ? LastWeaponID : ARWeaponID);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Slot] Grenade thrown, remaining: %d"), GrenadeCount);
		}
		return;
	}

	if (CurrentSlot == EEquippedSlot::Weapon && CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void AWeaponTestCharacter::Reload()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartReload();
	}
}

void AWeaponTestCharacter::Turn(const FInputActionValue& Value)
{
	float TurnValue = Value.Get<float>();
	if (Controller != nullptr)
	{
		AddControllerYawInput(TurnValue);
	}
}

void AWeaponTestCharacter::LookUp(const FInputActionValue& Value)
{
	float LookValue = Value.Get<float>();
	if (Controller != nullptr)
	{
		AddControllerPitchInput(LookValue);
	}
}

void AWeaponTestCharacter::SwitchToAR()
{
	SwitchWeaponByID(ARWeaponID);
}

void AWeaponTestCharacter::SwitchToPistol()
{
	SwitchWeaponByID(PistolWeaponID);
}

void AWeaponTestCharacter::SwitchToShotgun()
{
	SwitchWeaponByID(ShotgunWeaponID);
}

void AWeaponTestCharacter::SwitchToGrenade()
{
	// 이미 Grenade 슬롯이면 재진입 무시 (입력 중복 + 미래 확장 방어)
	if (CurrentSlot == EEquippedSlot::Grenade)
	{
		return;
	}

	if (GrenadeCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Slot] No grenades left!"));
		return;
	}

	// 현재 무기 숨기기 (Destroy 하지 말고 Hidden 처리)
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
		CurrentWeapon->SetActorHiddenInGame(true);
	}

	CurrentSlot = EEquippedSlot::Grenade;
	UE_LOG(LogTemp, Warning, TEXT("[Slot] Switched to Grenade (Count: %d)"), GrenadeCount);
}

void AWeaponTestCharacter::SwitchWeaponByID(FName WeaponID)
{
	if (WeaponID.IsNone() || !GenericWeaponClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TestChar] SwitchWeaponByID: invalid ID or GenericWeaponClass not set"));
		return;
	}

	// 중복 호출 방어: 이미 해당 무기를 장착 중이면 무시
	// (수류탄 슬롯에서 복귀하는 경우는 숨김 해제 위해 재스폰 필요하므로 예외)
	if (CurrentWeapon && CurrentSlot != EEquippedSlot::Grenade && CurrentWeapon->WeaponID == WeaponID)
	{
		return;
	}

	// 수류탄 모드였으면 무기 모드로 복귀
	if (CurrentSlot == EEquippedSlot::Grenade)
	{
		CurrentSlot = EEquippedSlot::Weapon;
		StopThrowableAim();
	}

	// 기존 무기 제거 (숨김 무기 포함)
	if (CurrentWeapon)
	{
		CurrentWeapon->OnUnequipped();
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(GenericWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!CurrentWeapon) return;

	// DataTable에서 데이터 로드
	CurrentWeapon->InitFromDataTable(WeaponID);

	CurrentWeapon->AttachToComponent(
		FirstPersonCamera,
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		NAME_None);

	CurrentWeapon->SetActorRelativeLocation(FVector(30.0f, 15.0f, -10.0f));
	CurrentWeapon->SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	CurrentWeapon->OwningCharacter = this;
	CurrentWeapon->OnEquipped();

	if (CurrentWeapon->WeaponMesh1P)
	{
		CurrentWeapon->WeaponMesh1P->SetVisibility(true);
		CurrentWeapon->WeaponMesh1P->SetOnlyOwnerSee(false);
	}
	if (CurrentWeapon->WeaponMesh3P)
	{
		CurrentWeapon->WeaponMesh3P->SetVisibility(false);
	}

	// 마지막 무기 기억
	LastWeaponID = WeaponID;

	UE_LOG(LogTemp, Warning, TEXT("[TestChar] Weapon switched to: %s"), *WeaponID.ToString());

	// HUD 위젯 등 BP 측 갱신
	if (CurrentWeapon)
	{
		BP_OnWeaponEquipped(CurrentWeapon);
	}
}

void AWeaponTestCharacter::AddGrenade(FName GrenadeID)
{
	if (!HasAuthority()) return;

	if (GrenadeID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grenade] AddGrenade: invalid ID"));
		return;
	}

	if (GrenadeWeaponID != GrenadeID)
	{
		GrenadeWeaponID = GrenadeID;
	}

	GrenadeCount = FMath::Min(GrenadeCount + 1, MaxGrenadeCount);

	UE_LOG(LogTemp, Log, TEXT("[Grenade] AddGrenade: %s, count = %d/%d"),
		*GrenadeID.ToString(), GrenadeCount, MaxGrenadeCount);
}

void AWeaponTestCharacter::ServerApplyDamage(float Damage, ACharacter* DamageInstigator, FHitResult HitResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[TestChar] Damage: %.1f, Bone: %s"), Damage, *HitResult.BoneName.ToString());
}

void AWeaponTestCharacter::ClientShowHitMarker_Implementation(bool bIsHeadshot)
{
	UE_LOG(LogTemp, Warning, TEXT("[TestChar] HitMarker! Headshot: %d"), bIsHeadshot);
}

FVector AWeaponTestCharacter::GetAimDirection() const
{
	return FirstPersonCamera->GetForwardVector();
}

void AWeaponTestCharacter::ThrowGrenade()
{
	if (GrenadeCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grenade] No grenades left!"));
		return;
	}

	if (!GenericThrowableClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grenade] GenericThrowableClass not set!"));
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);

	FVector SpawnLocation = CameraLocation + CameraRotation.Vector() * 100.f;
	FRotator SpawnRotation = CameraRotation;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AThrowableBase* Grenade = GetWorld()->SpawnActor<AThrowableBase>(
		GenericThrowableClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Grenade)
	{
		// DataTable에서 수류탄 데이터 로드
		Grenade->InitFromDataTable(GrenadeWeaponID);

		FVector ThrowDirection = CameraRotation.Vector();
		ThrowDirection.Z += 0.2f;
		ThrowDirection.Normalize();

		Grenade->ServerThrow(ThrowDirection);
		GrenadeCount--;

		UE_LOG(LogTemp, Warning, TEXT("[Grenade] Thrown! Remaining: %d"), GrenadeCount);
	}
}

// ==================== WP4-37/38: 수류탄 조준 시스템 ====================

bool AWeaponTestCharacter::IsCurrentWeaponThrowable() const
{
	return CurrentSlot == EEquippedSlot::Grenade && GrenadeCount > 0;
}

void AWeaponTestCharacter::StartThrowableAim()
{
	bIsAimingThrowable = true;
}

void AWeaponTestCharacter::StopThrowableAim()
{
	bIsAimingThrowable = false;
	TrajectoryHelper::ClearTrajectory(TrajectorySpline, TrajectoryMeshes);
	if (TargetMarkerDecal)
	{
		TargetMarkerDecal->SetVisibility(false);
	}
}

void AWeaponTestCharacter::UpdateThrowableAimPreview()
{
	if (!GenericThrowableClass || !GetController() || !GetWorld())
	{
		return;
	}

	// GenericThrowableClass CDO 의 DataTable 에서 투척 물리 파라미터 조회
	// (실제 ServerThrow 와 동일한 ThrowForce / ThrowableGravityScale 사용)
	AThrowableBase* ThrowableCDO = GenericThrowableClass->GetDefaultObject<AThrowableBase>();
	if (!ThrowableCDO || !ThrowableCDO->WeaponDataTable)
	{
		return;
	}

	FWeaponData* Data = ThrowableCDO->WeaponDataTable->FindRow<FWeaponData>(
		GrenadeWeaponID, TEXT("ThrowableAimPreview"));
	if (!Data)
	{
		return;
	}

	// ThrowGrenade 와 동일한 SpawnLocation / Direction 계산
	FVector CameraLocation;
	FRotator CameraRotation;
	GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const FVector StartLocation = CameraLocation + CameraRotation.Vector() * 100.f;

	FVector ThrowDirection = CameraRotation.Vector();
	ThrowDirection.Z += 0.2f;
	ThrowDirection.Normalize();

	const FVector LaunchVelocity = ThrowDirection * Data->ThrowForce;

	FPredictProjectilePathParams PredictParams(
		TrajectoryProjectileRadius,
		StartLocation,
		LaunchVelocity,
		MaxTrajectorySimTime);

	PredictParams.bTraceWithCollision = true;
	PredictParams.bTraceComplex = false;
	PredictParams.ActorsToIgnore.Add(this);
	PredictParams.SimFrequency = 15.f;
	PredictParams.OverrideGravityZ = -980.f * Data->ThrowableGravityScale;
	PredictParams.TraceChannel = ECC_Visibility;

	FPredictProjectilePathResult Result;
	UGameplayStatics::PredictProjectilePath(this, PredictParams, Result);

	CachedTrajectoryResult = Result;

	TrajectoryHelper::UpdateSplineFromPath(
		TrajectorySpline,
		TrajectoryMeshes,
		Result.PathData,
		TrajectorySplineMesh,
		TrajectoryMeshMaterial,
		this);

	if (TargetMarkerDecal)
	{
		const bool bBlockingHit = Result.HitResult.bBlockingHit;
		const FVector LandingPoint = bBlockingHit
			? Result.HitResult.ImpactPoint
			: Result.LastTraceDestination.Location;
		const FVector LandingNormal = bBlockingHit
			? Result.HitResult.ImpactNormal
			: FVector::UpVector;

		TargetMarkerDecal->SetWorldLocationAndRotation(LandingPoint, LandingNormal.Rotation());
		TargetMarkerDecal->SetVisibility(bBlockingHit);
	}
}
