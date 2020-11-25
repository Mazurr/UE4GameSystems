// Fill out your copyright notice in the Description page of Project Settings.


#include "TEST_AddHealth.h"

void ATEST_AddHealth::OnInteract()
{
	// Heal player and destroy self
	InteractiveInstigator->UpdateHealth(HealingValue);
	Destroy();
}