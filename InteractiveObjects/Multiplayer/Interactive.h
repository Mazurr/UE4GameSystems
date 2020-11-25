// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "TESTCharacter.h"
#include "Engine/Canvas.h"
#include "TEST_Interactive.generated.h"

UCLASS(Abstract)
class TEST_API ATEST_Interactive : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATEST_Interactive();

	// Main mesh of object
	UPROPERTY(EditAnywhere)
	class UStaticMeshComponent* ObjMesh;

	class UMaterialInterface* MeshMaterial;

	// Message to display after pointing 
	UPROPERTY(EditAnywhere)
	FString message = FString(TEXT("Interactive"));

	// Name to display in invetory
	// This and three above must to be set in Blueprint
	UPROPERTY(EditAnywhere)
	FString Name;
	
	// Actions to do on every client
	virtual void OnInteract();

	// Actions to do on server side
	UFUNCTION(BlueprintAuthorityOnly, Category = "Interactive")
	virtual void InteractBy(ATESTCharacter* Character);

private:
	// Invoke OnInteract and assign character to InteractiveInstigator on every client
	UFUNCTION(NetMulticast, Reliable)
	void ClientInteractBy(ATESTCharacter* Character);
	
protected:
	// Character which interact with item
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interactive")
	ATESTCharacter* InteractiveInstigator;
	

};
