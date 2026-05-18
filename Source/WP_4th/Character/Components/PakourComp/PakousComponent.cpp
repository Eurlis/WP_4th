// Fill out your copyright notice in the Description page of Project Settings.

#include "PakousComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "MotionWarping/Public/MotionWarpingComponent.h"
#include "Net/UnrealNetwork.h"

UPakousComponent::UPakousComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);
}

void UPakousComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPakousComponent, ParkourState);
	DOREPLIFETIME(UPakousComponent, ClimbStartPos);
	DOREPLIFETIME(UPakousComponent, ClimbTargetPos);
}

void UPakousComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar) return;

	MoveComp   = OwnerChar->GetCharacterMovement();
	CapsuleComp = OwnerChar->GetCapsuleComponent();
	MeshComp   = OwnerChar->GetMesh();
	WarpComp   = OwnerChar->FindComponentByClass<UMotionWarpingComponent>();

	AnimInstance = MeshComp ? MeshComp->GetAnimInstance() : nullptr;
	if (AnimInstance)
		AnimInstance->OnMontageEnded.AddDynamic(this, &UPakousComponent::OnClimbUpEnd);
}

void UPakousComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerChar) return;

	// 모든 상태 로직은 서버 전용
	if (!GetOwner()->HasAuthority()) return;

	if (WallJumpCooldownTimer > 0.f)
		WallJumpCooldownTimer -= DeltaTime;
	if (ExitCooldownTimer > 0.f)
		ExitCooldownTimer -= DeltaTime;

	DetectWall();

	switch (ParkourState)
	{
	case EParkourState::None:
		if (MoveComp->IsMovingOnGround())
			bHasWallJumped = false;
		if (!MoveComp->IsMovingOnGround() && CanEnterWallAttach())
			EnterWallAttach();
		break;

	case EParkourState::WallAttach:
		TickWallAttach(DeltaTime);
		break;

	case EParkourState::WallClimb:
		TickWallClimb(DeltaTime);
		break;

	case EParkourState::WallSlide:
		TickWallSlide(DeltaTime);
		break;

	case EParkourState::ClimbingUp:
		TickClimbingUp(DeltaTime);
		break;
	}
}

// ─────────────────────────────────────────────────────────────
//  멀티플레이 입력 API
// ─────────────────────────────────────────────────────────────
void UPakousComponent::SetClimbInput(bool bHeld)
{
	if (GetOwner()->HasAuthority())
	{
		bClimbInputHeld = bHeld;
	}
	else
	{
		ServerSetClimbInput(bHeld);
	}
}

void UPakousComponent::ServerSetClimbInput_Implementation(bool bHeld)
{
	bClimbInputHeld = bHeld;
}

bool UPakousComponent::TryHandleJump()
{
	if (!GetOwner()->HasAuthority())
	{
		ServerTryHandleJump();
		// ParkourState는 복제되므로 클라에서도 정확함.
		// None 상태면 false 반환 → 캐릭터의 일반 Jump() 허용
		return ParkourState != EParkourState::None;
	}

	if (ParkourState == EParkourState::WallAttach)
	{
		TriggerWallJump();
		return true;
	}

	bClimbInputHeld = true;

	if (GetIsClimbing())
	{
		if (!TryClimbUp()) ExitClimb(true);
		return true;
	}

	return TryParkour();
}

void UPakousComponent::ServerTryHandleJump_Implementation()
{
	TryHandleJump();
}

// ─────────────────────────────────────────────────────────────
//  RepNotify — 클라이언트에서 상태 변경 수신 시
// ─────────────────────────────────────────────────────────────
void UPakousComponent::OnRep_ParkourState()
{
	if (ParkourState == EParkourState::ClimbingUp)
	{
		// 클라이언트에서 몽타주 재생 (서버는 TryClimbUp에서 이미 재생)
		if (AnimInstance && ClimbUpMontage)
			AnimInstance->Montage_Play(ClimbUpMontage);
	}
}

