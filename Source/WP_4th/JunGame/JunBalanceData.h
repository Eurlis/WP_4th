#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "JunBalanceData.generated.h"

USTRUCT(BlueprintType)
struct FJunRingPhaseRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	int32 PhaseIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float TargetRadius = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float WaitTime = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float ShrinkTime = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float DamageInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float DamagePerTick = 5.f;
};

USTRUCT(BlueprintType)
struct FJunDeathmatchSettingsRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deathmatch")
	float MatchStartDelay = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deathmatch")
	float PostMatchDelay = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deathmatch")
	int32 TargetKillCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deathmatch")
	float RespawnDelay = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deathmatch")
	bool bStartRingOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deathmatch")
	bool bAllowRespawn = true;
};
