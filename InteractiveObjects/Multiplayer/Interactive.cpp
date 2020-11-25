// Fill out your copyright notice in the Description page of Project Settings.


#include "TEST_Interactive.h"

// Sets default values
ATEST_Interactive::ATEST_Interactive()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	ObjMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Interavtive Object"));
	ObjMesh->BodyInstance.SetCollisionProfileName("IgnoreOnlyPawn");
	ObjMesh->SetMobility(EComponentMobility::Movable);
	ObjMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ObjMesh->SetSimulatePhysics(true);

	MeshMaterial = CreateDefaultSubobject<UMaterialInterface>(TEXT("InteractiveMaterial"));
	ObjMesh->SetMaterial(0, MeshMaterial);
	RootComponent = ObjMesh;
}

void ATEST_Interactive::OnInteract()
{
}

// Take interacting character and sent to every player
void ATEST_Interactive::InteractBy(ATESTCharacter* Character)
{
	if (Role == ROLE_Authority)
	{
		InteractiveInstigator = Character;
		// Notify clients of the interaction
		ClientInteractBy(Character);
	}
}

void ATEST_Interactive::ClientInteractBy_Implementation(ATESTCharacter* Character)
{
	InteractiveInstigator = Character;
	OnInteract();
}
