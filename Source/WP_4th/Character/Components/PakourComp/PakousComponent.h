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
	ClimbingUp
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
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Public API (ApexCharacterBase 호환) ---
	// 점프 입력 처리. 클라이언트에서 호출하면 내부적으로 Server RPC 전송
	bool TryHandleJump();
	bool TryParkour();
	bool TryClimbUp();
	void ExitClimb(bool bJumpOff);
	void TriggerWallJump();

	// 멀티플레이 안전한 클라이밍 입력 설정. 직접 필드 접근 대신 이 함수 사용
	void SetClimbInput(bool bHeld);

	UFUNCTION(BlueprintCallable)
	bool GetIsClimbing() const;

	UFUNCTION(BlueprintCallable)
	EParkourState GetParkourState() const { return ParkourState; }

	bool CanWallJump() const;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Animation")
	UAnimMontage* ClimbUpMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Parkour|Climb")
	float ClimbUpDuration = 0.45f;

private:
	// --- Server RPCs ---
	UFUNCTION(Server, Reliable)
	void ServerTryHandleJump();

	UFUNCTION(Server, Reliable)
	void ServerSetClimbInput(bool bHeld);

	// --- RepNotify ---
	UFUNCTION()
	void OnRep_ParkourState();

	// --- Replicated State ---
	UPROPERTY(ReplicatedUsing=OnRep_ParkourState)
	EParkourState ParkourState = EParkourState::None;

	UPROPERTY(Replicated)
	FVector ClimbStartPos = FVector::ZeroVector;

	UPROPERTY(Replicated)
	FVector ClimbTargetPos = FVector::ZeroVector;

	// --- Local State (서버 전용, 복제 불필요) ---
	FVector WallNormal = FVector::ZeroVector;
	FVector WallHitLocation = FVector::ZeroVector;

	bool bWallForward = false;
	bool bWallTall = false;
	float WallHeight = 0.f;

	bool bClimbInputHeld = false;

	float AttachTimer = 0.f;
	float ClimbTimer = 0.f;
	float WallJumpCooldownTimer = 0.f;
	float ExitCooldownTimer = 0.f;
	float ClimbUpTimer = 0.f;

	bool bHasWallJumped = false;

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

	bool FindClimbTarget(FVector& OutTarget);

	void SetParkourState(EParkourState NewState);

	UFUNCTION()
	void OnClimbUpEnd(UAnimMontage* Montage, bool bInterrupted);
};
