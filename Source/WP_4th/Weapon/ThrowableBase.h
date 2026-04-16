// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "ThrowableBase.generated.h"

class UProjectileMovementComponent;

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
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerThrow(FVector ThrowDirection);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastExplosionEffects(FVector ExplosionLocation);

protected:
	FTimerHandle FuseTimerHandle;

	void Explode();
};
