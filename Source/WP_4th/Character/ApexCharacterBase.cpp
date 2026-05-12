#include "ApexCharacterBase.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Character/Components/HPComp/HealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "WeaponBase.h"
#include "ThrowableBase.h"
#include "WeaponData.h"
#include "Pickup/PickupBase.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractableInterface.h"
#include "Net/UnrealNetwork.h"
#include "OJJ_GameMode/ApexDeathmatchGameMode.h"
#include "WP_4th.h"

AApexCharacterBase::AApexCharacterBase()
{
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(15.748037,12.499209,-0.000011), FRotator(0.000000,71.172832,0.000000));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->BrakingDecelerationFalling = 1500.0f;
	MovementComponent->AirControl = 0.5f;
	MovementComponent->NavAgentProps.bCanCrouch = true;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	PakComp = CreateDefaultSubobject<UPakousComponent>(TEXT("PakComp"));
	bIsSprinting = false;
	bIsSliding = false;
	SlideAnimationPhase = ESlideAnimationPhase::None;

	SprintSpeed = 700.f;
	WalkSpeed = 400.f;
	CrouchSpeed = 200.f;
	SlideMaxSpeed = 900.f;
	SlideMinSpeed = 250.f;
	SlopeAccelMultiplier = 0.f;
	SlideJumpSpeedMultiplier = 1.25f;
	SlideEnterDuration = 0.16f;
	SlideExitDuration = 0.20f;
	SlideFlatDeceleration = 200.f;    // 낮춰서 17° 이상 경사면 가속 체감
	SlideUphillDeceleration = 1650.f;
	SlideDownhillAcceleration = 900.f;
	SlideUngroundedGracePeriod = 0.15f;

	SlideDirection = FVector::ForwardVector;
	SlideSpeed = 0.f;
	SlideEnterEndTime = 0.f;
	SlideExitEndTime = 0.f;
	SlideUngroundedTime = 0.f;
	DefaultGroundFriction = MovementComponent->GroundFriction;
	DefaultBrakingDecelerationWalking = MovementComponent->BrakingDecelerationWalking;
	DefaultMaxWalkSpeedCrouched = MovementComponent->MaxWalkSpeedCrouched;

	MotionWarpingComp = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	ZiplineComp = CreateDefaultSubobject<UZiplineRiderComponent>(TEXT("ZiplineComp"));
	InteractionComp = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComp"));


	//Weapon Setting
	ConstructorHelpers::FClassFinder<AWeaponBase> BP_Generic (TEXT("/Game/OJJ/BP/BP_Weapon_Generic.BP_Weapon_Generic_C"));
	if (BP_Generic.Succeeded()) GenericWeaponClass = BP_Generic.Class;

	// BP_Wraith/Octane Default 누락 시 안전망 폴백
	ConstructorHelpers::FClassFinder<AThrowableBase> BP_Throwable(TEXT("/Game/OJJ/BP/BP_Throwable_Generic.BP_Throwable_Generic_C"));
	if (BP_Throwable.Succeeded()) GenericThrowableClass = BP_Throwable.Class;

	ConstructorHelpers::FClassFinder<APickupBase> BP_Pickup(TEXT("/Game/OJJ/BP/BP_PickupBase.BP_PickupBase_C"));
	if (BP_Pickup.Succeeded()) PickupClass = BP_Pickup.Class;
	//Input Setting
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Jump(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	if (IA_Jump.Succeeded()) JumpAction = IA_Jump.Object;

	// MoveAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Move(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	if (IA_Move.Succeeded()) MoveAction = IA_Move.Object;

	// LookAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Look(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	if (IA_Look.Succeeded()) LookAction = IA_Look.Object;

	// MouseLookAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_MouseLook(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
	if (IA_MouseLook.Succeeded()) MouseLookAction = IA_MouseLook.Object;

	// SprintAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Sprint(TEXT("/Game/Input/Actions/IA_Sprint.IA_Sprint"));
	if (IA_Sprint.Succeeded()) SprintAction = IA_Sprint.Object;

	// CrouchAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Crouch(TEXT("/Game/Input/Actions/IA_Crouch.IA_Crouch"));
	if (IA_Crouch.Succeeded()) CrouchAction = IA_Crouch.Object;

	// SlideAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Slide(TEXT("/Game/Input/Actions/IA_Slide.IA_Slide"));
	if (IA_Slide.Succeeded()) SlideAction = IA_Slide.Object;

	// FireAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Fire(TEXT("/Game/OJJ/Inputs/IA_Fire.IA_Fire"));
	if (IA_Fire.Succeeded()) FireAction = IA_Fire.Object;

	// ReloadAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Reload(TEXT("/Game/OJJ/Inputs/IA_Reload.IA_Reload"));
	if (IA_Reload.Succeeded()) ReloadAction = IA_Reload.Object;
	// AimAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Aim(TEXT("/Game/OJJ/Inputs/IA_Aim.IA_Aim"));
	if (IA_Aim.Succeeded()) AimAction = IA_Aim.Object;

	// SwitchARAction (파일명 오타: IA_SwichAR)
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SwitchAR(TEXT("/Game/OJJ/Inputs/IA_SwichAR.IA_SwichAR"));
	if (IA_SwitchAR.Succeeded()) SwitchARAction = IA_SwitchAR.Object;

	// SwitchPistolAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SwitchPistol(TEXT("/Game/OJJ/Inputs/IA_SwitchPistol.IA_SwitchPistol"));
	if (IA_SwitchPistol.Succeeded()) SwitchPistolAction = IA_SwitchPistol.Object;

	// SwitchShotgunAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SwitchShotgun(TEXT("/Game/OJJ/Inputs/IA_SwitchShotgun.IA_SwitchShotgun"));
	if (IA_SwitchShotgun.Succeeded()) SwitchShotgunAction = IA_SwitchShotgun.Object;

	// SwitchGrenadeAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_SwitchGrenade(TEXT("/Game/OJJ/Inputs/IA_SwitchGrenade.IA_SwitchGrenade"));
	if (IA_SwitchGrenade.Succeeded()) SwitchGrenadeAction = IA_SwitchGrenade.Object;

	// InteractAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Interact(TEXT("/Game/OJJ/Inputs/IA_Interact.IA_Interact"));
	if (IA_Interact.Succeeded()) InteractAction = IA_Interact.Object;

	// DropAction
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Drop(TEXT("/Game/OJJ/Inputs/IA_DropWeapon.IA_DropWeapon"));
	if (IA_Drop.Succeeded()) DropAction = IA_Drop.Object;
}

