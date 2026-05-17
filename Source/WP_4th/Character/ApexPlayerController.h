// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WP_4thPlayerController.h"
#include "Components/HPComp/HealthComponent.h"
#include "GameFramework/PlayerController.h"
#include "ApexPlayerController.generated.h"

class UUserWidget;

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
	UFUNCTION(BlueprintImplementableEvent, Category="UI")
	void ShowHitMarker(EHitSoundType HitSoundType);

	UFUNCTION(Client, Reliable)
	void ClientShowMatchResult(APlayerState* WinnerPS, int32 WinnerKills);

	UFUNCTION(BlueprintImplementableEvent, Category="Match")
	void BP_OnShowMatchResult(APlayerState* WinnerPS, int32 WinnerKills);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|MatchTimer")
	TSubclassOf<UUserWidget> MatchTimerWidgetClass;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "UI|MatchTimer")
	TObjectPtr<UUserWidget> MatchTimerWidget;
};
