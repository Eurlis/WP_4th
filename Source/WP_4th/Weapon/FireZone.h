// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireZone.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class WP_4TH_API AFireZone : public AActor
{
	GENERATED_BODY()

public:
	AFireZone();

	UFUNCTION(BlueprintCallable, Category = "FireZone")
	void InitializeFireZone(float InDuration, float InTickInterval,
	                        float InDamagePerTick, float InRadius,
	                        AActor* InInstigator);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "FireZone")
	USphereComponent* DamageSphere;

	UPROPERTY(VisibleAnywhere, Category = "FireZone")
	UStaticMeshComponent* VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireZone")
	float Duration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireZone")
	float TickInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireZone")
	float DamagePerTick = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireZone")
	float Radius = 400.0f;

	UPROPERTY()
	AActor* DamageInstigator = nullptr;

	// 시전자 컨트롤러 캐시 (시전자 사망 후에도 킬 크레딧 유지)
	TWeakObjectPtr<AController> CachedInstigatorController;

	FTimerHandle DamageTimerHandle;
	FTimerHandle LifetimeTimerHandle;

	void ApplyTickDamage();
	void DestroyFireZone();
};
