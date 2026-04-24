// Fill out your copyright notice in the Description page of Project Settings.

#include "PakousComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "MotionWarping/Public/MotionWarpingComponent.h"

UPakousComponent::UPakousComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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

	if (WallJumpCooldownTimer > 0.f)
		WallJumpCooldownTimer -= DeltaTime;
	if (ExitCooldownTimer > 0.f)
		ExitCooldownTimer -= DeltaTime;

	DetectWall();

	switch (ParkourState)
	{
	case EParkourState::None:
		// 착지하면 벽점프 사용 여부 리셋
		if (MoveComp->IsMovingOnGround())
			bHasWallJumped = false;
		// 공중+벽 감지 → WallAttach 자동 진입
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
//  DetectWall
// ─────────────────────────────────────────────────────────────
void UPakousComponent::DetectWall()
{
	if (!OwnerChar || !CapsuleComp) return;

	const float Half     = CapsuleComp->GetScaledCapsuleHalfHeight();
	const float Radius   = CapsuleComp->GetScaledCapsuleRadius();
	const FVector Origin = OwnerChar->GetActorLocation();
	const FVector Forward = OwnerChar->GetActorForwardVector();

	// 허리 높이 트레이스 (Half * -0.2 ≈ 무릎~허리) — 낮은 벽 감지용
	const FVector WaistStart = Origin + FVector(0.f, 0.f, Half * -0.2f);
	const FVector WaistEnd   = WaistStart + Forward * 80.f;

	// 가슴 높이 트레이스
	const FVector ChestStart = Origin + FVector(0.f, 0.f, Half * 0.3f);
	const FVector ChestEnd   = ChestStart + Forward * 80.f;

	// 눈 높이 트레이스
	const FVector EyeStart = Origin + FVector(0.f, 0.f, Half * 0.85f);
	const FVector EyeEnd   = EyeStart + Forward * 80.f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerChar);

	FHitResult WaistHit, ChestHit, EyeHit;
	const bool bWaistHit  = GetWorld()->LineTraceSingleByChannel(WaistHit,  WaistStart, WaistEnd,  ECC_Visibility, Params);
	const bool bChestHit  = GetWorld()->LineTraceSingleByChannel(ChestHit,  ChestStart, ChestEnd,  ECC_Visibility, Params);
	const bool bEyeHit    = GetWorld()->LineTraceSingleByChannel(EyeHit,    EyeStart,   EyeEnd,    ECC_Visibility, Params);

#if WITH_EDITOR
	DrawDebugLine(GetWorld(), WaistStart, WaistEnd, bWaistHit ? FColor::Green : FColor::Yellow, false, -1.f, 0, 1.f);
	DrawDebugLine(GetWorld(), ChestStart, ChestEnd, bChestHit ? FColor::Green : FColor::Red,    false, -1.f, 0, 1.f);
	DrawDebugLine(GetWorld(), EyeStart,   EyeEnd,   bEyeHit   ? FColor::Green : FColor::Blue,   false, -1.f, 0, 1.f);
#endif

	// 허리 또는 가슴 트레이스가 닿으면 벽 감지
	bWallForward = bWaistHit || bChestHit;

	if (bWallForward)
	{
		// 우선순위: 가슴 > 허리
		const FHitResult& PrimaryHit = bChestHit ? ChestHit : WaistHit;
		WallNormal      = PrimaryHit.ImpactNormal;
		WallHitLocation = PrimaryHit.ImpactPoint;

		// 벽 높이 = 눈 트레이스가 통과하면 낮은 벽
		bWallTall  = bEyeHit;

		// 벽 상단까지의 높이 계산 (위쪽 스윕으로 에지 찾기)
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
	if (bHasWallJumped) return false;       // 착지 전 재사용 금지
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

	// 벽 표면에서 캡슐 반지름+2 만큼 떨어진 위치로 스냅 (벽 관통 방지)
	const float Radius = CapsuleComp->GetScaledCapsuleRadius();
	const FVector SafePos = WallHitLocation + WallNormal * (Radius + 2.f);
	const FVector SnapPos = FVector(SafePos.X, SafePos.Y, OwnerChar->GetActorLocation().Z);
	OwnerChar->SetActorLocation(SnapPos, true);

	// 벽을 향해 바라보기
	const FRotator FaceWall = (-WallNormal).Rotation();
	OwnerChar->SetActorRotation(FRotator(0.f, FaceWall.Yaw, 0.f));
}

void UPakousComponent::TickWallAttach(float DeltaTime)
{
	AttachTimer += DeltaTime;

	// 땅에 닿으면 해제
	if (MoveComp->IsMovingOnGround())
	{
		ExitClimb(false);
		return;
	}

	// 벽이 사라지면 해제
	if (!bWallForward)
	{
		ExitClimb(false);
		return;
	}

	// 타임아웃 → WallSlide
	if (AttachTimer >= MaxAttachTime)
	{
		EnterWallSlide();
		return;
	}

	// Jump Hold 입력 → WallClimb
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

	// 땅에 닿으면 해제
	if (MoveComp->IsMovingOnGround())
	{
		ExitClimb(false);
		return;
	}

	// 벽이 사라지면 해제
	if (!bWallForward)
	{
		ExitClimb(false);
		return;
	}

	// 타임아웃 → WallSlide
	if (ClimbTimer >= MaxClimbTime)
	{
		EnterWallSlide();
		return;
	}

	// 스페이스 홀드 중 눈 트레이스가 벽 위를 벗어나면 자동 올라서기
	if (bClimbInputHeld && !bWallTall)
	{
		if (TryClimbUp())
			return;
	}

	// 시간 경과에 따라 ClimbSpeed → MinClimbSpeed 선형 감속
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

	// 땅에 닿으면 해제
	if (MoveComp->IsMovingOnGround())
	{
		ExitClimb(false);
		return;
	}

	// 벽이 사라지면 해제
	if (!bWallForward)
	{
		ExitClimb(false);
		return;
	}

	// 느리게 내려가기
	MoveComp->Velocity = FVector(0.f, 0.f, -SlideSpeed);
}

// ─────────────────────────────────────────────────────────────
//  ExitClimb
// ─────────────────────────────────────────────────────────────
void UPakousComponent::ExitClimb(bool bJumpOff)
{
	if (!OwnerChar || !MoveComp) return;

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
//  TriggerWallJump — WallAttach에서 Jump Press 시 호출
// ─────────────────────────────────────────────────────────────
void UPakousComponent::TriggerWallJump()
{
	if (!OwnerChar || !MoveComp) return;
	if (WallJumpCooldownTimer > 0.f) return;

	UE_LOG(LogTemp, Log, TEXT("[Parkour] TriggerWallJump"));

	bHasWallJumped = true;

	const FVector JumpDir = (WallNormal + FVector(0.f, 0.f, WallJumpUpBias)).GetSafeNormal();
	const FVector Impulse = JumpDir * WallJumpForce;

	ExitClimb(true);
	MoveComp->Launch(Impulse);
}

// ─────────────────────────────────────────────────────────────
//  TryParkour — 지상에서 앞에 벽 감지 시 호출 (공중→벽은 Tick에서 자동)
// ─────────────────────────────────────────────────────────────
bool UPakousComponent::TryParkour()
{
	if (!bWallForward) return false;
	if (ExitCooldownTimer > 0.f) return false;
	if (MoveComp->IsMovingOnGround())
	{
		// 지상에서 달려가다가 벽 만남 → 바로 WallClimb
		const float Speed = OwnerChar->GetVelocity().Size2D();
		if (Speed < 50.f) return false;

		EnterWallAttach();
		return true;
	}
	return false;
}

// ─────────────────────────────────────────────────────────────
//  TryClimbUp — 벽 상단 착지 지점 연산 후 ClimbingUp 상태 진입
// ─────────────────────────────────────────────────────────────
bool UPakousComponent::TryClimbUp()
{
	if (!OwnerChar || !MoveComp || !CapsuleComp) return false;

	// WallSlide(타임아웃 후 탈진 상태)에서는 올라서기 불가
	if (ParkourState == EParkourState::WallSlide) return false;

	const float Half     = CapsuleComp->GetScaledCapsuleHalfHeight();
	const float Radius   = CapsuleComp->GetScaledCapsuleRadius();
	const FVector Origin = OwnerChar->GetActorLocation();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerChar);

	// ① 벽 에지(top) 탐색 — End를 발 아래까지 내려서 캐릭터가 벽 꼭대기 근처일 때도 포착
	const FVector TopSearchStart = WallHitLocation + FVector(0.f, 0.f, 500.f);
	const FVector TopSearchEnd   = FVector(WallHitLocation.X, WallHitLocation.Y, Origin.Z - Half);
	FHitResult EdgeHit;
	if (!GetWorld()->LineTraceSingleByChannel(EdgeHit, TopSearchStart, TopSearchEnd, ECC_Visibility, Params))
		return false;

	// ② 에지 위에서 캡슐 지름 + 여유만큼 벽 너머로 이동 후 낙하 탐색
	const float OverOffset = Radius * 2.f + 30.f;
	const FVector OverEdge = EdgeHit.ImpactPoint + FVector(0.f, 0.f, 10.f) + (-WallNormal) * OverOffset;
	const FVector DropEnd  = OverEdge - FVector(0.f, 0.f, 500.f);
	FHitResult GroundHit;
	if (!GetWorld()->LineTraceSingleByChannel(GroundHit, OverEdge, DropEnd, ECC_Visibility, Params))
		return false;

	// ③ 착지 후보 위치
	const FVector CandidatePos = GroundHit.ImpactPoint + FVector(0.f, 0.f, Half + 2.f);

	// ④ 캡슐 공간 검증 — 착지 지점에 실제로 들어갈 수 있는지 확인
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(Radius - 1.f, Half - 1.f);
	if (GetWorld()->OverlapBlockingTestByChannel(CandidatePos, FQuat::Identity, ECC_Pawn, CapsuleShape, Params))
		return false;

	ClimbTargetPos = CandidatePos;

#if WITH_EDITOR
	DrawDebugLine(GetWorld(), TopSearchStart, TopSearchEnd, FColor::Orange, false, 3.f);
	DrawDebugSphere(GetWorld(), EdgeHit.ImpactPoint, 8.f, 6, FColor::Yellow, false, 3.f);
	DrawDebugLine(GetWorld(), OverEdge, DropEnd, FColor::Magenta, false, 3.f);
	DrawDebugSphere(GetWorld(), ClimbTargetPos, 15.f, 8, FColor::Cyan, false, 3.f);
#endif

	UE_LOG(LogTemp, Log, TEXT("[Parkour] TryClimbUp target=%s"), *ClimbTargetPos.ToString());

	if (AnimInstance && ClimbUpMontage)
		AnimInstance->Montage_Play(ClimbUpMontage);

	EnterClimbingUp();
	return true;
}

// ─────────────────────────────────────────────────────────────
//  ClimbingUp — 코드 기반 위치 보간으로 착지 지점까지 이동
// ─────────────────────────────────────────────────────────────
void UPakousComponent::EnterClimbingUp()
{
	if (!OwnerChar || !MoveComp) return;

	UE_LOG(LogTemp, Log, TEXT("[Parkour] EnterClimbingUp"));

	SetParkourState(EParkourState::ClimbingUp);
	ClimbUpTimer  = 0.f;
	ClimbStartPos = OwnerChar->GetActorLocation();

	// Flying + 무중력 유지 (이미 설정돼 있지만 명시)
	MoveComp->SetMovementMode(MOVE_Flying);
	MoveComp->GravityScale = 0.f;
	MoveComp->Velocity = FVector::ZeroVector;
}

void UPakousComponent::TickClimbingUp(float DeltaTime)
{
	if (!OwnerChar) return;

	ClimbUpTimer += DeltaTime;
	const float Alpha = FMath::Clamp(ClimbUpTimer / ClimbUpDuration, 0.f, 1.f);

	// 벽을 통과하지 않도록 L자 경로:
	// Phase 0~0.5 : 현재 위치에서 수직 상승 (벽 에지 높이까지)
	// Phase 0.5~1 : 수평 이동 (벽 위로 넘어감)
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

	// 경로 자체가 벽을 우회하므로 Sweep 불필요
	OwnerChar->SetActorLocation(NewPos, false);

	if (Alpha >= 1.f)
	{
		OwnerChar->SetActorLocation(ClimbTargetPos, false);
		ExitClimb(false);
	}
}

// ─────────────────────────────────────────────────────────────
//  OnClimbUpEnd — 몽타주 종료 콜백 (위치는 TickClimbingUp이 보장)
// ─────────────────────────────────────────────────────────────
void UPakousComponent::OnClimbUpEnd(UAnimMontage* Montage, bool bInterrupted)
{
	if (!ClimbUpMontage || Montage != ClimbUpMontage) return;
	// 위치 이동은 TickClimbingUp이 담당하므로 여기선 아무것도 하지 않음
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
