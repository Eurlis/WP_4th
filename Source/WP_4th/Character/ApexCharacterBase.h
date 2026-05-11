#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Components/PakourComp/PakousComponent.h"
#include "MotionWarping/Public/MotionWarping.h"
#include "Character/Components/ZiplineComp/ZiplineRiderComponent.h"
#include "Interaction/AmmoReserveOwnerInterface.h"
#include "Weapon/WeaponTypes.h"
#include "ApexCharacterBase.generated.h"

class UHealthComponent;
class UInputAction;
class USkeletalMeshComponent;
class UCameraComponent;
class AWeaponBase;
class UInteractionComponent;
class AThrowableBase;
class APickupBase;

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
UENUM(BlueprintType)
enum class EWeaponSlotType : uint8
{
	Main1     = 0 UMETA(DisplayName = "Main 1"),
	Main2     = 1 UMETA(DisplayName = "Main 2"),
	Pistol    = 2 UMETA(DisplayName = "Pistol"),
	Throwable = 3 UMETA(DisplayName = "Throwable")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReserveAmmoChangedSignature, EAmmoType, Type, int32, NewAmount);

UCLASS(abstract)
class WP_4TH_API AApexCharacterBase : public ACharacter, public IAmmoReserveOwnerInterface
{
	GENERATED_BODY()

	// ─── First Person Components ──────────────────────────────────
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

public:
	AApexCharacterBase();

	UFUNCTION()
	void OnRep_CurrentWeapon();
	void EquipWeapon(FName WeaponID);
	void SwitchWeaponByID(FName WeaponID);

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void AddGrenade(FName GrenadeID);

	bool TryAddGrenadeAuth(FName GrenadeID);

	// ─── Components ───────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Components")
	UPakousComponent* PakComp;
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	AWeaponBase* CurrentWeapon;
	UPROPERTY(EditAnywhere, Category= "Weapon")
	TSubclassOf<AWeaponBase> GenericWeaponClass;
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	EEquippedSlot CurrentSlot = EEquippedSlot::Weapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInteractionComponent* InteractionComp;

	// ─── Weapon Slots ─────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category = "Weapons")
	TSubclassOf<AThrowableBase> GenericThrowableClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Drop")
	TSubclassOf<APickupBase> PickupClass;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Slots")
	TArray<FName> WeaponSlots;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Slots")
	int32 ActiveSlotIndex = -1;

	UPROPERTY()
	FName LastWeaponID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grenade")
	int32 MaxGrenadeCount = 3;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Grenade")
	TArray<FGrenadeStockEntry> GrenadeStock;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Grenade")
	FName ActiveGrenadeID = NAME_None;

	// ─── Ammo Pool ────────────────────────────────────────────────
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ammo")
	int32 LightAmmo = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ammo")
	int32 HeavyAmmo = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ammo")
	int32 EnergyAmmo = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ammo")
	int32 ShotgunAmmo = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo|Max")
	int32 MaxLightAmmo = 240;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo|Max")
	int32 MaxHeavyAmmo = 240;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo|Max")
	int32 MaxEnergyAmmo = 240;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo|Max")
	int32 MaxShotgunAmmo = 64;

	UPROPERTY(BlueprintAssignable, Category = "Ammo|Events")
	FOnReserveAmmoChangedSignature OnReserveAmmoChanged;

	UFUNCTION(BlueprintPure, Category = "Ammo")
	int32 GetMaxAmmoForType(EAmmoType Type) const;

	UFUNCTION(BlueprintPure, Category = "Ammo")
	int32 GetAmmoForType(EAmmoType Type) const;

	void SetAmmoForType(EAmmoType Type, int32 NewAmount);

	// IAmmoReserveOwnerInterface
	virtual int32 GetReserveAmmo(EAmmoType Type) const override;
	virtual int32 AddAmmo(EAmmoType Type, int32 Count) override;
	virtual int32 ConsumeReserve(EAmmoType Type, int32 Needed) override;

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Ammo")
	void ServerAddAmmo(EAmmoType Type, int32 Count);

	// ─── Slot RPCs ────────────────────────────────────────────────
	UFUNCTION(Server, Reliable)
	void ServerSwitchToSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerAddWeaponToSlot(FName WeaponID);

	UFUNCTION(Server, Reliable)
	void ServerDropCurrentWeapon();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* TargetInteractable);

	// ─── BP Events ────────────────────────────────────────────────
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Events", meta = (DisplayName = "On Weapon Equipped"))
	void BP_OnWeaponEquipped(AWeaponBase* NewWeapon);

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

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TacticalAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* UltimateAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DropAction;

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

	// ─── Slot Switch Helpers ──────────────────────────────────────
	void SwitchToSlot0();
	void SwitchToSlot1();
	void SwitchToSlot2();
	void SwitchToSlot3();
	void OnDropPressed();

private:
	bool CanStartSlide() const;
	void BeginSlide();
	void EndSlide(bool bPlayExitPhase);
	void SetSlideAnimationPhaseState(ESlideAnimationPhase NewPhase);
	void ApplySlideMovementSettings();
	void RestoreDefaultMovementSettings();
	void TickSlide(float DeltaTime);

	FVector SlideDirection;
	float SlideSpeed;
	float SlideEnterEndTime;
	float SlideExitEndTime;
	float SlideUngroundedTime;
	float DefaultGroundFriction;
	float DefaultBrakingDecelerationWalking;
	float DefaultMaxWalkSpeedCrouched;
	float SavedDefaultWalkSpeed = 0.f;

	// ─── Slot Private Helpers ─────────────────────────────────────
	bool IsSlotEmpty(int32 SlotIndex) const;
	int32 FindNextAvailableSlot(int32 SkipIndex) const;
	EWeaponSlotType GetSlotForCategory(EWeaponType Category) const;
	void SwitchToSlot_Internal(int32 SlotIndex);
	void SwitchToFirstAvailableSlot();
	void SpawnPickupFromSlot(int32 SlotIndex);
	void ThrowGrenade();
	void StartThrowableAim();
	void StopThrowableAim();
	bool bIsAimingThrowable = false;
	float LastDropTime = -10.f;
	static constexpr float DropCooldown = 0.3f;

	// ─── Grenade Stock Helpers ────────────────────────────────────
	int32 GetGrenadeCountByID(FName GrenadeID) const;
	int32 GetTotalGrenadeCount() const;
	bool IsGrenadeStockFull(FName GrenadeID) const;
	TArray<FName> GetAvailableGrenadeIDs() const;
	FName GetNextGrenadeIDInCycle() const;
	bool AddGrenadeStock(FName GrenadeID, int32 Amount);
	bool RemoveGrenadeStock(FName GrenadeID, int32 Amount);

	// ─── Death ────────────────────────────────────────────────────
	UFUNCTION()
	void HandleDeath();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();

	// MontionWarping
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,Category="Components")
	UMotionWarpingComponent* MotionWarpingComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,Category="Components")
	UZiplineRiderComponent* ZiplineComp;

	UFUNCTION()
	void OnInteract();
	UFUNCTION(Server, Reliable)
	void Server_SetAiming(bool bAiming);
protected:
	virtual void ActivateTactical() {};
	virtual void ActivateUltimate() {};
};
