// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class AActor;

namespace TrajectoryHelper
{
	void UpdateSplineFromPath(
		USplineComponent* Spline,
		TArray<TObjectPtr<USplineMeshComponent>>& MeshPool,
		const TArray<FPredictProjectilePathPointData>& PathData,
		UStaticMesh* Mesh,
		UMaterialInterface* Material,
		AActor* Owner);

	void ClearTrajectory(
		USplineComponent* Spline,
		TArray<TObjectPtr<USplineMeshComponent>>& MeshPool);
}
