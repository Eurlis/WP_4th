#pragma once

#include "CoreMinimal.h"
#include "JunGame/JunDeathmatchGameMode.h"
#include "ApexDeathmatchGameMode.generated.h"

class ABulletPoolManager;

UCLASS(Blueprintable)
class WP_4TH_API AApexDeathmatchGameMode : public AJunDeathmatchGameMode
{
	GENERATED_BODY()

public:
	AApexDeathmatchGameMode();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apex|BulletPool")
	TSubclassOf<ABulletPoolManager> BulletPoolManagerClass;

	UPROPERTY()
	TObjectPtr<ABulletPoolManager> SpawnedBulletPool;

private:
	void SpawnBulletPool();
};
