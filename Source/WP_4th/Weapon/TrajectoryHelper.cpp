// Fill out your copyright notice in the Description page of Project Settings.

#include "TrajectoryHelper.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

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

		// === [TrajDebug] 임시 디버그 로그 (스팸 방지: 30프레임마다 1회) ===
		static int32 DebugCallCount = 0;
		const bool bShouldLog = (DebugCallCount++ % 30 == 0);
		if (bShouldLog)
		{
			if (Material)
			{
				UE_LOG(LogTemp, Warning, TEXT("[TrajDebug] Material param OK: %s"), *Material->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[TrajDebug] Material param is NULLPTR!"));
			}
			UE_LOG(LogTemp, Warning, TEXT("[TrajDebug] Mesh param: %s, MeshPool.Num=%d"),
				Mesh ? *Mesh->GetName() : TEXT("nullptr"), MeshPool.Num());
		}

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

				// === [TrajDebug] 임시: 다이내믹 머티리얼 인스턴스로 강제 빨강 ===
				UMaterialInstanceDynamic* MID = NewMesh->CreateAndSetMaterialInstanceDynamic(0);
				if (MID)
				{
					MID->SetVectorParameterValue(FName("BaseColor"), FLinearColor::Red);
					MID->SetVectorParameterValue(FName("Emissive"), FLinearColor::Red * 5);
				}

				// === [TrajDebug] 첫 메시 생성 시 적용 결과 확인 ===
				if (bShouldLog && MeshPool.Num() == 0)
				{
					UMaterialInterface* AppliedMat = NewMesh->GetMaterial(0);
					UE_LOG(LogTemp, Warning, TEXT("[TrajDebug] NEW Mesh - Applied: %s, SlotCount: %d, StaticMeshSlots: %d, MID: %d"),
						AppliedMat ? *AppliedMat->GetName() : TEXT("nullptr"),
						NewMesh->GetNumMaterials(),
						Mesh ? Mesh->GetStaticMaterials().Num() : -1,
						MID ? 1 : 0);
				}
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
				// === [TrajDebug] 첫 메시 재사용 상태 확인 ===
				if (bShouldLog && i == 0)
				{
					UMaterialInterface* CurMat = MeshComp->GetMaterial(0);
					UE_LOG(LogTemp, Warning, TEXT("[TrajDebug-Reuse] i=0 Current: %s, ParamMat: %s, Match: %d"),
						CurMat ? *CurMat->GetName() : TEXT("nullptr"),
						Material ? *Material->GetName() : TEXT("nullptr"),
						(CurMat == Material) ? 1 : 0);
				}

				// 풀 재사용: MID가 없으면 재적용 + MID 강제 빨강
				if (Material && Cast<UMaterialInstanceDynamic>(MeshComp->GetMaterial(0)) == nullptr)
				{
					MeshComp->SetMaterial(0, Material);

					// === [TrajDebug] 임시: MID 강제 빨강 ===
					UMaterialInstanceDynamic* ReuseMID = MeshComp->CreateAndSetMaterialInstanceDynamic(0);
					if (ReuseMID)
					{
						ReuseMID->SetVectorParameterValue(FName("BaseColor"), FLinearColor::Red);
						ReuseMID->SetVectorParameterValue(FName("Emissive"), FLinearColor::Red * 5);
					}

					if (bShouldLog && i == 0)
					{
						UE_LOG(LogTemp, Warning, TEXT("[TrajDebug-Reuse] i=0 Re-applied material + MID red, MID=%d"), ReuseMID ? 1 : 0);
					}
				}

				const FVector StartPos = Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
				const FVector StartTan = Spline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World);
				const FVector EndPos   = Spline->GetLocationAtSplinePoint(i + 1, ESplineCoordinateSpace::World);
				const FVector EndTan   = Spline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::World);

				// World → SplineMesh Local 변환 (SetStartAndEnd는 Local 좌표 기대)
				const FTransform MeshXform = MeshComp->GetComponentTransform();
				const FVector LocalStart    = MeshXform.InverseTransformPosition(StartPos);
				const FVector LocalStartTan = MeshXform.InverseTransformVector(StartTan);
				const FVector LocalEnd      = MeshXform.InverseTransformPosition(EndPos);
				const FVector LocalEndTan   = MeshXform.InverseTransformVector(EndTan);

				MeshComp->SetStartAndEnd(LocalStart, LocalStartTan, LocalEnd, LocalEndTan, true);
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