void AApexCharacterBase::OnRep_CurrentWeapon()
{
	// 이전 무기 정리: 서버 Destroy 도달 전이거나 다른 무기로 바뀐 경우 detach (회귀 #5 ghost 차단)
	if (IsValid(PreviousWeapon) && PreviousWeapon != CurrentWeapon)
	{
		PreviousWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	if (CurrentWeapon)
	{
		CurrentWeapon->OwningCharacter = this;
		CurrentWeapon->AttachToComponent(
			FirstPersonMesh,
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			FName("weapon_r"));
		CurrentWeapon->OnEquipped();
	}

	// nullptr 케이스도 BP에 전달 — BP 측에서 HUD 클리어/숨김 처리 (회귀 #5 방지)
	BP_OnWeaponEquipped(CurrentWeapon);

	PreviousWeapon = CurrentWeapon;
}

void AApexCharacterBase::OnRep_CurrentSlot()
{
	// Grenade 슬롯 진입: 현재 무기 메시 숨김 (서버 SwitchToSlot_Internal과 동일 시각 효과)
	if (CurrentSlot == EEquippedSlot::Grenade && PreviousSlot != EEquippedSlot::Grenade)
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->SetActorHiddenInGame(true);
		}
	}
	// Grenade 슬롯 이탈: 미리보기 정리. CurrentWeapon 복귀/생성은 SwitchWeaponByID + OnRep_CurrentWeapon 경로가 처리.
	else if (CurrentSlot != EEquippedSlot::Grenade && PreviousSlot == EEquippedSlot::Grenade)
	{
		if (bIsAimingThrowable)
		{
			StopThrowableAim();
		}
	}

	PreviousSlot = CurrentSlot;
}

void AApexCharacterBase::EquipWeapon(FName WeaponID)
{
	SwitchWeaponByID(WeaponID);
}

void AApexCharacterBase::SwitchWeaponByID(FName WeaponID)
{
	if (!HasAuthority()) return;
	if (WeaponID.IsNone() || !GenericWeaponClass) return;

	// 중복 호출 방어 (수류탄 슬롯 복귀 시는 예외)
	if (CurrentWeapon && CurrentSlot != EEquippedSlot::Grenade && CurrentWeapon->WeaponID == WeaponID)
		return;

	if (CurrentSlot == EEquippedSlot::Grenade)
	{
		CurrentSlot = EEquippedSlot::Weapon;
		StopThrowableAim();
	}

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

	CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(
		GenericWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!CurrentWeapon) return;

	CurrentWeapon->InitFromDataTable(WeaponID);

	CurrentWeapon->AttachToComponent(
		FirstPersonMesh,
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		FName("weapon_r"));

	CurrentWeapon->OwningCharacter = this;
	CurrentWeapon->OnEquipped();

	LastWeaponID = WeaponID;

	BP_OnWeaponEquipped(CurrentWeapon);
}

void AApexCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickSlide(DeltaTime);

	if (IsPlayerControlled() && FirstPersonCameraComponent)
	{
		const bool bAiming = CurrentWeapon && CurrentWeapon->bIsAiming && CurrentSlot == EEquippedSlot::Weapon;
		const float TargetFOV = DefaultFOV * (bAiming ? CurrentWeapon->GetADSFOVMultiplier() : 1.f);
		FirstPersonCameraComponent->SetFieldOfView(
			FMath::FInterpTo(FirstPersonCameraComponent->FieldOfView, TargetFOV, DeltaTime, ADSInterpSpeed));
	}
}

void AApexCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->MaxWalkSpeed = WalkSpeed;
	DefaultGroundFriction = MovementComponent->GroundFriction;
	DefaultBrakingDecelerationWalking = MovementComponent->BrakingDecelerationWalking;
	DefaultFirstPersonMeshLocation = FirstPersonMesh->GetRelativeLocation();

	if (HasAuthority() && IsValid(HealthComponent))
	{
		HealthComponent->OnDeath.AddDynamic(this, &AApexCharacterBase::HandleDeath);
	}

	if (HasAuthority() && WeaponSlots.Num() == 0)
	{
		WeaponSlots.SetNum(4);
		for (int32 i = 0; i < 4; ++i) { WeaponSlots[i] = NAME_None; }
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (SavedDefaultWalkSpeed <= 0.f)
			SavedDefaultWalkSpeed = MoveComp->MaxWalkSpeed;
	}
}

void AApexCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AApexCharacterBase::DoJumpStart);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AApexCharacterBase::DoJumpEnd);
		}

		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AApexCharacterBase::MoveInput);
		}

		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AApexCharacterBase::LookInput);
		}

		if (MouseLookAction)
		{
			EIC->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AApexCharacterBase::LookInput);
		}

		if (SprintAction)
		{
			EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AApexCharacterBase::StartSprint);
			EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AApexCharacterBase::StopSprint);
		}

		if (CrouchAction)
		{
			EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AApexCharacterBase::StartCrouch);
			EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AApexCharacterBase::StopCrouch);
		}

		if (SlideAction)
		{
			EIC->BindAction(SlideAction, ETriggerEvent::Started, this, &AApexCharacterBase::StartSlide);
			EIC->BindAction(SlideAction, ETriggerEvent::Completed, this, &AApexCharacterBase::StopSlide);
		}
		if (FireAction)
		{
			EIC->BindAction(FireAction, ETriggerEvent::Started, this, &AApexCharacterBase::StartFire);
			EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AApexCharacterBase::StopFire);
		}
		if (ReloadAction)
		{
			EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &AApexCharacterBase::Reload);
		}
		if (AimAction)
		{
			EIC->BindAction(AimAction, ETriggerEvent::Started, this, &AApexCharacterBase::OnAimStarted);
		}
		if (InteractAction)
		{
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AApexCharacterBase::OnInteract);
		}
		if (TacticalAction)
		{
			EIC->BindAction(TacticalAction, ETriggerEvent::Started, this , &AApexCharacterBase::ActivateTactical);
		}
		if (UltimateAction)
		{
			EIC->BindAction(UltimateAction, ETriggerEvent::Started, this, &AApexCharacterBase::ActivateUltimate);
		}
		if (SwitchARAction)
		{
			EIC->BindAction(SwitchARAction, ETriggerEvent::Started, this, &AApexCharacterBase::SwitchToSlot0);
		}
		if (SwitchPistolAction)
		{
			EIC->BindAction(SwitchPistolAction, ETriggerEvent::Started, this, &AApexCharacterBase::SwitchToSlot1);
		}
		if (SwitchShotgunAction)
		{
			EIC->BindAction(SwitchShotgunAction, ETriggerEvent::Started, this, &AApexCharacterBase::SwitchToSlot2);
		}
		if (SwitchGrenadeAction)
		{
			EIC->BindAction(SwitchGrenadeAction, ETriggerEvent::Started, this, &AApexCharacterBase::SwitchToSlot3);
		}
		if (DropAction)
		{
			EIC->BindAction(DropAction, ETriggerEvent::Started, this, &AApexCharacterBase::OnDropPressed);
		}
	}
	else
	{
		UE_LOG(LogWP_4th, Error, TEXT("'%s' Failed to find an Enhanced Input Component!"), *GetNameSafe(this));
	}
}

void AApexCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AApexCharacterBase, CurrentWeapon);
	// 슬롯 모드는 본인+타 플레이어 시각/입력 분기에 영향 → 모든 클라 복제
	DOREPLIFETIME(AApexCharacterBase, CurrentSlot);
	DOREPLIFETIME(AApexCharacterBase, bIsSprinting);
	DOREPLIFETIME(AApexCharacterBase, bIsSliding);
	DOREPLIFETIME(AApexCharacterBase, SlideAnimationPhase);
	DOREPLIFETIME_CONDITION(AApexCharacterBase, LightAmmo,       COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AApexCharacterBase, HeavyAmmo,       COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AApexCharacterBase, EnergyAmmo,      COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AApexCharacterBase, ShotgunAmmo,     COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AApexCharacterBase, WeaponSlots,     COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AApexCharacterBase, ActiveSlotIndex, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AApexCharacterBase, GrenadeStock,    COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AApexCharacterBase, ActiveGrenadeID, COND_OwnerOnly);
}

void AApexCharacterBase::MoveInput(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void AApexCharacterBase::LookInput(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoAim(LookAxisVector.X, LookAxisVector.Y);
}

void AApexCharacterBase::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AApexCharacterBase::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AApexCharacterBase::DoJumpStart()
{
	if (bIsSliding) { Server_SlideJump(); return; }
	if (PakComp && PakComp->TryHandleJump()) return;
	Jump();
}

void AApexCharacterBase::DoJumpEnd()
{
	StopJumping();
	if (PakComp) PakComp->bClimbInputHeld = false;
}

float AApexCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || !IsValid(HealthComponent))
	{
		return 0.f;
	}

	if (IsValid(EventInstigator))
	{
		LastDamageInstigatorController = EventInstigator;
		LastDamageTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	}

	HealthComponent->ApplyDamage(DamageAmount, false);
	return DamageAmount;
}

void AApexCharacterBase::StartSprint()
{
	if (!bIsSliding)
	{
		Server_StartSprint();
	}
}

void AApexCharacterBase::StopSprint()
{
	Server_StopSprint();
}

void AApexCharacterBase::Server_StartSprint_Implementation()
{
	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AApexCharacterBase::Server_StopSprint_Implementation()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = bIsSliding ? SlideMaxSpeed : WalkSpeed;
}

void AApexCharacterBase::OnRep_IsSprinting()
{
}

void AApexCharacterBase::StartCrouch()
{
	if (!bIsSliding)
	{
		Crouch();
	}
}

void AApexCharacterBase::StopCrouch()
{
	if (!bIsSliding)
	{
		UnCrouch();
	}
}

void AApexCharacterBase::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPersonMesh->SetRelativeLocation(DefaultFirstPersonMeshLocation + FVector(0.f, 0.f, -ScaledHalfHeightAdjust));
}

void AApexCharacterBase::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPersonMesh->SetRelativeLocation(DefaultFirstPersonMeshLocation);
}

bool AApexCharacterBase::CanStartSlide() const
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	return !bIsSliding
		&& MovementComponent
		&& MovementComponent->IsMovingOnGround()
		&& MovementComponent->Velocity.Size2D() >= SprintSpeed * 0.8f;
}

void AApexCharacterBase::StartSlide()
{
	if (CanStartSlide())
	{
		Server_StartSlide();
	}
}

void AApexCharacterBase::StopSlide()
{
	Server_StopSlide();
}

void AApexCharacterBase::Server_StartSlide_Implementation()
{
	if (!CanStartSlide())
	{
		return;
	}

	BeginSlide();
	Multicast_PlaySlideAnim();
}

void AApexCharacterBase::Server_StopSlide_Implementation()
{
	EndSlide(true);
}

void AApexCharacterBase::Server_SlideJump_Implementation()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const FVector CurrentVelocity = MovementComponent->Velocity;
	const FVector HorizontalVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.f);
	const float HorizontalSpeed = HorizontalVelocity.Size();
	const float SpeedRatio = FMath::Clamp(HorizontalSpeed / SlideMaxSpeed, 0.f, 1.f);
	const float JumpBoost = FMath::Lerp(1.0f, SlideJumpSpeedMultiplier, SpeedRatio);

	FVector LaunchVelocity = HorizontalVelocity.GetSafeNormal() * HorizontalSpeed * JumpBoost;
	LaunchVelocity.Z = MovementComponent->JumpZVelocity;

	EndSlide(false);
	LaunchCharacter(LaunchVelocity, true, true);
}

