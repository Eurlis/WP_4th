// Fill out your copyright notice in the Description page of Project Settings.


#include "Octane.h"


// Sets default values
AOctane::AOctane()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ConstructorHelpers::FObjectFinder<USkeletalMesh> OctaneMesh(TEXT("/Game/Models/Octane/Octane/ocatane.ocatane"));
	if (OctaneMesh.Succeeded())ㅈ
	{
		FirstPersonMesh->SetSkeletalMesh(OctaneMesh.Object);
		GetMesh()->SetSkeletalMesh(OctaneMesh.Object);
	}

	// 2. 메시 Transform (Wraith랑 동일하게 시작)
	GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -100.f), FRotator(0, -90.f, 0));

	// 3. ABP 연결
	ConstructorHelpers::FClassFinder<UAnimInstance> ABP_Octane(TEXT("/Game/BluePrints/ABP/ABP_Octane.ABP_Octane"));
	if (ABP_Octane.Succeeded())
		GetMesh()->SetAnimInstanceClass(ABP_Octane.Class);

}

// Called when the game starts or when spawned
void AOctane::BeginPlay()
{
	Super::BeginPlay();

}

void AOctane::ActivateTactical()
{
	Super::ActivateTactical();
}

void AOctane::ActivateUltimate()
{
	Super::ActivateUltimate();
}

// Called every frame
void AOctane::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AOctane::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

