// Fill out your copyright notice in the Description page of Project Settings.


#include "Wraith.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"


// Sets default values
AWraith::AWraith()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Asset 세팅
	ConstructorHelpers::FObjectFinder<USkeletalMesh> WraithMesh(TEXT("/Game/Models/Wraith/Wraith.Wraith"));
	if (WraithMesh.Succeeded())
	{
		FirstPersonMesh->SetSkeletalMesh(WraithMesh.Object);
		GetMesh()->SetSkeletalMesh(WraithMesh.Object);
	}

	// Transfrom 설정.. 터지지마세요.. FirstPersonMesh
	FirstPersonMesh->SetWorldLocationAndRotation(FVector(0.000000,0.000000,0.000000),FRotator(0.000000,0.000000,0.000000));
	FirstPersonMesh->SetRelativeScale3D(FVector(1.000000,1.000000,1.000000));

	//FirstPersonCameraComponent TransForm
	/*FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(12.520892,3.034245,-0.000011), FRotator(-20.000000,50.000000,-110.000000));
	FirstPersonCameraComponent->SetRelativeScale3D(FVector(1.000000,1.000000,1.000000));
	*/

	//Mesh TransForm
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.000000,0.000000,-100.000000), FRotator(0.000000,-90.000000,0.000000));
	GetMesh()->SetRelativeScale3D(FVector(1.000000,1.000000,1.000000));
	GetCapsuleComponent()->SetRelativeScale3D(FVector(1.f));

	ConstructorHelpers::FClassFinder<UAnimInstance> ABP_Wraith(TEXT("/Game/BluePrints/ABP/ABP_Wraith.ABP_Wraith_C"));
	if (ABP_Wraith.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(ABP_Wraith.Class);
	}

}

// Called when the game starts or when spawned
void AWraith::BeginPlay()
{
	Super::BeginPlay();
	EquipWeapon("R301");
}

// Called every frame
void AWraith::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AWraith::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

