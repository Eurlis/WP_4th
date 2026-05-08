// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ApexCharacterBase.h"
#include "Wraith.generated.h"

UCLASS()
class WP_4TH_API AWraith : public AApexCharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AWraith();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void ActivateUltimate() override;
	virtual void ActivateTactical() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(ReplicatedUsing = OnRep_IsInVoid, BlueprintReadOnly, Category="Skill")
	bool IsInVoid;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	UFUNCTION()
	void OnRep_IsInVoid();

	UFUNCTION(Server, Reliable)
	void Server_ActivateTactical();

	UFUNCTION(NetMulticast, Reliable)
	void Multcast_SetvoidState(bool bInVoid);

private:
	void EnterVoid();
	void ExitVoid();

	FTimerHandle TacticalDurationTimer;
	FTimerHandle TacticalCooldownTimer;
	bool bTacticalOnCooldown =false;

	float TacticalDuration = 3.f;
	float TacticalCooldown = 25.f;
};
