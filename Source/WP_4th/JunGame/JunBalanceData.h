#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "JunBalanceData.generated.h"

UENUM(BlueprintType)
enum class EJunRingCenterMode : uint8
{
	OwnerLocation,
	FixedLocation,
	KeepCurrent
};

USTRUCT(BlueprintType)
struct FJunRingPhaseRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	int32 PhaseIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring", meta = (ClampMin = "0.0"))
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float TargetRadius = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float WaitTime = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float DelayBeforeShrink = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float ShrinkTime = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float ShrinkDuration = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float DamageInterval = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float DamageTickInterval = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float DamagePerTick = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float DamagePerSecond = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	EJunRingCenterMode CenterMode = EJunRingCenterMode::KeepCurrent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	FVector FixedCenter = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	float WarningLeadTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	bool bUseStepDamageInterval = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ring")
	FString Notes;
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
