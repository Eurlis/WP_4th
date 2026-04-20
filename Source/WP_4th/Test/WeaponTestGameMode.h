// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WeaponTestGameMode.generated.h"

class ABulletPoolManager;

UCLASS()
class WP_4TH_API AWeaponTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWeaponTestGameMode();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Pool")
	TSubclassOf<ABulletPoolManager> BulletPoolManagerClass;

	UPROPERTY()
	ABulletPoolManager* BulletPoolInstance;
};
