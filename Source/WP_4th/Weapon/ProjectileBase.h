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
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class UMaterialInterface;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	UNiagaraComponent* TracerComponent;

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
	// 임팩트 에셋들을 RPC 인자로 직접 전달 — 클라 투사체는 풀 한정으로 CachedWeaponData가 비어있을 수 있어 신뢰 불가.
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSpawnImpactEffects(
		FVector ImpactLocation,
		FVector ImpactNormal,
		UPrimitiveComponent* HitComp,
		bool bHitCharacter,
		UNiagaraSystem* ImpactFX,
		USoundBase* ImpactSound,
		UMaterialInterface* DecalMaterial,
		FVector DecalSize,
		float DecalLifeSpan,
		UNiagaraSystem* BloodFX,
		USoundBase* BloodSound
	);

	// 트레이서 활성화 브로드캐스트 — 클라 풀 인스턴스는 CachedWeaponData가 비어있으므로 서버가 에셋 포인터를 직접 전달.
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastActivateTracer(UNiagaraSystem* TracerFX);

	// 투사체 비활성화 시각 처리 브로드캐스트 — 서버 OnHit/수명만료 후 모든 클라이언트에서 잔류 총알 제거.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDeactivate();
};
