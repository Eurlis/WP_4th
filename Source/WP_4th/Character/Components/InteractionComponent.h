#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableChanged, AActor*, NewInteractable);

UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class WP_4TH_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float TraceDistance = 250.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// LineTrace 호출 간격 (10Hz 기본)
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float TraceInterval = 0.1f;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	AActor* CurrentInteractable = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FString CurrentPrompt;

	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractableChanged OnInteractableChanged;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	float TimeSinceLastTrace = 0.0f;

	void PerformTrace();
	bool GetViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
	void SetCurrentInteractable(AActor* NewInteractable);
};
