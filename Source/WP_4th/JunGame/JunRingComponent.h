#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "JunGame/JunBalanceData.h"
#include "JunRingComponent.generated.h"

class APawn;
class UDamageType;

UENUM(BlueprintType)
enum class EJunRingPhaseState : uint8
{
	Inactive,
	Waiting,
	Shrinking,
	Completed
};

DECLARE_MULTICAST_DELEGATE_OneParam(FJunRingDamageAppliedSignature, APawn*);

UCLASS(ClassGroup=(Jun), meta=(BlueprintSpawnableComponent))
class WP_4TH_API UJunRingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJunRingComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Ring")
	void StartRing();

	UFUNCTION(BlueprintPure, Category = "Ring")
	float GetCurrentRadius() const { return CurrentRadius; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	int32 GetCurrentPhaseIndex() const { return CurrentPhaseIndex; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	bool HasRingStarted() const { return bRingStarted; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	bool IsRingShrinking() const { return bIsShrinking; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	FVector GetRingCenter() const { return RingCenter; }

	UFUNCTION(BlueprintCallable, Category = "Ring")
	void SetRingCenter(const FVector& NewRingCenter);

	UFUNCTION(BlueprintPure, Category = "Ring")
	EJunRingPhaseState GetRingPhaseState() const { return PhaseState; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	float GetPhaseTimeRemaining() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring")
	bool bStartAutomatically;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring")
	bool bUseOwnerLocationAsCenter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring")
	FVector RingCenter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring", meta = (ClampMin = "0.0"))
	float InitialRadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring")
	TArray<FJunRingPhaseRow> RingPhases;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring|Balance")
	UDataTable* RingPhaseDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring|Debug")
	bool bEnableDebugDraw;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring|Debug")
	float DebugDrawDuration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring|Damage")
	TSubclassOf<UDamageType> RingDamageType;

	FJunRingDamageAppliedSignature OnRingDamageApplied;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentRadius, BlueprintReadOnly, Category = "Ring")
	float CurrentRadius;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	int32 CurrentPhaseIndex;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	bool bRingStarted;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	bool bIsShrinking;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	EJunRingPhaseState PhaseState;

private:
	void LoadRingPhasesFromDataTable();
	void BeginPhase(int32 PhaseIndex);
	void StartShrinkForCurrentPhase();
	void CompletePhase();
	void ApplyRingDamage();
	bool IsOutsideRing(const FVector& TargetLocation) const;

	UFUNCTION()
	void OnRep_CurrentRadius();

	FTimerHandle PhaseStartTimerHandle;
	FTimerHandle PhaseEndTimerHandle;
	FTimerHandle DamageTickTimerHandle;

	float PhaseStartRadius;
	float PhaseTargetRadius;
	float ShrinkStartTime;
	float ShrinkEndTime;
	float PhaseStateEndTime;
};
