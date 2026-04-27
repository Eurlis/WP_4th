// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemBase.h"
#include "WeaponData.h"
#include "ThrowableBase.generated.h"

class UProjectileMovementComponent;
class UDataTable;
class AFireZone;
class UNiagaraComponent;

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
	void MulticastExplosionEffects(FVector ExplosionLocation, FVector ThrowDir);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayThrowSound();

	// ArcStar 부착 이펙트 (부착 후 1초 뒤 호출)
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStickEffects(FVector StickLocation, AActor* StuckActor);

	// ===== Arc Star 부착 시스템 =====
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Throwable|Sticky")
	bool bIsStuck = false;

	UPROPERTY()
	AActor* StuckTarget = nullptr;

	// 시각적 회전 (표창 효과)
	bool bIsVisualSpinning = false;

	// 충돌 핸들러 (Arc Star 부착용)
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	                     UPrimitiveComponent* OtherComp,
	                     FVector NormalImpulse, const FHitResult& Hit);

	// 충돌 즉시 폭발 핸들러 (Thermite, bIsIncendiary=true)
	UFUNCTION()
	void OnImpactExplode(UPrimitiveComponent* HitComp, AActor* OtherActor,
	                     UPrimitiveComponent* OtherComp,
	                     FVector NormalImpulse, const FHitResult& Hit);

	// Arc Star 안정성용: ProjectileMovement 정지 이벤트 (OnComponentHit 보조)
	UFUNCTION()
	void OnProjectileStopped(const FHitResult& ImpactResult);

	virtual void Tick(float DeltaTime) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	FTimerHandle FuseTimerHandle;
	FTimerHandle MaxLifetimeHandle; // Arc Star 최후의 보루 (공중 정지 방지)
	FTimerHandle StickEffectTimerHandle; // Arc Star 부착 후 ShockFX 딜레이용

	// 부착 시 스폰된 ShockFX 컴포넌트 (폭발 시 정리)
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActiveShockFXComponent;

	void Explode();
	void ApplyThrowableData(const FWeaponData& Data);
	void StickToTarget(const FHitResult& Hit);
	void ForceExplode();
};