void AApexCharacterBase::Multicast_PlaySlideAnim_Implementation()
{
}

void AApexCharacterBase::BeginSlide()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	const FVector HorizontalVelocity(MovementComponent->Velocity.X, MovementComponent->Velocity.Y, 0.f);

	SlideDirection = HorizontalVelocity.GetSafeNormal();
	if (SlideDirection.IsNearlyZero())
	{
		SlideDirection = GetActorForwardVector().GetSafeNormal();
	}

	SlideUngroundedTime = 0.f;
	SlideSpeed = HorizontalVelocity.Size();
	Crouch();
	bIsSliding = true;
	ApplySlideMovementSettings();
	SetSlideAnimationPhaseState(ESlideAnimationPhase::Enter);

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	SlideEnterEndTime = CurrentTime + SlideEnterDuration;
	SlideExitEndTime = 0.f;
}

void AApexCharacterBase::EndSlide(bool bPlayExitPhase)
{
	if (!bIsSliding && (!bPlayExitPhase || SlideAnimationPhase == ESlideAnimationPhase::None))
	{
		return;
	}

	bIsSliding = false;
	RestoreDefaultMovementSettings();
	UnCrouch();

	if (bPlayExitPhase)
	{
		SetSlideAnimationPhaseState(ESlideAnimationPhase::Exit);
		SlideExitEndTime = GetWorld()->GetTimeSeconds() + SlideExitDuration;
	}
	else
	{
		SetSlideAnimationPhaseState(ESlideAnimationPhase::None);
		SlideExitEndTime = 0.f;
	}
}

void AApexCharacterBase::SetSlideAnimationPhaseState(ESlideAnimationPhase NewPhase)
{
	SlideAnimationPhase = NewPhase;
}

void AApexCharacterBase::ApplySlideMovementSettings()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->GroundFriction = 0.0f;
	MovementComponent->BrakingDecelerationWalking = 0.0f;
	MovementComponent->MaxWalkSpeed = SlideMaxSpeed;
	MovementComponent->MaxWalkSpeedCrouched = SlideMaxSpeed; // Crouch 상태에서 실제 적용되는 속도 상한
}

void AApexCharacterBase::RestoreDefaultMovementSettings()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->GroundFriction = DefaultGroundFriction;
	MovementComponent->BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
	MovementComponent->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	MovementComponent->MaxWalkSpeedCrouched = DefaultMaxWalkSpeedCrouched;
}

void AApexCharacterBase::TickSlide(float DeltaTime)
{
	if (HasAuthority() && SlideAnimationPhase == ESlideAnimationPhase::Exit && SlideExitEndTime > 0.f)
	{
		if (GetWorld()->GetTimeSeconds() >= SlideExitEndTime)
		{
			SetSlideAnimationPhaseState(ESlideAnimationPhase::None);
			SlideExitEndTime = 0.f;
		}
	}

	if (!bIsSliding || !HasAuthority())
	{
		return;
	}

	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	if (!MovementComponent->IsMovingOnGround())
	{
		SlideUngroundedTime += DeltaTime;
		if (SlideUngroundedTime >= SlideUngroundedGracePeriod)
		{
			EndSlide(true);
		}
		return;
	}

	SlideUngroundedTime = 0.f;

	if (SlideAnimationPhase == ESlideAnimationPhase::Enter && GetWorld()->GetTimeSeconds() >= SlideEnterEndTime)
	{
		SetSlideAnimationPhaseState(ESlideAnimationPhase::Loop);
	}

	const FVector FloorNormal = MovementComponent->CurrentFloor.HitResult.IsValidBlockingHit()
		? MovementComponent->CurrentFloor.HitResult.Normal.GetSafeNormal()
		: FVector::UpVector;
	const FVector PlaneVelocity = FVector::VectorPlaneProject(MovementComponent->Velocity, FloorNormal);
	FVector MoveDirection = PlaneVelocity.GetSafeNormal();

	if (MoveDirection.IsNearlyZero())
	{
		MoveDirection = FVector::VectorPlaneProject(SlideDirection, FloorNormal).GetSafeNormal();
	}

	if (MoveDirection.IsNearlyZero())
	{
		EndSlide(true);
		return;
	}

	SlideDirection = MoveDirection;

	const FVector DownhillVector = FVector::VectorPlaneProject(FVector(0.f, 0.f, -1.f), FloorNormal);
	const FVector DownhillDirection = DownhillVector.GetSafeNormal();
	const float SlopeAmount = DownhillVector.Size();
	const float DownhillAlignment = FVector::DotProduct(MoveDirection, DownhillDirection);
	const float UphillAlignment = FMath::Max(-DownhillAlignment, 0.f);
	// SlideSpeed로 가속/감속 계산 (Velocity에서 읽으면 slope 재투영마다 속도 손실)
	float SpeedDelta = -SlideFlatDeceleration * DeltaTime;
	SpeedDelta -= UphillAlignment * SlopeAmount * SlideUphillDeceleration * DeltaTime;
	SpeedDelta += FMath::Max(DownhillAlignment, 0.f) * SlopeAmount * SlideDownhillAcceleration * DeltaTime;
	SpeedDelta += FMath::Max(DownhillAlignment, 0.f) * SlopeAmount * SlopeAccelMultiplier * DeltaTime;

	SlideSpeed = FMath::Clamp(SlideSpeed + SpeedDelta, 0.f, SlideMaxSpeed);
	if (SlideSpeed <= KINDA_SMALL_NUMBER)
	{
		EndSlide(true);
		return;
	}

	// 수평 방향으로만 velocity 설정 — movement component가 slope following 처리
	FVector HorizDir = FVector(MoveDirection.X, MoveDirection.Y, 0.f).GetSafeNormal();
	if (HorizDir.IsNearlyZero())
		HorizDir = FVector(SlideDirection.X, SlideDirection.Y, 0.f).GetSafeNormal();
	MovementComponent->Velocity = HorizDir * SlideSpeed;

	if (SlideSpeed < SlideMinSpeed && DownhillAlignment <= 0.05f)
	{
		EndSlide(true);
	}
}

