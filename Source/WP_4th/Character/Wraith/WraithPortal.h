// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "GameFramework/Character.h"
#include "WraithPortal.generated.h"

class USplineComponent;
class USphereComponent;
UCLASS()
class WP_4TH_API AWraithPortal : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWraithPortal();

	void SetPathAndLink(const TArray<FVector>& Path, AWraithPortal* Other);
	void AddCooldown(AActor* Actor, float Duration);
	UPROPERTY(VisibleAnywhere) USphereComponent* TriggerSphere;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal")
	UNiagaraComponent* PortalVFX;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) USplineComponent* PortalSpline;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
private:
	UPROPERTY() AWraithPortal* LinkedPortal;

	UPROPERTY() ACharacter* TravelingChar = nullptr;
	float TravelDistance = 0.f;
	float TravelSpeed = 1000.f;
	bool bIsTraveling = false;
	TSet<AActor*> RecentlyTeleported;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,int32 OtherBodyIndex, bool bFromSweep,const FHitResult& SweepResult);

	void ClearCooldown(AActor* Actor);
	void StartTraversal(ACharacter* Char);
	void FinishTraversal();

};
