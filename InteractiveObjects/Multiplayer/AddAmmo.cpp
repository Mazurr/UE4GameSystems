// Fill out your copyright notice in the Description page of Project Settings.


#include "TEST_AddAmmo.h"

void ATEST_AddAmmo::OnInteract()
{
	// Add ammunition and destroy
	InteractiveInstigator->CurrentAmmo += AmmoValue;
	Destroy();
}