void AApexCharacterBase::OnRep_IsSliding()
{
	if (bIsSliding)
	{
		ApplySlideMovementSettings();
		if (SlideAnimationPhase == ESlideAnimationPhase::None)
		{
			SetSlideAnimationPhaseState(ESlideAnimationPhase::Enter);
		}
	}
	else
	{
		RestoreDefaultMovementSettings();
		if (SlideAnimationPhase == ESlideAnimationPhase::Loop)
		{
			SetSlideAnimationPhaseState(ESlideAnimationPhase::Exit);
		}
	}
}

void AApexCharacterBase::OnRep_SlideAnimationPhase()
{
}

void AApexCharacterBase::OnAimStarted()
{
	if (CurrentSlot != EEquippedSlot::Weapon || !CurrentWeapon) return;
	Server_SetAiming(!CurrentWeapon->bIsAiming);
}

void AApexCharacterBase::OnAimStopped()
{
	Server_SetAiming(false);

}

void AApexCharacterBase::StartFire()
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

void AApexCharacterBase::StopFire()
{
	if (CurrentSlot == EEquippedSlot::Grenade && bIsAimingThrowable)
	{
		StopThrowableAim();

		// 카메라 방향 계산 후 서버 권한으로 위임 (클라 로컬 SpawnActor ghost 회피, 회귀 #8 방지)
		FVector ThrowDirection = FVector::ForwardVector;
		if (GetController())
		{
			FVector CameraLocation;
			FRotator CameraRotation;
			GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);

			ThrowDirection = CameraRotation.Vector();
			ThrowDirection.Z += 0.2f;
			ThrowDirection.Normalize();
		}

		ServerThrowGrenade(ThrowDirection);
		return;
	}

	if (CurrentSlot == EEquippedSlot::Weapon && CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

bool AApexCharacterBase::ServerThrowGrenade_Validate(FVector ThrowDirection)
{
	return !ThrowDirection.IsNearlyZero();
}

void AApexCharacterBase::ServerThrowGrenade_Implementation(FVector ThrowDirection)
{
	// ThrowDirection은 호환성 인자 — 현재 ThrowGrenade()가 자체 카메라 방향 계산 사용
	// (향후 ThrowGrenade(FVector) 오버로드 분리 시 클라 방향 전달 가능)
	ThrowGrenade();

	if (GetTotalGrenadeCount() <= 0)
	{
		SwitchToFirstAvailableSlot();
	}
}

void AApexCharacterBase::Reload()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartReload();
	}
}

void AApexCharacterBase::HandleDeath()
{
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (AApexDeathmatchGameMode* GM = World->GetAuthGameMode<AApexDeathmatchGameMode>())
			{
				AController* Killer = LastDamageInstigatorController.Get();
				const float Dt = World->GetTimeSeconds() - LastDamageTime;
				if (Dt > DamageAttributionWindowSeconds)
				{
					Killer = nullptr;
				}

				GM->HandleApexPawnKilled(Killer, GetController());
			}
		}

		Multicast_OnDeath();
	}
}

void AApexCharacterBase::OnInteract()
{
	if (ZiplineComp) ZiplineComp->TryInterract();

	if (!InteractionComp || !InteractionComp->CurrentInteractable) return;

	AActor* Target = InteractionComp->CurrentInteractable;
	IInteractableInterface* Iface = Cast<IInteractableInterface>(Target);
	if (!Iface || !Iface->CanInteract(this)) return;

	if (HasAuthority())
		Iface->OnInteract(this);
	else
		ServerInteract(Target);
}

void AApexCharacterBase::ServerInteract_Implementation(AActor* TargetInteractable)
{
	if (!TargetInteractable) return;

	const float MaxAllowedDist = 500.f;
	if (GetDistanceTo(TargetInteractable) > MaxAllowedDist)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interact] Target too far: %.1f"), GetDistanceTo(TargetInteractable));
		return;
	}

	IInteractableInterface* Iface = Cast<IInteractableInterface>(TargetInteractable);
	if (Iface && Iface->CanInteract(this))
		Iface->OnInteract(this);
}

void AApexCharacterBase::Server_SetAiming_Implementation(bool bAiming)
{
	if (!CurrentWeapon) return;

	if (bAiming)
		CurrentWeapon->StartAiming();
	else
		CurrentWeapon->StopAiming();

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp) return;

	if (bAiming)
	{
		if (SavedDefaultWalkSpeed <= 0.f)
			SavedDefaultWalkSpeed = MoveComp->MaxWalkSpeed;
		MoveComp->MaxWalkSpeed = SavedDefaultWalkSpeed * ADSWalkSpeedMultiplier;
	}
	else
	{
		if (SavedDefaultWalkSpeed > 0.f)
			MoveComp->MaxWalkSpeed = SavedDefaultWalkSpeed;
	}

}

