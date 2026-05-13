// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WP_4thPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "ApexPlayerController.generated.h"

/**
 *
 */
UCLASS()
class WP_4TH_API AApexPlayerController : public AWP_4thPlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category="UI")
	void ShowEnemyHealthBar(AActor* EnemyActor, float HP, float MaxHp, float Shield, float MaxShield);
};
