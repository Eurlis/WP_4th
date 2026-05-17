#pragma once

#include "CoreMinimal.h"
#include "JunGame/JunDeathmatchGameState.h"
#include "ApexDeathmatchGameState.generated.h"

UCLASS(Blueprintable)
class WP_4TH_API AApexDeathmatchGameState : public AJunDeathmatchGameState
{
	GENERATED_BODY()

public:
	AApexDeathmatchGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match Timer")
	float MatchStartServerTime = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Match Timer")
	float MatchDurationReplicated = 300.0f;

	UFUNCTION(BlueprintPure, Category = "Match Timer")
	float GetRemainingMatchTime() const;

	UFUNCTION(BlueprintPure, Category = "Match Timer")
	bool IsMatchActive() const;
};
