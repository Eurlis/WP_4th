// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletPoolManager.generated.h"

class AProjectileBase;

UCLASS()
class WP_4TH_API ABulletPoolManager : public AActor
{
	GENERATED_BODY()

public:
	ABulletPoolManager();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool")
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool")
	int32 PoolSize;

	UPROPERTY()
	TArray<AProjectileBase*> Pool;

	void InitializePool();

	UFUNCTION(BlueprintCallable, Category = "Pool")
	AProjectileBase* GetProjectile();

	UFUNCTION(BlueprintCallable, Category = "Pool")
	void ReturnProjectile(AProjectileBase* Projectile);

private:
	int32 GetActiveCount() const;
};
