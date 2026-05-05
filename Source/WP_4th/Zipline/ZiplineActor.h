// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Zipline/IInteractionInterface.h"
#include "ZiplineActor.generated.h"

class USplineComponent;
class UStaticMeshComponent;
class USphereComponent;
class UCableComponent;

UCLASS()
class WP_4TH_API AZiplineActor : public AActor, public IIInteractionInterface
{
	GENERATED_BODY()

	virtual void Interact(ACharacter* Interactor) override;

public:
	AZiplineActor();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

public:
	// ── 위치 마커 (에디터에서 드래그) ─────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ZipLine")
	USceneComponent* StartPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ZipLine")
	USceneComponent* EndPoint;

	// ── 비주얼 ───────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ZipLine")
	UCableComponent* CableComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ZipLine")
	UStaticMeshComponent* StartPoleMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ZipLine")
	UStaticMeshComponent* EndPoleMesh;

	// ── 탑승 감지 ─────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ZipLine")
	USphereComponent* StartInteractionZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ZipLine")
	USphereComponent* EndInteractionZone;

	// ── 이동 경로 (StartPoint/EndPoint 기반 자동 업데이트) ────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ZipLine")
	USplineComponent* SplineComp;

	// ── 수치 ─────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ZipLine|Stats")
	float MaxRideSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ZipLine|Stats")
	float AccelerationRate = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ZipLine|Stats")
	float InterationRadius = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ZipLine|Stats")
	float RiderHeightOffset = 0.f;

	USplineComponent* GetSplineComp() const { return SplineComp; }
};
