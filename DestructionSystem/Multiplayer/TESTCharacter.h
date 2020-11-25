// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "TESTCharacter.generated.h"

class UInputComponent;

UCLASS(config=Game)
class ATESTCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* Mesh1P;

	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class UStaticMeshComponent* MeshHead;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;

public:
	
	ATESTCharacter();
	
	// Required network setup
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay();

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class ATESTProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	class USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* FireAnimation;

	// To call in Blueprint and use on HUD
	UFUNCTION(BlueprintPure)
	int GetAmmo();

	// To call in Blueprint and use on HUD
	UFUNCTION(BlueprintPure)
	int GetHealth();

	// To call in Blueprint and use on HUD
	UFUNCTION(BlueprintPure)
	int GetMaxHealth();

	// To call in Blueprint and use on HUD
	UFUNCTION(BlueprintPure)
	FString GetInteractionMessage();

	// To call in Blueprint and use on HUD
	UFUNCTION(BlueprintPure)
	FString GetBackpackItemName();

	// Amount of character ammunition
	int CurrentAmmo;
protected:
	// Start fire on client to play sound and invoke 
	// to server function after press fire
	void StartFire();
	
	// Sets a flag that prohibits the player from shooting
	void StopFire();

	// Server spawn projectile after shoot
	UFUNCTION(Server, Reliable)
	void OnFire();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	// Set time to next fire
	float FireRate;

	// Flag that give posibility to shoot
	bool bIsFiringWeapon;
	FTimerHandle FiringTimer;

	// If player take damage reduce his health
	UFUNCTION(BlueprintCallable)
	float TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// Character starting and max health value 
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	int MaxHealth = 100;

private:
	// Character current health
	UPROPERTY(Replicated, VisibleAnywhere)
	int CurrentHealth = 50;

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface
public:
	/** Returns Mesh1P subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
	
	// Update player health level
	// HealtgChange this is the amout to change health by, can be + or -
	UFUNCTION(BlueprintCallable, BluePrintAuthorityOnly)
	void UpdateHealth(int HealthChange);
};

