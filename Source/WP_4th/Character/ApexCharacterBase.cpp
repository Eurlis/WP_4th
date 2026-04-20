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
#include "Net/UnrealNetwork.h"
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
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
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
	SlideMaxSpeed = 1400.f;
	SlideMinSpeed = 250.f;
	SlopeAccelMultiplier = 2400.f;
	SlideJumpSpeedMultiplier = 1.25f;
	SlideEnterDuration = 0.16f;
	SlideExitDuration = 0.20f;
	SlideFlatDeceleration = 950.f;
	SlideUphillDeceleration = 1650.f;
	SlideDownhillAcceleration = 900.f;
	SlideUngroundedGracePeriod = 0.15f;

	SlideDirection = FVector::ForwardVector;
	SlideEnterEndTime = 0.f;
	SlideExitEndTime = 0.f;
	SlideUngroundedTime = 0.f;
	DefaultGroundFriction = MovementComponent->GroundFriction;
	DefaultBrakingDecelerationWalking = MovementComponent->BrakingDecelerationWalking;
}

void AApexCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickSlide(DeltaTime);
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
	}
	else
	{
		UE_LOG(LogWP_4th, Error, TEXT("'%s' Failed to find an Enhanced Input Component!"), *GetNameSafe(this));
	}
}

void AApexCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AApexCharacterBase, bIsSprinting);
	DOREPLIFETIME(AApexCharacterBase, bIsSliding);
	DOREPLIFETIME(AApexCharacterBase, SlideAnimationPhase);
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
	if (bIsSliding)
	{
		Server_SlideJump();
		return;
	}
	if (PakComp && PakComp->CanWallJump())
	{
		return;
	}
	Jump();
}

void AApexCharacterBase::DoJumpEnd()
{
	StopJumping();
}

float AApexCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || !IsValid(HealthComponent))
	{
		return 0.f;
	}

	bool bIsHeadshot = false;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);
		bIsHeadshot = PointDamage->HitInfo.BoneName == FName("head");
	}

	HealthComponent->ApplyDamage(DamageAmount, bIsHeadshot);
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
}

void AApexCharacterBase::RestoreDefaultMovementSettings()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	MovementComponent->GroundFriction = DefaultGroundFriction;
	MovementComponent->BrakingDecelerationWalking = DefaultBrakingDecelerationWalking;
	MovementComponent->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
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
	const float CurrentSpeed = PlaneVelocity.Size();

	float SpeedDelta = -SlideFlatDeceleration * DeltaTime;
	SpeedDelta -= UphillAlignment * SlopeAmount * SlideUphillDeceleration * DeltaTime;
	SpeedDelta += FMath::Max(DownhillAlignment, 0.f) * SlopeAmount * SlideDownhillAcceleration * DeltaTime;
	SpeedDelta += FMath::Max(DownhillAlignment, 0.f) * SlopeAmount * SlopeAccelMultiplier * DeltaTime;

	const float NewSpeed = FMath::Clamp(CurrentSpeed + SpeedDelta, 0.f, SlideMaxSpeed);
	if (NewSpeed <= KINDA_SMALL_NUMBER)
	{
		EndSlide(true);
		return;
	}

	MovementComponent->Velocity = MoveDirection * NewSpeed;

	if (NewSpeed < SlideMinSpeed && DownhillAlignment <= 0.05f)
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

void AApexCharacterBase::HandleDeath()
{
	if (HasAuthority())
	{
		Multicast_OnDeath();
	}
}

void AApexCharacterBase::Multicast_OnDeath_Implementation()
{
	GetMesh()->SetSimulatePhysics(true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (AController* PC = GetController())
	{
		PC->UnPossess();
	}
}
