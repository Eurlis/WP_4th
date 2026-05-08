// Fill out your copyright notice in the Description page of Project Settings.

#include "ZiplineActor.h"

#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "CableComponent.h"
#include "GameFramework/Character.h"
#include "Character/Components/ZiplineComp/ZiplineRiderComponent.h"

void AZiplineActor::Interact(ACharacter* Interactor)
{
	if (!Interactor) return;

	UZiplineRiderComponent* RiderComp = Interactor->FindComponentByClass<UZiplineRiderComponent>();
	if (!RiderComp) return;

	if (RiderComp->IsRidingZipline())
		RiderComp->RequestDetach(true);
	else
		RiderComp->RequestAttach(this);
}

AZiplineActor::AZiplineActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// 루트
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;


	StartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("StartPoint"));
	StartPoint->SetupAttachment(Root);

	EndPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EndPoint"));
	EndPoint->SetupAttachment(Root);
	EndPoint->SetRelativeLocation(FVector(500.f, 0.f, 0.f)); // 기본 길이

	CableComp = CreateDefaultSubobject<UCableComponent>(TEXT("CableComp"));
	CableComp->SetupAttachment(StartPoint);
	CableComp->bAttachEnd  = false;
	CableComp->NumSegments = 16;
	CableComp->CableWidth  = 4.f;
	CableComp->CableLength = 500.f;

	StartPoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartPoleMesh"));
	StartPoleMesh->SetupAttachment(StartPoint);
	StartPoleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EndPoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EndPoleMesh"));
	EndPoleMesh->SetupAttachment(EndPoint);
	EndPoleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	StartInteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("StartInteractionZone"));
	StartInteractionZone->SetupAttachment(StartPoint);
	StartInteractionZone->SetSphereRadius(180.f);
	StartInteractionZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	EndInteractionZone = CreateDefaultSubobject<USphereComponent>(TEXT("EndInteractionZone"));
	EndInteractionZone->SetupAttachment(EndPoint);
	EndInteractionZone->SetSphereRadius(180.f);
	EndInteractionZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// 스플라인 — 라이더 이동 경로용 (OnConstruction에서 자동 업데이트)
	SplineComp = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComp"));
	SplineComp->SetupAttachment(Root);
}

void AZiplineActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FVector S = StartPoint->GetRelativeLocation();
	const FVector E = EndPoint->GetRelativeLocation();

	// EndPoint 월드 위치를 CableComp 로컬 공간으로 변환
	const FVector EndWorld  = EndPoint->GetComponentLocation();
	const FVector EndLocal  = CableComp->GetComponentTransform().InverseTransformPosition(EndWorld);
	CableComp->EndLocation  = EndLocal;
	CableComp->CableLength  = FVector::Dist(StartPoint->GetComponentLocation(), EndWorld);

	// 스플라인 실시간 업데이트
	SplineComp->SetLocationAtSplinePoint(0, S, ESplineCoordinateSpace::Local);
	SplineComp->SetLocationAtSplinePoint(1, E, ESplineCoordinateSpace::Local);
	SplineComp->UpdateSpline();

	StartInteractionZone->SetSphereRadius(InterationRadius);
	EndInteractionZone->SetSphereRadius(InterationRadius);
}

void AZiplineActor::BeginPlay()
{
	Super::BeginPlay();
}
