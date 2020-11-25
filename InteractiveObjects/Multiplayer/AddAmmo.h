// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TEST_Interactive.h"
#include "TEST_AddAmmo.generated.h"

/**
 * 
 */
UCLASS()
class TEST_API ATEST_AddAmmo : public ATEST_Interactive
{
	GENERATED_BODY()

public:
	// Function inherited from ATEST_Interactive to 
	// do action on client side
	void OnInteract() override;

private:
	// Amount of ammo given to the player
	// Must be set in blueprint
	UPROPERTY(EditAnywhere)
	int AmmoValue;
};
