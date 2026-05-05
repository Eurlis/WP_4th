#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnManager.generated.h"

class UDataTable;
class APickupBase;

UCLASS()
class WP_4TH_API APickupSpawnManager : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnManager();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	UDataTable* WeaponDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup")
	TSubclassOf<APickupBase> PickupClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	float SpawnZOffset = 50.0f;

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void SpawnAllPickups();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void ClearAllPickups();

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TArray<APickupBase*> SpawnedPickups;
};
