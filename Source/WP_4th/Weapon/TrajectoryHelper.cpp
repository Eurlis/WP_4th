// Fill out your copyright notice in the Description page of Project Settings.

#include "TrajectoryHelper.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/Actor.h"

namespace TrajectoryHelper
{
	void UpdateSplineFromPath(
		USplineComponent* Spline,
		TArray<TObjectPtr<USplineMeshComponent>>& MeshPool,
		const TArray<FPredictProjectilePathPointData>& PathData,
		UStaticMesh* Mesh,
		UMaterialInterface* Material,
		AActor* Owner)
	{
		if (!Spline || !Owner) return;

		Spline->ClearSplinePoints(false);
		for (const FPredictProjectilePathPointData& Point : PathData)
		{
			Spline->AddSplinePoint(Point.Location, ESplineCoordinateSpace::World, false);
		}
		Spline->UpdateSpline();

		const int32 NumPoints = Spline->GetNumberOfSplinePoints();
		const int32 RequiredMeshes = FMath::Max(0, NumPoints - 1);

		while (MeshPool.Num() < RequiredMeshes)
		{
			USplineMeshComponent* NewMesh = NewObject<USplineMeshComponent>(
				Owner, USplineMeshComponent::StaticClass(), NAME_None, RF_Transient);
			NewMesh->SetMobility(EComponentMobility::Movable);
			NewMesh->SetForwardAxis(ESplineMeshAxis::X);
			NewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (Mesh)
			{
				NewMesh->SetStaticMesh(Mesh);
			}
			if (Material)
			{
				NewMesh->SetMaterial(0, Material);
			}
			NewMesh->AttachToComponent(Spline, FAttachmentTransformRules::KeepRelativeTransform);
			NewMesh->RegisterComponent();
			MeshPool.Add(NewMesh);
		}

		for (int32 i = 0; i < MeshPool.Num(); ++i)
		{
			USplineMeshComponent* MeshComp = MeshPool[i];
			if (!MeshComp) continue;

			if (i < RequiredMeshes)
			{
				const FVector StartPos = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
				const FVector StartTan = Spline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
				const FVector EndPos   = Spline->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::World);
				const FVector EndTan   = Spline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::World);

				MeshComp->SetStartAndEnd(StartPos, StartTan, EndPos, EndTan, true);
				MeshComp->SetVisibility(true);
			}
			else
			{
				MeshComp->SetVisibility(false);
			}
		}
	}

	void ClearTrajectory(
		USplineComponent* Spline,
		TArray<TObjectPtr<USplineMeshComponent>>& MeshPool)
	{
		if (Spline)
		{
			Spline->ClearSplinePoints(true);
		}
		for (TObjectPtr<USplineMeshComponent>& MeshComp : MeshPool)
		{
			if (MeshComp)
			{
				MeshComp->SetVisibility(false);
			}
		}
	}
}
