#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JunPawnDeathListener.generated.h"

class APawn;
class AJunDeathmatchGameMode;

UCLASS()
class WP_4TH_API UJunPawnDeathListener : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AJunDeathmatchGameMode* InOwnerGameMode, APawn* InObservedPawn);

	UFUNCTION()
	void HandleObservedDeath();

private:
	UPROPERTY()
	TObjectPtr<AJunDeathmatchGameMode> OwnerGameMode;

	UPROPERTY()
	TObjectPtr<APawn> ObservedPawn;
};
