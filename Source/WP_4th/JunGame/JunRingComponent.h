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
	Paused,
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

	UFUNCTION(BlueprintCallable, Category = "Ring|Balance")
	bool InitFromDataTable(UDataTable* InTable);

	UFUNCTION(BlueprintCallable, Category = "Ring|Balance")
	bool InitFromPhaseRows(const TArray<FJunRingPhaseRow>& InRows);

	UFUNCTION(BlueprintCallable, Category = "Ring")
	void StopRing();

	UFUNCTION(BlueprintCallable, Category = "Ring")
	void PauseRing();

	UFUNCTION(BlueprintCallable, Category = "Ring")
	void ResumeRing();

	UFUNCTION(BlueprintCallable, Category = "Ring")
	void ResetRing();

	UFUNCTION(BlueprintCallable, Category = "Ring")
	void ResetForRound();

	UFUNCTION(BlueprintCallable, Category = "Ring")
	bool AdvanceToPhase(int32 PhaseIndex);

	UFUNCTION(BlueprintCallable, Category = "Ring|Balance")
	bool ReloadRingData();

	UFUNCTION(BlueprintPure, Category = "Ring")
	float GetCurrentRadius() const { return CurrentRadius; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	float GetTargetRadius() const { return PhaseTargetRadius; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	int32 GetCurrentPhaseIndex() const { return CurrentPhaseIndex; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	bool HasRingStarted() const { return bRingStarted; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	bool IsRingShrinking() const { return bIsShrinking; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	bool IsRingPaused() const { return bIsPaused; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	bool IsRingActive() const { return bRingStarted && PhaseState != EJunRingPhaseState::Inactive; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	FVector GetRingCenter() const { return RingCenter; }

	UFUNCTION(BlueprintPure, Category = "Ring")
	FVector GetTargetRingCenter() const { return TargetRingCenter; }

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

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Ring")
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ring|Debug")
	bool bLogValidationDetails;

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
	bool bIsPaused;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	EJunRingPhaseState PhaseState;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	FVector TargetRingCenter;

private:
	void LoadRingPhasesFromDataTable();
	void NormalizeDefaultPhasesForInitialRadius();
	bool ValidateRingPhases(const TArray<FJunRingPhaseRow>& CandidatePhases, FString& OutReason) const;
	void BeginPhase(int32 PhaseIndex);
	void StartShrinkForCurrentPhase();
	void CompletePhase();
	void ApplyRingDamage();
	bool IsOutsideRing(const FVector& TargetLocation) const;
	void ClearRingTimers();
	void ApplyPhaseCenterPolicy(const FJunRingPhaseRow& Phase);
	void UpdateCurrentRadiusFromShrinkTime();
	float GetPhaseWaitTime(const FJunRingPhaseRow& Phase) const;
	float GetPhaseShrinkTime(const FJunRingPhaseRow& Phase) const;
	float GetPhaseDamageInterval(const FJunRingPhaseRow& Phase) const;
	float GetPhaseDamageAmount(const FJunRingPhaseRow& Phase) const;

	UFUNCTION()
	void OnRep_CurrentRadius();

	FTimerHandle PhaseStartTimerHandle;
	FTimerHandle PhaseEndTimerHandle;
	FTimerHandle DamageTickTimerHandle;

	float PhaseStartRadius;
	UPROPERTY(Replicated)
	float PhaseTargetRadius;
	float ShrinkStartTime;
	float ShrinkEndTime;
	float PhaseStateEndTime;
	float PausedPhaseTimeRemaining;
	float PausedShrinkTimeRemaining;
};
