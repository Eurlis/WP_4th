// Fill out your copyright notice in the Description page of Project Settings.


#include "ApexPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "WP_4th.h"

void AApexPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController() || !MatchTimerWidgetClass)
	{
		return;
	}

	MatchTimerWidget = CreateWidget<UUserWidget>(this, MatchTimerWidgetClass);
	if (MatchTimerWidget)
	{
		MatchTimerWidget->AddToViewport(0);
	}
	else
	{
		UE_LOG(LogWP_4th, Error, TEXT("[ApexPC] Failed to create MatchTimer widget."));
	}
}

void AApexPlayerController::ClientShowMatchResult_Implementation(APlayerState* WinnerPS, int32 WinnerKills)
{
	BP_OnShowMatchResult(WinnerPS, WinnerKills);
}
