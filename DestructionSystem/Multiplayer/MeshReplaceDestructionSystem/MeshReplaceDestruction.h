// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "TEST_Destructable.generated.h"

UCLASS()
class TEST_API ATEST_Destructable : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATEST_Destructable();

	// Main mesh
	UPROPERTY(EditAnywhere)
	class UStaticMeshComponent* SolidMesh;

	// Material to change after change state to Damaged
	UPROPERTY(EditAnywhere)
	class UMaterialInterface* DamagedMaterial;

	// Sound on change state to damaged
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class USoundBase* DamageSound;

	// Sound on break main mesh to parts
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class USoundBase* BreakSound;

	// Particles on break
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UParticleSystem* ParticleEmitter;
	
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	// Cofigure initial values for parts like
	// invisibility on start
	void ConfigurePartsOnStart();
	
	// Function to collect all SolidMesh childs with tag part,
	// Parts and main mesh are added in Blueprint
	TArray<UStaticMeshComponent*> GetPartsComponents();
	
	// Store all parts 
	TArray<UStaticMeshComponent*> PartsComponents;

	// Show parts and change value of collision and physics
	void ShowParts(FVector DealerLocation);
	
	// Init ShowParts, set timer to destroy parts, 
	// destroy main mesh after detach childrens
	void Break(AActor* InBreakingActor);

	// After some time destroy parts
	void DestroyParts();

private:
	// Flags to change state of object
	// States: Solid, Damaged, Broken
	bool bDamaged = false;
	bool bDestroyed = false;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	// Variables to set time to destroy parts
	FTimerHandle DestroyTimer;
	float DestroyTime = 10;
};
