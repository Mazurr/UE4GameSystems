// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TEST_Interactive.h"
#include "TEST_Pickup.generated.h"

/**
 * 
 */
UCLASS()
class TEST_API ATEST_Pickup : public ATEST_Interactive
{
	GENERATED_BODY()

private:
	// Do actions on every client like destroy
	// play sound or add particle
	void OnInteract() override;
};
