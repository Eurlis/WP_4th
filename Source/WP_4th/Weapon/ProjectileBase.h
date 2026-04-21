// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponData.h"
#include "ProjectileBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class ACharacter;
class AController;
class UPrimitiveComponent;

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

	// ========== Weapon Data (impact effects) ==========
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetWeaponData(const FWeaponData& InWeaponData);

protected:
	FTimerHandle LifeSpanTimerHandle;

	// 캐시된 WeaponData - OnHit에서 임팩트 이펙트 정보 사용
	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	FWeaponData CachedWeaponData;

	// 이펙트 브로드캐스트 RPC (모든 클라이언트 동기화)
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSpawnImpactEffects(FVector ImpactLocation, FVector ImpactNormal, UPrimitiveComponent* HitComp, bool bHitCharacter);
};
