// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireZone.generated.h"

class UBoxComponent;
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

	// 수평 확장 박스 (직사각형 화염)
	UPROPERTY(VisibleAnywhere, Category = "FireZone")
	TObjectPtr<UBoxComponent> DamageBox;

	UPROPERTY(VisibleAnywhere, Category = "FireZone")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireZone")
	float Duration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireZone")
	float TickInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireZone")
	float DamagePerTick = 20.0f;

	// 박스 반크기: X=앞뒤(짧음), Y=좌우(김), Z=위아래 (실제 크기 = 값 × 2)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FireZone")
	FVector BoxExtent = FVector(200.0f, 600.0f, 100.0f);

	UPROPERTY()
	AActor* DamageInstigator = nullptr;

	// 시전자 컨트롤러 캐시 (시전자 사망 후에도 킬 크레딧 유지)
	TWeakObjectPtr<AController> CachedInstigatorController;

	FTimerHandle DamageTimerHandle;
	FTimerHandle LifetimeTimerHandle;

	void ApplyTickDamage();
	void DestroyFireZone();
};
