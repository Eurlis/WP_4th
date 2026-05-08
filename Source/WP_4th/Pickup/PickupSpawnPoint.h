#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class UBillboardComponent;
class UArrowComponent;

UCLASS()
class WP_4TH_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APickupSpawnPoint();

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawn|Visualization")
	UBillboardComponent* Billboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawn|Visualization")
	UArrowComponent* Arrow;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bAllowWeapons = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bAllowThrowables = true;

	// 비어두면 카테고리 허용 플래그 사용. 채우면 그것만 후보로 사용 (필터)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TArray<FName> AllowedWeaponIDs;
};
