#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/PakourComp/PakousComponent.h"
#include "MotionWarping/Public/MotionWarping.h"
#include "ApexCharacterBase.generated.h"

class UHealthComponent;
class UInputAction;
class USkeletalMeshComponent;
class UCameraComponent;
class AWeaponBase;

UENUM(BlueprintType)
enum class ESlideAnimationPhase : uint8
{
	None,
	Enter,
	Loop,
	Exit
};
UENUM()
enum class EEquippedSlot : uint8
{
	Weapon,
	Grenade
};
UCLASS(abstract)
class WP_4TH_API AApexCharacterBase : public ACharacter
{
	GENERATED_BODY()

	// ─── First Person Components ──────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

public:
	AApexCharacterBase();
	
	void EquipWeapon(FName WeaponID);
	
	// ─── Components ───────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Components")
	UPakousComponent* PakComp;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	AWeaponBase* CurrentWeapon;
	UPROPERTY(EditAnywhere, Category= "Weapon")
	TSubclassOf<AWeaponBase> GenericWeaponClass;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	EEquippedSlot CurrentSlot = EEquippedSlot::Weapon;
	// ─── Movement State ───────────────────────────────────────────
	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting, BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting;

	UPROPERTY(ReplicatedUsing = OnRep_IsSliding, BlueprintReadOnly, Category = "Movement")
	bool bIsSliding;

	UPROPERTY(ReplicatedUsing = OnRep_SlideAnimationPhase, BlueprintReadOnly, Category = "Movement")
	ESlideAnimationPhase SlideAnimationPhase;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float CrouchSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SlideMaxSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SlideMinSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SlopeAccelMultiplier;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SlideJumpSpeedMultiplier;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
	float SlideEnterDuration;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
	float SlideExitDuration;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
	float SlideFlatDeceleration;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
	float SlideUphillDeceleration;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
	float SlideDownhillAcceleration;

	UPROPERTY(EditDefaultsOnly, Category = "Movement|Slide")
	float SlideUngroundedGracePeriod;

	// ─── Input Actions ────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MouseLookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SlideAction;
	
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SwitchARAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SwitchPistolAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SwitchShotgunAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SwitchGrenadeAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AimAction;

	// ─── ADS ──────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category = "ADS")
	float DefaultFOV = 70.f;

	UPROPERTY(EditAnywhere, Category = "ADS")
	float ADSInterpSpeed = 12.f;

	UPROPERTY(EditAnywhere, Category = "ADS")
	float ADSWalkSpeedMultiplier = 0.6f;

	// ─── Getters ──────────────────────────────────────────────────
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	// ─── Damage ───────────────────────────────────────────────────
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ─── Base Input ───────────────────────────────────────────────
	void MoveInput(const FInputActionValue& Value);
	void LookInput(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoAim(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	virtual void DoJumpEnd();

	// ─── Weapon Actions ───────────────────────────────────────────
	void StartFire();
	void StopFire();
	void OnAimStarted();
	void OnAimStopped();
	void Reload();

	// ─── Sprint ───────────────────────────────────────────────────
	void StartSprint();
	void StopSprint();

	UFUNCTION(Server, Reliable)
	void Server_StartSprint();

	UFUNCTION(Server, Reliable)
	void Server_StopSprint();

	UFUNCTION()
	void OnRep_IsSprinting();

	// ─── Crouch ───────────────────────────────────────────────────
	void StartCrouch();
	void StopCrouch();

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	FVector DefaultFirstPersonMeshLocation;

	// ─── Slide ────────────────────────────────────────────────────
	void StartSlide();
	void StopSlide();

	UFUNCTION(Server, Reliable)
	void Server_StartSlide();

	UFUNCTION(Server, Reliable)
	void Server_StopSlide();

	UFUNCTION(Server, Reliable)
	void Server_SlideJump();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlaySlideAnim();

	UFUNCTION()
	void OnRep_IsSliding();

	UFUNCTION()
	void OnRep_SlideAnimationPhase();

private:
	bool CanStartSlide() const;
	void BeginSlide();
	void EndSlide(bool bPlayExitPhase);
	void SetSlideAnimationPhaseState(ESlideAnimationPhase NewPhase);
	void ApplySlideMovementSettings();
	void RestoreDefaultMovementSettings();
	void TickSlide(float DeltaTime);

	FVector SlideDirection;
	float SlideSpeed;           // slope 재투영 오차 방지용 별도 속도 트래킹
	float SlideEnterEndTime;
	float SlideExitEndTime;
	float SlideUngroundedTime;
	float DefaultGroundFriction;
	float DefaultBrakingDecelerationWalking;
	float DefaultMaxWalkSpeedCrouched;
	float SavedDefaultWalkSpeed = 0.f;

	
	
	// ─── Death ────────────────────────────────────────────────────
	UFUNCTION()
	void HandleDeath();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();
	
	// MontionWarping
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,Category="Components")
	UMotionWarpingComponent* MotionWarpingComp;
	
};
