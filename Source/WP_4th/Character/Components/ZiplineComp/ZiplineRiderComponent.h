// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZiplineRiderComponent.generated.h"


class AZiplineActor;
class UCharacterMovementComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class WP_4TH_API UZiplineRiderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UZiplineRiderComponent();


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category ="Zipline")
	void TryInterract();

	void RequestAttach(AZiplineActor* Zipline);
	void RequestDetach(bool bJump);

	UFUNCTION(BlueprintCallable, Category ="Zipline")
	bool IsRidingZipline() const { return bIsRiding; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="Zipline")
	float ScanRadius = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="Zipline")
	float JumpExitSpeedMultiplier = 1.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category ="Zipline")
	float JumpExitUpForce = 350.f;

private:
	UPROPERTY(ReplicatedUsing=OnRep_IsRiding)
	bool bIsRiding = false;

	UPROPERTY(Replicated)
	AZiplineActor* CurrentZipline = nullptr;

	float CurrentSplineDistance = 0.f;
	int8 MoveDirection = 1;
	float CurrentSpeed = 0.f;

	ACharacter* OwnerChar = nullptr;
	UCharacterMovementComponent* MoveComp = nullptr;
	float SavedGravityScale = 1.f;

	UFUNCTION(Server, Reliable)
	void Server_Attach(AZiplineActor* Zipline, float StartDist, int8 Dir);
	UFUNCTION(Server, Reliable)
	void Server_Detach(bool bJump);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDetach(FVector LaunchVelocity);

	UFUNCTION()
	void OnRep_IsRiding();

	void TickZiplineMovement(float DeltaTime);

	void ApplyZiplinePhysics();
	void RestorePhysics();

	bool FindNearestZipline(AZiplineActor*& OutZipline, float& OutDist, int8& OutDir) const;

};