void AApexCharacterBase::Multicast_OnDeath_Implementation()
{
	GetMesh()->SetSimulatePhysics(true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (HasAuthority())
	{
		if (AController* PC = GetController())
		{
			PC->UnPossess();
		}
	}
}

// ==================== 슬롯 입력 래퍼 ====================

void AApexCharacterBase::SwitchToSlot0() { ServerSwitchToSlot(0); }
void AApexCharacterBase::SwitchToSlot1() { ServerSwitchToSlot(1); }
void AApexCharacterBase::SwitchToSlot2() { ServerSwitchToSlot(2); }
void AApexCharacterBase::SwitchToSlot3() { ServerSwitchToSlot(3); }

void AApexCharacterBase::OnDropPressed()
{
	ServerDropCurrentWeapon();
}

// ==================== 4슬롯 무기 시스템 ====================

bool AApexCharacterBase::IsSlotEmpty(int32 SlotIndex) const
{
	return SlotIndex < 0 || SlotIndex >= WeaponSlots.Num() || WeaponSlots[SlotIndex].IsNone();
}

int32 AApexCharacterBase::FindNextAvailableSlot(int32 SkipIndex) const
{
	for (int32 i = 0; i < WeaponSlots.Num(); ++i)
	{
		if (i == SkipIndex) continue;
		if (!WeaponSlots[i].IsNone()) return i;
	}
	return -1;
}

EWeaponSlotType AApexCharacterBase::GetSlotForCategory(EWeaponType WeaponCategory) const
{
	switch (WeaponCategory)
	{
		case EWeaponType::Pistol:    return EWeaponSlotType::Pistol;
		case EWeaponType::Throwable: return EWeaponSlotType::Throwable;
		default:                     return EWeaponSlotType::Main1;
	}
}

void AApexCharacterBase::SwitchToSlot_Internal(int32 SlotIndex)
{
	if (SlotIndex == (int32)EWeaponSlotType::Throwable)
	{
		if (GetTotalGrenadeCount() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Slot] No grenades left!"));
			return;
		}

		// 4번 슬롯 재진입 = 다음 종류로 사이클
		if (CurrentSlot == EEquippedSlot::Grenade && ActiveSlotIndex == SlotIndex)
		{
			const FName Next = GetNextGrenadeIDInCycle();
			if (!Next.IsNone() && Next != ActiveGrenadeID)
			{
				ActiveGrenadeID = Next;
				if (WeaponSlots.IsValidIndex(SlotIndex))
					WeaponSlots[SlotIndex] = Next;
				UE_LOG(LogTemp, Warning, TEXT("[Slot] Cycled active grenade -> %s"), *Next.ToString());
			}
			return;
		}

		// ActiveGrenadeID 보정: 비었거나 카운트 0이면 첫 보유 종류로 고정
		if (ActiveGrenadeID.IsNone() || GetGrenadeCountByID(ActiveGrenadeID) == 0)
		{
			const TArray<FName> Avail = GetAvailableGrenadeIDs();
			if (Avail.Num() > 0)
			{
				ActiveGrenadeID = Avail[0];
				if (WeaponSlots.IsValidIndex(SlotIndex))
					WeaponSlots[SlotIndex] = Avail[0];
			}
			else return;
		}

		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
			CurrentWeapon->SetActorHiddenInGame(true);
		}

		ActiveSlotIndex = SlotIndex;
		CurrentSlot = EEquippedSlot::Grenade;
		UE_LOG(LogTemp, Warning, TEXT("[Slot] Switched to Throwable (Active: %s, %d)"),
			*ActiveGrenadeID.ToString(), GetGrenadeCountByID(ActiveGrenadeID));
	}
	else
	{
		if (IsSlotEmpty(SlotIndex)) return;
		ActiveSlotIndex = SlotIndex;
		SwitchWeaponByID(WeaponSlots[SlotIndex]);
	}
}

void AApexCharacterBase::SpawnPickupFromSlot(int32 SlotIndex)
{
	if (!HasAuthority()) return;
	if (IsSlotEmpty(SlotIndex)) return;
	if (!PickupClass) return;

	const FName WeaponID = WeaponSlots[SlotIndex];
	const FVector Forward = GetActorForwardVector();
	const FVector Location = GetActorLocation() + Forward * 80.0f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;

	APickupBase* Pickup = GetWorld()->SpawnActor<APickupBase>(
		PickupClass, Location, FRotator::ZeroRotator, Params);

	if (Pickup)
	{
		Pickup->PickupWeaponID = WeaponID;
		Pickup->ForceNetUpdate();
		Pickup->RefreshFromDataTable();
	}
}

void AApexCharacterBase::ServerSwitchToSlot_Implementation(int32 SlotIndex)
{
	if (!HasAuthority()) return;
	if (SlotIndex < 0 || SlotIndex >= WeaponSlots.Num()) return;
	if (SlotIndex == (int32)EWeaponSlotType::Throwable)
	{
		if (GetTotalGrenadeCount() <= 0) return;
	}
	else
	{
		if (IsSlotEmpty(SlotIndex)) return;
	}
	SwitchToSlot_Internal(SlotIndex);
}

void AApexCharacterBase::ServerAddWeaponToSlot_Implementation(FName WeaponID)
{
	if (!HasAuthority()) return;
	if (WeaponID.IsNone()) return;

	EWeaponType Category = EWeaponType::Rifle;

	if (GenericWeaponClass)
	{
		AWeaponBase* WeaponCDO = GenericWeaponClass->GetDefaultObject<AWeaponBase>();
		if (WeaponCDO && WeaponCDO->WeaponDataTable)
		{
			const FWeaponData* Data = WeaponCDO->WeaponDataTable->FindRow<FWeaponData>(
				WeaponID, TEXT("ServerAddWeaponToSlot"));
			if (Data) Category = Data->Category;
		}
	}
	if (Category == EWeaponType::Rifle && GenericThrowableClass)
	{
		AThrowableBase* ThrowableCDO = GenericThrowableClass->GetDefaultObject<AThrowableBase>();
		if (ThrowableCDO && ThrowableCDO->WeaponDataTable)
		{
			const FWeaponData* Data = ThrowableCDO->WeaponDataTable->FindRow<FWeaponData>(
				WeaponID, TEXT("ServerAddWeaponToSlot_Throwable"));
			if (Data) Category = Data->Category;
		}
	}

	int32 TargetSlot = -1;

	if (Category == EWeaponType::Throwable)
	{
		// helper에 모든 결정권 위임 (풀 거부, 활성 종류 보존, 슬롯 갱신)
		TryAddGrenadeAuth(WeaponID);
		return;
	}
	else
	{
		// Pistol 포함 모든 총기류 → Main1(0) 또는 Main2(1)
		if (IsSlotEmpty((int32)EWeaponSlotType::Main1))
			TargetSlot = (int32)EWeaponSlotType::Main1;
		else if (IsSlotEmpty((int32)EWeaponSlotType::Main2))
			TargetSlot = (int32)EWeaponSlotType::Main2;
		else
			TargetSlot = (ActiveSlotIndex == 0 || ActiveSlotIndex == 1)
					   ? ActiveSlotIndex
					   : (int32)EWeaponSlotType::Main1;
	}

	if (TargetSlot < 0) return;

	const bool bSlotWasOccupied = !IsSlotEmpty(TargetSlot);
	const bool bIsActiveSlot = (TargetSlot == ActiveSlotIndex);

	if (bSlotWasOccupied) SpawnPickupFromSlot(TargetSlot);

	WeaponSlots[TargetSlot] = WeaponID;

	if (ActiveSlotIndex < 0 || (bSlotWasOccupied && bIsActiveSlot))
		SwitchToSlot_Internal(TargetSlot);
}

