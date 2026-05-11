// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ApexCharacterBase.h"
#include "GameFramework/Actor.h"
#include "OctaneLaunchPad.generated.h"

class UBoxComponent;
class USkeletalMeshComponent;

UCLASS()
class WP_4TH_API AOctaneLaunchPad : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AOctaneLaunchPad();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category= "Components")
	USkeletalMeshComponent* PadMesh;

	UPROPERTY(VisibleAnywhere, Category= "Components")
	UBoxComponent* TriggerBox;

	UPROPERTY(EditDefaultsOnly, Category= "LaunchPad")
	float StandingAngle = 75.f;

	UPROPERTY(EditDefaultsOnly, Category= "LaunchPad")
	float SlidingAngle = 20.f;

	UPROPERTY(EditDefaultsOnly, Category= "LaunchPad")
	float LaunchStrength = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category= "LaunchPad")
	float LifeTime = 30.f;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;


private:
	UFUNCTION()
	void OnTriggerOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	FTimerHandle LifeTimeHandle;
	TMap<AApexCharacterBase*, FTimerHandle> LaunchCooldowns;


};