// ─────────────────────────────────────────────────────────────
//  DetectWall (서버 전용)
// ─────────────────────────────────────────────────────────────
void UPakousComponent::DetectWall()
{
	if (!OwnerChar || !CapsuleComp) return;

	const float Half     = CapsuleComp->GetScaledCapsuleHalfHeight();
	const float Radius   = CapsuleComp->GetScaledCapsuleRadius();
	const FVector Origin = OwnerChar->GetActorLocation();
	const FVector Forward = OwnerChar->GetActorForwardVector();

	const FVector WaistStart = Origin + FVector(0.f, 0.f, Half * -0.2f);
	const FVector WaistEnd   = WaistStart + Forward * 80.f;

	const FVector ChestStart = Origin + FVector(0.f, 0.f, Half * 0.3f);
	const FVector ChestEnd   = ChestStart + Forward * 80.f;

	const FVector EyeStart = Origin + FVector(0.f, 0.f, Half * 0.85f);
	const FVector EyeEnd   = EyeStart + Forward * 80.f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerChar);

	FHitResult WaistHit, ChestHit, EyeHit;
	const bool bWaistHit = GetWorld()->LineTraceSingleByChannel(WaistHit, WaistStart, WaistEnd, ECC_Visibility, Params);
	const bool bChestHit = GetWorld()->LineTraceSingleByChannel(ChestHit, ChestStart, ChestEnd, ECC_Visibility, Params);
	const bool bEyeHit   = GetWorld()->LineTraceSingleByChannel(EyeHit,   EyeStart,   EyeEnd,   ECC_Visibility, Params);

	bWallForward = bWaistHit || bChestHit;

	if (bWallForward)
	{
		const FHitResult& PrimaryHit = bChestHit ? ChestHit : WaistHit;
		WallNormal      = PrimaryHit.ImpactNormal;
		WallHitLocation = PrimaryHit.ImpactPoint;

		bWallTall = bEyeHit;

		FHitResult TopHit;
		const FVector TopSweepStart = WallHitLocation + FVector(0.f, 0.f, 500.f);
		const FVector TopSweepEnd   = WallHitLocation - FVector(0.f, 0.f, 10.f);
		bool bTopFound = GetWorld()->LineTraceSingleByChannel(TopHit, TopSweepStart, TopSweepEnd, ECC_Visibility, Params);
		if (bTopFound)
			WallHeight = TopHit.ImpactPoint.Z - Origin.Z + Half;
		else
			WallHeight = 0.f;
	}
	else
	{
		bWallTall = false;
		WallHeight = 0.f;
	}
}

bool UPakousComponent::CanEnterWallAttach() const
{
	if (!bWallForward) return false;
	if (bHasWallJumped) return false;
	if (WallJumpCooldownTimer > 0.f) return false;
	if (ExitCooldownTimer > 0.f) return false;
	return true;
}

// ─────────────────────────────────────────────────────────────
//  WallAttach
// ─────────────────────────────────────────────────────────────
void UPakousComponent::EnterWallAttach()
{
	if (!OwnerChar || !MoveComp) return;

	UE_LOG(LogTemp, Log, TEXT("[Parkour] EnterWallAttach"));

	SetParkourState(EParkourState::WallAttach);
	AttachTimer = 0.f;

	MoveComp->SetMovementMode(MOVE_Flying);
	MoveComp->GravityScale = 0.f;
	MoveComp->Velocity = FVector::ZeroVector;

	const float Radius = CapsuleComp->GetScaledCapsuleRadius();
	const FVector SafePos = WallHitLocation + WallNormal * (Radius + 2.f);
	const FVector SnapPos = FVector(SafePos.X, SafePos.Y, OwnerChar->GetActorLocation().Z);
	OwnerChar->SetActorLocation(SnapPos, true);

	const FRotator FaceWall = (-WallNormal).Rotation();
	OwnerChar->SetActorRotation(FRotator(0.f, FaceWall.Yaw, 0.f));
}

void UPakousComponent::TickWallAttach(float DeltaTime)
{
	AttachTimer += DeltaTime;

	if (MoveComp->IsMovingOnGround())
	{
		ExitClimb(false);
		return;
	}

	if (!bWallForward)
	{
		ExitClimb(false);
		return;
	}

	if (AttachTimer >= MaxAttachTime)
	{
		EnterWallSlide();
		return;
	}

	if (bClimbInputHeld)
	{
		EnterWallClimb();
	}
}

// ─────────────────────────────────────────────────────────────
//  WallClimb
// ─────────────────────────────────────────────────────────────
void UPakousComponent::EnterWallClimb()
{
	if (!OwnerChar || !MoveComp) return;

	UE_LOG(LogTemp, Log, TEXT("[Parkour] EnterWallClimb"));

	SetParkourState(EParkourState::WallClimb);
	ClimbTimer = 0.f;
}

