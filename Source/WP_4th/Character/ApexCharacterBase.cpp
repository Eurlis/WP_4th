#include "ApexCharacterBase.h"
#include "Character/Components/HealthComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "WP_4th.h"
#include "Engine/DamageEvents.h"

AApexCharacterBase::AApexCharacterBase()
{
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// ─── First Person Mesh ────────────────────────────────────────
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// ─── Camera ───────────────────────────────────────────────────
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// ─── Third Person Mesh ────────────────────────────────────────
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);
	// ─── Movement ─────────────────────────────────────────────────
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// ─── Health ───────────────────────────────────────────────────
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	// ─── Speed Defaults ───────────────────────────────────────────
	SprintSpeed              = 700.f;
	WalkSpeed                = 400.f;
	CrouchSpeed              = 200.f;
	SlideMaxSpeed            = 1400.f;
	SlideMinSpeed            = 250.f;
	SlopeAccelMultiplier     = 2400.f;
	SlideJumpSpeedMultiplier = 1.25f;
}

void AApexCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickSlide(DeltaTime);
}

void AApexCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
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
		// ─── Base ─────────────────────────────────────────────────
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started,   this, &AApexCharacterBase::DoJumpStart);
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

		// ─── Sprint ───────────────────────────────────────────────
		if (SprintAction)
		{
			EIC->BindAction(SprintAction, ETriggerEvent::Started,   this, &AApexCharacterBase::StartSprint);
			EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AApexCharacterBase::StopSprint);
		}

		// ─── Crouch ───────────────────────────────────────────────
		if (CrouchAction)
		{
			EIC->BindAction(CrouchAction, ETriggerEvent::Started,   this, &AApexCharacterBase::StartCrouch);
			EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AApexCharacterBase::StopCrouch);
		}

		// ─── Slide ────────────────────────────────────────────────
		if (SlideAction)
		{
			EIC->BindAction(SlideAction, ETriggerEvent::Started,   this, &AApexCharacterBase::StartSlide);
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
}

// ─── Base Input ───────────────────────────────────────────────────────────────

void AApexCharacterBase::MoveInput(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	DoMove(MovementVector.X, MovementVector.Y);
}

void AApexCharacterBase::LookInput(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
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
		AddMovementInput(GetActorRightVector(),   Right);
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
	Jump();
}

void AApexCharacterBase::DoJumpEnd()
{
	StopJumping();
}

// ─── Damage ───────────────────────────────────────────────────────────────────

float AApexCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority()) return 0.f;
	if (!IsValid(HealthComponent)) return 0.f;

	bool bIsHeadshot = false;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);
		bIsHeadshot = PointDamage->HitInfo.BoneName == FName("head");
	}

	HealthComponent->ApplyDamage(DamageAmount, bIsHeadshot);
	return DamageAmount;
}

// ─── Sprint ───────────────────────────────────────────────────────────────────

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
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AApexCharacterBase::OnRep_IsSprinting()
{
	// 애니메이션 블루프린트 연동 시 여기서 처리
}

// ─── Crouch ───────────────────────────────────────────────────────────────────

void AApexCharacterBase::StartCrouch()
{
	Crouch();
}

void AApexCharacterBase::StopCrouch()
{
	UnCrouch();
}

void AApexCharacterBase::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPersonMesh->SetRelativeLocation(
		DefaultFirstPersonMeshLocation + FVector(0.f, 0.f, -ScaledHalfHeightAdjust)
	);
}

void AApexCharacterBase::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	FirstPersonMesh->SetRelativeLocation(DefaultFirstPersonMeshLocation);
}

// ─── Slide ────────────────────────────────────────────────────────────────────

void AApexCharacterBase::StartSlide()
{
	// 최소 스프린트 속도 이상이어야 슬라이딩 진입
	if (GetCharacterMovement()->Velocity.Size2D() >= SprintSpeed * 0.8f)
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
	bIsSliding = true;
	Crouch();
	Multicast_PlaySlideAnim();
}

void AApexCharacterBase::Server_StopSlide_Implementation()
{
	if (!bIsSliding) return;
	bIsSliding = false;
	UnCrouch();
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
}

void AApexCharacterBase::Server_SlideJump_Implementation()
{
	UCharacterMovementComponent* CMC = GetCharacterMovement();

	// 현재 수평 속도 기반으로 점프 런치 벡터 계산
	FVector CurrentVel    = CMC->Velocity;
	float   HorizontalSpeed = CurrentVel.Size2D();
	float   SpeedRatio    = FMath::Clamp(HorizontalSpeed / SlideMaxSpeed, 0.f, 1.f);
	float   JumpBoost     = FMath::Lerp(1.0f, SlideJumpSpeedMultiplier, SpeedRatio);

	FVector LaunchVel = CurrentVel.GetSafeNormal2D() * HorizontalSpeed * JumpBoost;
	LaunchVel.Z       = CMC->JumpZVelocity;

	// 슬라이드 종료 후 발사
	bIsSliding = false;
	UnCrouch();
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;

	LaunchCharacter(LaunchVel, true, true);
}

void AApexCharacterBase::Multicast_PlaySlideAnim_Implementation()
{
	// 추후 AnimInstance 연동
}

void AApexCharacterBase::TickSlide(float DeltaTime)
{
	if (!bIsSliding || !HasAuthority()) return;

	UCharacterMovementComponent* CMC = GetCharacterMovement();
	if (!CMC->IsMovingOnGround())
	{
		// 공중으로 뜨면 슬라이드 종료
		Server_StopSlide();
		return;
	}

	// ── 경사 가속 ────────────────────────────────────────────────
	// 바닥 노멀에서 중력의 사면 성분을 추출 → 내리막 방향 + 크기
	FVector FloorNormal  = CMC->CurrentFloor.HitResult.Normal;
	FVector GravityDir   = FVector(0.f, 0.f, -1.f);
	// GravityDir의 노멀 평면 투영 = 경사 방향 벡터 (크기 = sin(경사각))
	FVector SlopeAccelDir = GravityDir - (FVector::DotProduct(GravityDir, FloorNormal) * FloorNormal);
	float   SlopeSin      = SlopeAccelDir.Size(); // 0=평지, 1=수직

	if (SlopeSin > KINDA_SMALL_NUMBER)
	{
		SlopeAccelDir /= SlopeSin; // Normalize
		CMC->Velocity += SlopeAccelDir * SlopeSin * SlopeAccelMultiplier * DeltaTime;
	}

	// ── 최대 속도 캡 ─────────────────────────────────────────────
	float Speed2D = CMC->Velocity.Size2D();
	if (Speed2D > SlideMaxSpeed)
	{
		FVector2D ClampedXY = FVector2D(CMC->Velocity.X, CMC->Velocity.Y).GetSafeNormal() * SlideMaxSpeed;
		CMC->Velocity.X = ClampedXY.X;
		CMC->Velocity.Y = ClampedXY.Y;
	}

	// ── 평지에서 속도가 너무 느려지면 자동 종료 ─────────────────
	if (SlopeSin < 0.05f && Speed2D < SlideMinSpeed)
	{
		Server_StopSlide();
	}
}

void AApexCharacterBase::OnRep_IsSliding()
{
	// 클라이언트 연출 처리 (카메라 FOV 등)
}

// ─── Death ────────────────────────────────────────────────────────────────────

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
