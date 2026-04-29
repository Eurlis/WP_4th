// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Character/ApexCharacterBase.h"
#include "Kismet/GameplayStatics.h"
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



UCLASS()
class WP_4TH_API AWeaponTestCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AWeaponTestCharacter();

protected:
	virtual void BeginPlay() override;

public:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

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

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartFire();
	void StopFire();
	void Reload();
	void SwitchWeaponByID(FName WeaponID);
	void SwitchToAR();
	void SwitchToPistol();
	void SwitchToShotgun();
	void SwitchToGrenade();
	void ThrowGrenade();
	void Turn(const FInputActionValue& Value);
	void LookUp(const FInputActionValue& Value);
	void OnAimStarted();
	void OnAimStopped();

	// === WP4-37/38 ===
	void StartThrowableAim();
	void StopThrowableAim();
	void UpdateThrowableAimPreview();
	bool IsCurrentWeaponThrowable() const;

	float SavedDefaultWalkSpeed = 0.f;
};
