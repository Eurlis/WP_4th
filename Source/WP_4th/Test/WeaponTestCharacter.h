// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Character/ApexCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon/WeaponTypes.h"
#include "Interaction/AmmoReserveOwnerInterface.h"
#include "WeaponTestCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UInputMappingContext;
class UInputAction;
class AWeaponBase;
class AThrowableBase;
class USplineComponent;
class USplineMeshComponent;
class UDecalComponent;
class UStaticMesh;
class UMaterialInterface;
class UInteractionComponent;
class APickupBase;


UENUM(BlueprintType)
enum class EWeaponSlotType : uint8
{
    Main1     = 0 UMETA(DisplayName = "Main 1"),
    Main2     = 1 UMETA(DisplayName = "Main 2"),
    Pistol    = 2 UMETA(DisplayName = "Pistol"),
    Throwable = 3 UMETA(DisplayName = "Throwable")
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReserveAmmoChangedSignature, EAmmoType, Type, int32, NewAmount);

UCLASS()
class WP_4TH_API AWeaponTestCharacter : public ACharacter, public IAmmoReserveOwnerInterface
{
	GENERATED_BODY()

public:
	AWeaponTestCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ========== Components ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	USkeletalMeshComponent* Mesh1P;

	// ========== Weapons ==========
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	AWeaponBase* CurrentWeapon;

	/** BP에서 HUD 갱신 등에 사용하는 무기 장착 이벤트 (CurrentWeapon이 set된 후 호출) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Events", meta = (DisplayName = "On Weapon Equipped"))
	void BP_OnWeaponEquipped(AWeaponBase* NewWeapon);

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	EEquippedSlot CurrentSlot = EEquippedSlot::Weapon;

	// BP_Weapon_Generic (WeaponBase 상속 BP 하나)
	UPROPERTY(EditAnywhere, Category = "Weapons")
	TSubclassOf<AWeaponBase> GenericWeaponClass;

	// BP_Throwable_Generic (ThrowableBase 상속 BP 하나)
	UPROPERTY(EditAnywhere, Category = "Weapons")
	TSubclassOf<AThrowableBase> GenericThrowableClass;

	// DataTable Row 이름
	UPROPERTY(EditAnywhere, Category = "Weapons")
	FName ARWeaponID = "R301";

	UPROPERTY(EditAnywhere, Category = "Weapons")
	FName PistolWeaponID = "Wingman";

	UPROPERTY(EditAnywhere, Category = "Weapons")
	FName ShotgunWeaponID = "Peacekeeper";

	UPROPERTY(EditAnywhere, Category = "Weapons")
	FName GrenadeWeaponID = "FragGrenade";

	// 마지막 장착 무기 (수류탄 후 복귀용)
	UPROPERTY()
	FName LastWeaponID = "R301";

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	int32 GrenadeCount = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapons")
	int32 MaxGrenadeCount = 3;

	// ========== Input Actions ==========
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* JumpAction;

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
	UInputAction* TurnAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookUpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* AimAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* DropAction;

	// === Interaction ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UInteractionComponent* InteractionComp;

	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* TargetInteractable);

	// ========== Weapon Slots ==========
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Slots")
	TArray<FName> WeaponSlots;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon|Slots")
	int32 ActiveSlotIndex = -1;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon|Drop")
	TSubclassOf<class APickupBase> PickupClass;

	float LastDropTime = -10.f;
	static constexpr float DropCooldown = 0.3f;

public:
	void SwitchWeaponByID(FName WeaponID);

	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void AddGrenade(FName GrenadeID);

	// ========== Ammo Pool (4 종 개별 — TMap 복제 미지원 회피) ==========
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

	// ========== ADS / Camera ==========
	UPROPERTY(EditAnywhere, Category = "Camera")
	float DefaultFOV = 90.0f;

	UPROPERTY(EditAnywhere, Category = "Camera")
	float ADSInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float ADSWalkSpeedMultiplier = 0.6f;

	// ========== WP4-37/38: 수류탄 조준 시스템 ==========
	UPROPERTY(VisibleAnywhere, Category = "Throwable|Aim")
	TObjectPtr<USplineComponent> TrajectorySpline;

	UPROPERTY(VisibleAnywhere, Category = "Throwable|Aim")
	TObjectPtr<UDecalComponent> TargetMarkerDecal;

	UPROPERTY(EditDefaultsOnly, Category = "Throwable|Aim")
	TObjectPtr<UStaticMesh> TrajectorySplineMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Throwable|Aim")
	TObjectPtr<UMaterialInterface> TrajectoryMeshMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Throwable|Aim")
	TObjectPtr<UMaterialInterface> TargetMarkerMaterial;

	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> TrajectoryMeshes;

	UPROPERTY(EditDefaultsOnly, Category = "Throwable|Aim")
	float MaxTrajectorySimTime = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Throwable|Aim")
	float TrajectoryProjectileRadius = 5.0f;

	bool bIsAimingThrowable = false;
	FPredictProjectilePathResult CachedTrajectoryResult;

	// ========== Weapon System Stubs ==========
	UFUNCTION()
	void ServerApplyDamage(float Damage, ACharacter* DamageInstigator, FHitResult HitResult);

	UFUNCTION(Client, Reliable)
	void ClientShowHitMarker(bool bIsHeadshot);

	FVector GetAimDirection() const;

	virtual void Tick(float DeltaTime) override;

	// ========== Slot RPC ==========
	UFUNCTION(Server, Reliable) void ServerSwitchToSlot(int32 SlotIndex);
	UFUNCTION(Server, Reliable) void ServerAddWeaponToSlot(FName WeaponID);
	UFUNCTION(Server, Reliable) void ServerDropCurrentWeapon();

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartFire();
	void StopFire();
	void Reload();
	void SwitchToAR();
	void SwitchToPistol();
	void SwitchToShotgun();
	void SwitchToGrenade();
	void ThrowGrenade();
	void Turn(const FInputActionValue& Value);
	void LookUp(const FInputActionValue& Value);
	void OnAimStarted();
	void OnAimStopped();
	void OnInteractInput(const FInputActionValue& Value);
	void OnDropPressed();

	// ========== Slot Helpers ==========
	bool IsSlotEmpty(int32 SlotIndex) const;
	int32 FindNextAvailableSlot(int32 SkipIndex) const;
	EWeaponSlotType GetSlotForCategory(EWeaponType Category) const;
	void SwitchToSlot_Internal(int32 SlotIndex);
	void SpawnPickupFromSlot(int32 SlotIndex);

	// === WP4-37/38 ===
	void StartThrowableAim();
	void StopThrowableAim();
	void UpdateThrowableAimPreview();
	bool IsCurrentWeaponThrowable() const;

	float SavedDefaultWalkSpeed = 0.f;
};