void AApexCharacterBase::ServerDropCurrentWeapon_Implementation()
{
	if (!HasAuthority()) return;
	if (ActiveSlotIndex < 0) return;
	if (IsSlotEmpty(ActiveSlotIndex)) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastDropTime < DropCooldown) return;
	LastDropTime = Now;

	SpawnPickupFromSlot(ActiveSlotIndex);

	const int32 DroppedSlot = ActiveSlotIndex;
	WeaponSlots[DroppedSlot] = NAME_None;

	if (DroppedSlot == (int32)EWeaponSlotType::Throwable)
	{
		GrenadeStock.Empty();
		ActiveGrenadeID = NAME_None;
	}

	const int32 NextSlot = FindNextAvailableSlot(DroppedSlot);
	if (NextSlot >= 0)
	{
		SwitchToSlot_Internal(NextSlot);
	}
	else
	{
		if (CurrentWeapon)
		{
			CurrentWeapon->Destroy();
			CurrentWeapon = nullptr;
		}
		if (CurrentSlot == EEquippedSlot::Grenade)
		{
			CurrentSlot = EEquippedSlot::Weapon;
			StopThrowableAim();
		}
		ActiveSlotIndex = -1;
	}
}

void AApexCharacterBase::AddGrenade(FName GrenadeID)
{
	if (HasAuthority())
		TryAddGrenadeAuth(GrenadeID);
}

bool AApexCharacterBase::TryAddGrenadeAuth(FName GrenadeID)
{
	if (!HasAuthority()) return false;
	if (GrenadeID.IsNone()) return false;

	if (IsGrenadeStockFull(GrenadeID))
	{
		UE_LOG(LogTemp, Log, TEXT("[Grenade] TryAddGrenadeAuth: %s full (%d/%d), refused"),
			*GrenadeID.ToString(), GetGrenadeCountByID(GrenadeID), MaxGrenadeCount);
		return false;
	}

	const bool bAdded = AddGrenadeStock(GrenadeID, 1);
	if (!bAdded) return false;

	// 활성 종류가 비어있을 때만 갱신
	if (ActiveGrenadeID.IsNone() || GetGrenadeCountByID(ActiveGrenadeID) == 0)
	{
		ActiveGrenadeID = GrenadeID;
		if (WeaponSlots.IsValidIndex((int32)EWeaponSlotType::Throwable))
			WeaponSlots[(int32)EWeaponSlotType::Throwable] = GrenadeID;
	}

	UE_LOG(LogTemp, Log, TEXT("[Grenade] TryAddGrenadeAuth: %s, count=%d/%d (Active: %s)"),
		*GrenadeID.ToString(), GetGrenadeCountByID(GrenadeID), MaxGrenadeCount, *ActiveGrenadeID.ToString());
	return true;
}

void AApexCharacterBase::ThrowGrenade()
{
	if (ActiveGrenadeID.IsNone() || GetGrenadeCountByID(ActiveGrenadeID) <= 0) return;
	if (!GenericThrowableClass || !GetController()) return;

	FVector CameraLocation;
	FRotator CameraRotation;
	GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);

	FVector SpawnLocation = CameraLocation + CameraRotation.Vector() * 100.f;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AThrowableBase* Grenade = GetWorld()->SpawnActor<AThrowableBase>(
		GenericThrowableClass, SpawnLocation, CameraRotation, SpawnParams);

	if (!Grenade) return;

	Grenade->InitFromDataTable(ActiveGrenadeID);

	FVector ThrowDirection = CameraRotation.Vector();
	ThrowDirection.Z += 0.2f;
	ThrowDirection.Normalize();

	Grenade->ServerThrow(ThrowDirection);

	if (HasAuthority())
	{
		const FName ThrownID = ActiveGrenadeID;
		RemoveGrenadeStock(ThrownID, 1);

		const int32 Remaining = GetGrenadeCountByID(ThrownID);
		UE_LOG(LogTemp, Warning, TEXT("[Grenade] Thrown! %s remaining: %d"), *ThrownID.ToString(), Remaining);

		if (Remaining == 0)
		{
			const FName Next = GetNextGrenadeIDInCycle();
			if (!Next.IsNone() && Next != ThrownID)
			{
				ActiveGrenadeID = Next;
				if (WeaponSlots.IsValidIndex((int32)EWeaponSlotType::Throwable))
					WeaponSlots[(int32)EWeaponSlotType::Throwable] = Next;
			}
			else
			{
				ActiveGrenadeID = NAME_None;
				if (WeaponSlots.IsValidIndex((int32)EWeaponSlotType::Throwable))
					WeaponSlots[(int32)EWeaponSlotType::Throwable] = NAME_None;
			}
		}
	}
}

void AApexCharacterBase::StartThrowableAim()
{
	bIsAimingThrowable = true;
}

void AApexCharacterBase::StopThrowableAim()
{
	bIsAimingThrowable = false;
}

// ==================== Ammo Pool ====================

int32 AApexCharacterBase::GetMaxAmmoForType(EAmmoType Type) const
{
	switch (Type)
	{
	case EAmmoType::Light:   return MaxLightAmmo;
	case EAmmoType::Heavy:   return MaxHeavyAmmo;
	case EAmmoType::Energy:  return MaxEnergyAmmo;
	case EAmmoType::Shotgun: return MaxShotgunAmmo;
	case EAmmoType::Sniper:  return MaxHeavyAmmo;
	default:                 return 0;
	}
}

int32 AApexCharacterBase::GetAmmoForType(EAmmoType Type) const
{
	switch (Type)
	{
	case EAmmoType::Light:   return LightAmmo;
	case EAmmoType::Heavy:   return HeavyAmmo;
	case EAmmoType::Energy:  return EnergyAmmo;
	case EAmmoType::Shotgun: return ShotgunAmmo;
	case EAmmoType::Sniper:  return HeavyAmmo;
	default:                 return 0;
	}
}

