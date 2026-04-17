#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "ApexCharacterBase.generated.h"

class UHealthComponent;
class UInputAction;
class USkeletalMeshComponent;
class UCameraComponent;

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

	// ─── Components ───────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

	// ─── Movement State ───────────────────────────────────────────
	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting, BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting;

	UPROPERTY(ReplicatedUsing = OnRep_IsSliding, BlueprintReadOnly, Category = "Movement")
	bool bIsSliding;

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

private:
	void TickSlide(float DeltaTime);

	// ─── Death ────────────────────────────────────────────────────
	UFUNCTION()
	void HandleDeath();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();
};
