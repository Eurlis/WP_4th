// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class ACharacter;
class AController;

UCLASS(Abstract)
class WP_4TH_API AProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AProjectileBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ========== Components ==========
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	USphereComponent* CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	UStaticMeshComponent* BulletMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	UProjectileMovementComponent* ProjectileMovement;

	// ========== Stats ==========
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Stats")
	float Damage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Stats")
	float BulletSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Stats")
	float GravityScale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Stats")
	float LifeSpan;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Stats")
	float HeadshotMultiplier;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Stats")
	float LegMultiplier;

	// ========== Owner ==========
	UPROPERTY()
	ACharacter* OwnerCharacter;

	UPROPERTY()
	AController* OwnerController;

	// ========== Pool State ==========
	UPROPERTY(BlueprintReadOnly, Category = "Projectile|Pool")
	bool bIsActive;

	// ========== Pool Interface ==========
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	virtual void Activate(FVector SpawnLocation, FVector Direction, float InDamage, float InSpeed, float InGravity, ACharacter* Shooter);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	virtual void Deactivate();

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
	FTimerHandle LifeSpanTimerHandle;
};