void AApexCharacterBase::SetAmmoForType(EAmmoType Type, int32 NewAmount)
{
	switch (Type)
	{
	case EAmmoType::Light:   LightAmmo   = NewAmount; break;
	case EAmmoType::Heavy:   HeavyAmmo   = NewAmount; break;
	case EAmmoType::Energy:  EnergyAmmo  = NewAmount; break;
	case EAmmoType::Shotgun: ShotgunAmmo = NewAmount; break;
	case EAmmoType::Sniper:  HeavyAmmo   = NewAmount; break;
	default: break;
	}
}

int32 AApexCharacterBase::GetReserveAmmo(EAmmoType Type) const
{
	return GetAmmoForType(Type);
}

int32 AApexCharacterBase::AddAmmo(EAmmoType Type, int32 Count)
{
	if (Count <= 0) return 0;

	if (!HasAuthority())
	{
		ServerAddAmmo(Type, Count);
		return 0;
	}

	const int32 Current = GetAmmoForType(Type);
	const int32 Max = GetMaxAmmoForType(Type);
	const int32 Added = FMath::Clamp(Max - Current, 0, Count);
	if (Added <= 0) return 0;

	const int32 NewAmount = Current + Added;
	SetAmmoForType(Type, NewAmount);

	OnReserveAmmoChanged.Broadcast(Type, NewAmount);
	return Added;
}

int32 AApexCharacterBase::ConsumeReserve(EAmmoType Type, int32 Needed)
{
	if (Needed <= 0 || !HasAuthority()) return 0;

	const int32 Current = GetAmmoForType(Type);
	if (Current <= 0) return 0;

	const int32 Consumed = FMath::Min(Needed, Current);
	const int32 NewAmount = Current - Consumed;
	SetAmmoForType(Type, NewAmount);

	OnReserveAmmoChanged.Broadcast(Type, NewAmount);
	return Consumed;
}

void AApexCharacterBase::ServerAddAmmo_Implementation(EAmmoType Type, int32 Count)
{
	AddAmmo(Type, Count);
}

// ==================== Grenade Stock Helpers ====================

int32 AApexCharacterBase::GetGrenadeCountByID(FName GrenadeID) const
{
	for (const FGrenadeStockEntry& E : GrenadeStock)
		if (E.GrenadeID == GrenadeID) return E.Count;
	return 0;
}

int32 AApexCharacterBase::GetTotalGrenadeCount() const
{
	int32 Total = 0;
	for (const FGrenadeStockEntry& E : GrenadeStock) Total += E.Count;
	return Total;
}

bool AApexCharacterBase::IsGrenadeStockFull(FName GrenadeID) const
{
	return GetGrenadeCountByID(GrenadeID) >= MaxGrenadeCount;
}

TArray<FName> AApexCharacterBase::GetAvailableGrenadeIDs() const
{
	TArray<FName> Out;
	for (const FGrenadeStockEntry& E : GrenadeStock)
		if (E.Count > 0) Out.Add(E.GrenadeID);
	return Out;
}

FName AApexCharacterBase::GetNextGrenadeIDInCycle() const
{
	const TArray<FName> Avail = GetAvailableGrenadeIDs();
	if (Avail.Num() == 0) return NAME_None;
	const int32 Idx = Avail.IndexOfByKey(ActiveGrenadeID);
	return Avail[(Idx == INDEX_NONE ? 0 : (Idx + 1) % Avail.Num())];
}

bool AApexCharacterBase::AddGrenadeStock(FName GrenadeID, int32 Amount)
{
	if (GrenadeID.IsNone() || Amount <= 0) return false;
	if (IsGrenadeStockFull(GrenadeID)) return false;

	for (FGrenadeStockEntry& E : GrenadeStock)
	{
		if (E.GrenadeID == GrenadeID)
		{
			E.Count = FMath::Min(E.Count + Amount, MaxGrenadeCount);
			return true;
		}
	}
	FGrenadeStockEntry NewEntry;
	NewEntry.GrenadeID = GrenadeID;
	NewEntry.Count = FMath::Min(Amount, MaxGrenadeCount);
	GrenadeStock.Add(NewEntry);
	return true;
}

bool AApexCharacterBase::RemoveGrenadeStock(FName GrenadeID, int32 Amount)
{
	if (GrenadeID.IsNone() || Amount <= 0) return false;
	for (int32 i = 0; i < GrenadeStock.Num(); ++i)
	{
		if (GrenadeStock[i].GrenadeID == GrenadeID)
		{
			GrenadeStock[i].Count = FMath::Max(0, GrenadeStock[i].Count - Amount);
			return true;
		}
	}
	return false;
}

void AApexCharacterBase::SwitchToFirstAvailableSlot()
{
	if (!HasAuthority()) return;
	for (int32 i = 0; i < WeaponSlots.Num(); ++i)
	{
		if (i == (int32)EWeaponSlotType::Throwable) continue;
		if (!WeaponSlots[i].IsNone())
		{
			SwitchToSlot_Internal(i);
			return;
		}
	}
	ActiveSlotIndex = -1;
}

// ─── Weapon System Interface (CLAUDE.md 합의) ─────────────────────
void AApexCharacterBase::ServerApplyDamage(float Damage, ACharacter* DamageInstigator, FHitResult HitResult)
{
	// stub — 실제 데미지 적용은 표준 TakeDamage 흐름(HealthComponent)이 처리
	UE_LOG(LogTemp, Warning, TEXT("[Apex] ServerApplyDamage: %.1f, Bone: %s"), Damage, *HitResult.BoneName.ToString());
}

void AApexCharacterBase::ClientShowHitMarker_Implementation(bool bIsHeadshot)
{
	// stub — 후속 작업에서 HUD 위젯 hit marker 트리거 예정
	UE_LOG(LogTemp, Warning, TEXT("[Apex] HitMarker! Headshot: %d"), bIsHeadshot);
}

FVector AApexCharacterBase::GetAimDirection() const
{
	if (FirstPersonCameraComponent)
	{
		return FirstPersonCameraComponent->GetForwardVector();
	}
	return GetActorForwardVector();
}
