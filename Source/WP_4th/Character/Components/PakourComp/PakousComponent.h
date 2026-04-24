// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MotionWarping/Public/MotionWarpingComponent.h"
#include "PakousComponent.generated.h"

UENUM(BlueprintType)
enum class EParkourState : uint8
{
	None,
	WallAttach,
	WallClimb,
	WallSlide,
	ClimbingUp   // 벽 상단으로 실제 이동 중
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class WP_4TH_API UPakousComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPakousComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Public API (ApexCharacterBase 호환) ---
	bool TryHandleJump();
	bool TryParkour();
	bool TryClimbUp();
	void ExitClimb(bool bJumpOff);
	void TriggerWallJump();

	UFUNCTION(BlueprintCallable)
	bool GetIsClimbing() const;

	UFUNCTION(BlueprintCallable)
	EParkourState GetParkourState() const { return ParkourState; }

	bool CanWallJump() const;

	// ApexCharacterBase에서 Jump Hold 여부를 쓰기
	bool bClimbInputHeld = false;

	// --- Tuning Parameters ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Attach")
	float MaxAttachTime = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Climb")
	float ClimbSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Climb")
	float MinClimbSpeed = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Climb")
	float MaxClimbTime = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Slide")
	float SlideSpeed = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Jump")
	float WallJumpForce = 750.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Jump")
	float WallJumpUpBias = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Jump")
	float WallJumpCooldown = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Attach")
	float ExitCooldown = 0.3f;

	// ClimbUp 몽타주 (BP에서 할당, 없어도 동작함)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Animation")
	UAnimMontage* ClimbUpMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Climb")
	float ClimbUpDuration = 0.45f;

private:
	// --- Internal State ---
	EParkourState ParkourState = EParkourState::None;

	FVector WallNormal = FVector::ZeroVector;
	FVector WallHitLocation = FVector::ZeroVector;
	FVector ClimbTargetPos = FVector::ZeroVector;

	bool bWallForward = false;
	bool bWallTall = false;
	float WallHeight = 0.f;

	float AttachTimer = 0.f;
	float ClimbTimer = 0.f;
	float WallJumpCooldownTimer = 0.f;
	float ExitCooldownTimer = 0.f;
	float ClimbUpTimer = 0.f;

	FVector ClimbStartPos = FVector::ZeroVector;

	bool bHasWallJumped = false;  // 착지 전까지 1회만 허용

	// --- Cached References ---
	ACharacter* OwnerChar = nullptr;
	class UCharacterMovementComponent* MoveComp = nullptr;
	class UCapsuleComponent* CapsuleComp = nullptr;
	class USkeletalMeshComponent* MeshComp = nullptr;
	class UMotionWarpingComponent* WarpComp = nullptr;
	class UAnimInstance* AnimInstance = nullptr;

	// --- Internal Methods ---
	void DetectWall();
	bool CanEnterWallAttach() const;

	void EnterWallAttach();
	void TickWallAttach(float DeltaTime);

	void EnterWallClimb();
	void TickWallClimb(float DeltaTime);

	void EnterWallSlide();
	void TickWallSlide(float DeltaTime);

	void EnterClimbingUp();
	void TickClimbingUp(float DeltaTime);

	// 벽 에지 위 착지 지점 탐색 — 성공 시 OutTarget 설정 후 true 반환
	bool FindClimbTarget(FVector& OutTarget);

	void SetParkourState(EParkourState NewState);

	UFUNCTION()
	void OnClimbUpEnd(UAnimMontage* Montage, bool bInterrupted);
};