void UPakousComponent::TickWallClimb(float DeltaTime)
{
	ClimbTimer += DeltaTime;

	if (!OwnerChar || !MoveComp) return;

	if (MoveComp->IsMovingOnGround())
	{
		ExitClimb(false);
		return;
	}

	if (!bWallForward)
	{
		ExitClimb(false);
		return;
	}

	if (ClimbTimer >= MaxClimbTime)
	{
		EnterWallSlide();
		return;
	}

	if (bClimbInputHeld && !bWallTall)
	{
		if (TryClimbUp())
			return;
	}

	const float t = FMath::Clamp(ClimbTimer / MaxClimbTime, 0.f, 1.f);
	const float CurrentSpeed = FMath::Lerp(ClimbSpeed, MinClimbSpeed, t);
	MoveComp->Velocity = FVector(0.f, 0.f, CurrentSpeed);
}

// ─────────────────────────────────────────────────────────────
//  WallSlide
// ─────────────────────────────────────────────────────────────
void UPakousComponent::EnterWallSlide()
{
	UE_LOG(LogTemp, Log, TEXT("[Parkour] EnterWallSlide"));
	SetParkourState(EParkourState::WallSlide);
}

void UPakousComponent::TickWallSlide(float DeltaTime)
{
	if (!OwnerChar || !MoveComp) return;

	if (MoveComp->IsMovingOnGround())
	{
		ExitClimb(false);
		return;
	}

	if (!bWallForward)
	{
		ExitClimb(false);
		return;
	}

	MoveComp->Velocity = FVector(0.f, 0.f, -SlideSpeed);
}

// ─────────────────────────────────────────────────────────────
//  ExitClimb (서버 전용)
// ─────────────────────────────────────────────────────────────
void UPakousComponent::ExitClimb(bool bJumpOff)
{
	if (!OwnerChar || !MoveComp) return;
	if (!GetOwner()->HasAuthority()) return;

	UE_LOG(LogTemp, Log, TEXT("[Parkour] ExitClimb bJumpOff=%d"), bJumpOff);

	SetParkourState(EParkourState::None);
	bClimbInputHeld = false;

	MoveComp->SetMovementMode(MOVE_Walking);
	MoveComp->GravityScale = 1.f;

	if (bJumpOff)
	{
		WallJumpCooldownTimer = WallJumpCooldown;
	}
	else
	{
		ExitCooldownTimer = ExitCooldown;
	}
}

// ─────────────────────────────────────────────────────────────
//  TriggerWallJump (서버 전용)
// ─────────────────────────────────────────────────────────────
void UPakousComponent::TriggerWallJump()
{
	if (!OwnerChar || !MoveComp) return;
	if (!GetOwner()->HasAuthority()) return;
	if (WallJumpCooldownTimer > 0.f) return;

	UE_LOG(LogTemp, Log, TEXT("[Parkour] TriggerWallJump"));

	bHasWallJumped = true;

	const FVector JumpDir = (WallNormal + FVector(0.f, 0.f, WallJumpUpBias)).GetSafeNormal();
	const FVector Impulse = JumpDir * WallJumpForce;

	ExitClimb(true);
	MoveComp->Launch(Impulse);
}

// ─────────────────────────────────────────────────────────────
//  TryParkour (서버 전용)
// ─────────────────────────────────────────────────────────────
bool UPakousComponent::TryParkour()
{
	if (!GetOwner()->HasAuthority()) return false;
	if (!bWallForward) return false;
	if (ExitCooldownTimer > 0.f) return false;
	if (MoveComp->IsMovingOnGround())
	{
		const float Speed = OwnerChar->GetVelocity().Size2D();
		if (Speed < 50.f) return false;

		EnterWallAttach();
		return true;
	}
	return false;
}

