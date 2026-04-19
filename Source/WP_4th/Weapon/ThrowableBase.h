// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "WeaponData.h"
#include "ThrowableBase.generated.h"

class UProjectileMovementComponent;
class UDataTable;

UCLASS(Abstract)
class WP_4TH_API AThrowableBase : public AItemBase
{
	GENERATED_BODY()

public:
	AThrowableBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Throwable|Components")
	UProjectileMovementComponent* ProjectileMovement;

	// --- Data ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Data")
	UDataTable* WeaponDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_WeaponID, Category = "Throwable|Data")
	FName WeaponID;

	UPROPERTY(BlueprintReadOnly, Category = "Throwable|Data")
	FWeaponData CurrentWeaponData;

	// --- Stats ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Stats")
	float ThrowForce;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Stats")
	float FuseTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Stats")
	float ExplosionDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Throwable|Stats")
	float ExplosionRadius;

	// --- Functions ---
	UFUNCTION(BlueprintCallable, Category = "Throwable")
	void InitFromDataTable(FName InWeaponID);

	UFUNCTION()
	void OnRep_WeaponID();

	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerThrow(FVector ThrowDirection);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastExplosionEffects(FVector ExplosionLocation);

protected:
	FTimerHandle FuseTimerHandle;

	void Explode();
	void ApplyThrowableData(const FWeaponData& Data);
};
