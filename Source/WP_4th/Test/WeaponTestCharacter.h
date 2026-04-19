// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "WeaponTestCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UInputMappingContext;
class UInputAction;
class AWeaponBase;
class AThrowableBase;

UENUM()
enum class EEquippedSlot : uint8
{
	Weapon,
	Grenade
};

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

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	EEquippedSlot CurrentSlot = EEquippedSlot::Weapon;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TSubclassOf<AWeaponBase> ARClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TSubclassOf<AWeaponBase> PistolClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TSubclassOf<AWeaponBase> ShotgunClass;

	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	TSubclassOf<AThrowableBase> GrenadeClass;

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

	// ========== Weapon System Stubs ==========
	UFUNCTION()
	void ServerApplyDamage(float Damage, ACharacter* DamageInstigator, FHitResult HitResult);

	UFUNCTION(Client, Reliable)
	void ClientShowHitMarker(bool bIsHeadshot);

	FVector GetAimDirection() const;

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartFire();
	void StopFire();
	void Reload();
	void SwitchWeapon(TSubclassOf<AWeaponBase> NewWeaponClass);
	void SwitchToAR();
	void SwitchToPistol();
	void SwitchToShotgun();
	void SwitchToGrenade();
	void ThrowGrenade();
	void Turn(const FInputActionValue& Value);
	void LookUp(const FInputActionValue& Value);
};