// ─────────────────────────────────────────────────────────────
//  TryClimbUp (서버 전용)
// ─────────────────────────────────────────────────────────────
bool UPakousComponent::TryClimbUp()
{
	if (!OwnerChar || !MoveComp || !CapsuleComp) return false;
	if (!GetOwner()->HasAuthority()) return false;

	if (ParkourState == EParkourState::WallSlide) return false;

	const float Half     = CapsuleComp->GetScaledCapsuleHalfHeight();
	const float Radius   = CapsuleComp->GetScaledCapsuleRadius();
	const FVector Origin = OwnerChar->GetActorLocation();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerChar);

	const FVector TopSearchStart = WallHitLocation + FVector(0.f, 0.f, 500.f);
	const FVector TopSearchEnd   = FVector(WallHitLocation.X, WallHitLocation.Y, Origin.Z - Half);
	FHitResult EdgeHit;
	if (!GetWorld()->LineTraceSingleByChannel(EdgeHit, TopSearchStart, TopSearchEnd, ECC_Visibility, Params))
		return false;

	const float OverOffset = Radius * 2.f + 30.f;
	const FVector OverEdge = EdgeHit.ImpactPoint + FVector(0.f, 0.f, 10.f) + (-WallNormal) * OverOffset;
	const FVector DropEnd  = OverEdge - FVector(0.f, 0.f, 500.f);
	FHitResult GroundHit;
	if (!GetWorld()->LineTraceSingleByChannel(GroundHit, OverEdge, DropEnd, ECC_Visibility, Params))
		return false;

	const FVector CandidatePos = GroundHit.ImpactPoint + FVector(0.f, 0.f, Half + 2.f);

	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Radius - 1.f, Half - 1.f);
	if (GetWorld()->OverlapBlockingTestByChannel(CandidatePos, FQuat::Identity, ECC_Pawn, CapsuleShape, Params))
		return false;

	// 복제될 변수에 할당 (클라이언트 lerp용)
	ClimbTargetPos = CandidatePos;

	UE_LOG(LogTemp, Log, TEXT("[Parkour] TryClimbUp target=%s"), *ClimbTargetPos.ToString());

	// 서버에서 몽타주 재생 (클라는 OnRep_ParkourState에서 재생)
	if (AnimInstance && ClimbUpMontage)
		AnimInstance->Montage_Play(ClimbUpMontage);

	EnterClimbingUp();
	return true;
}

// ─────────────────────────────────────────────────────────────
//  ClimbingUp (서버 전용)
// ─────────────────────────────────────────────────────────────
void UPakousComponent::EnterClimbingUp()
{
	if (!OwnerChar || !MoveComp) return;

	UE_LOG(LogTemp, Log, TEXT("[Parkour] EnterClimbingUp"));

	SetParkourState(EParkourState::ClimbingUp);
	ClimbUpTimer  = 0.f;
	ClimbStartPos = OwnerChar->GetActorLocation(); // 복제됨

	MoveComp->SetMovementMode(MOVE_Flying);
	MoveComp->GravityScale = 0.f;
	MoveComp->Velocity = FVector::ZeroVector;
}

void UPakousComponent::TickClimbingUp(float DeltaTime)
{
	if (!OwnerChar) return;

	ClimbUpTimer += DeltaTime;
	const float Alpha = FMath::Clamp(ClimbUpTimer / ClimbUpDuration, 0.f, 1.f);

	// L자 경로: Phase 0~0.5 수직 상승 → Phase 0.5~1 수평 이동
	const FVector TopPos = FVector(ClimbStartPos.X, ClimbStartPos.Y, ClimbTargetPos.Z);

	FVector NewPos;
	if (Alpha < 0.5f)
	{
		const float T = FMath::InterpEaseIn(0.f, 1.f, Alpha * 2.f, 2.f);
		NewPos = FMath::Lerp(ClimbStartPos, TopPos, T);
	}
	else
	{
		const float T = FMath::InterpEaseOut(0.f, 1.f, (Alpha - 0.5f) * 2.f, 2.f);
		NewPos = FMath::Lerp(TopPos, ClimbTargetPos, T);
	}

	OwnerChar->SetActorLocation(NewPos, false);

	if (Alpha >= 1.f)
	{
		OwnerChar->SetActorLocation(ClimbTargetPos, false);
		ExitClimb(false);
	}
}

// ─────────────────────────────────────────────────────────────
//  OnClimbUpEnd — 몽타주 종료 콜백
// ─────────────────────────────────────────────────────────────
void UPakousComponent::OnClimbUpEnd(UAnimMontage* Montage, bool bInterrupted)
{
	if (!ClimbUpMontage || Montage != ClimbUpMontage) return;
	// 위치 이동은 TickClimbingUp이 담당
}

// ─────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────
bool UPakousComponent::GetIsClimbing() const
{
	return ParkourState == EParkourState::WallClimb
		|| ParkourState == EParkourState::WallSlide
		|| ParkourState == EParkourState::ClimbingUp;
}

bool UPakousComponent::CanWallJump() const
{
	return ParkourState == EParkourState::WallAttach && WallJumpCooldownTimer <= 0.f;
}

void UPakousComponent::SetParkourState(EParkourState NewState)
{
	if (ParkourState == NewState) return;
	UE_LOG(LogTemp, Log, TEXT("[Parkour] State: %d -> %d"), (int32)ParkourState, (int32)NewState);
	ParkourState = NewState;
}
