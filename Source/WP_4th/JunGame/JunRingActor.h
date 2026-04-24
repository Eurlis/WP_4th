#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JunGame/JunRingComponent.h"
#include "JunRingActor.generated.h"

class UJunRingComponent;

UCLASS()
class WP_4TH_API AJunRingActor : public AActor
{
	GENERATED_BODY()

public:
	AJunRingActor();

	UFUNCTION(BlueprintCallable, Category = "Ring")
	void StartRing();

	UFUNCTION(BlueprintPure, Category = "Ring")
	float GetCurrentRadius() const;

	UFUNCTION(BlueprintPure, Category = "Ring")
	int32 GetCurrentPhaseIndex() const;

	UFUNCTION(BlueprintPure, Category = "Ring")
	bool HasRingStarted() const;

	UFUNCTION(BlueprintPure, Category = "Ring")
	bool IsRingShrinking() const;

	UFUNCTION(BlueprintPure, Category = "Ring")
	FVector GetRingCenter() const;

	UFUNCTION(BlueprintPure, Category = "Ring")
	EJunRingPhaseState GetRingPhaseState() const;

	UFUNCTION(BlueprintPure, Category = "Ring")
	float GetPhaseTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Ring")
	UJunRingComponent* GetRingComponent() const { return RingComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ring", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UJunRingComponent> RingComponent;

public:
	FJunRingDamageAppliedSignature OnRingDamageApplied;
};
