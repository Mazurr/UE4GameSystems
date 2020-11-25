// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TESTCharacter.h"
#include "TESTProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/InputSettings.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId
#include "DrawDebugHelpers.h"
#include "Blueprint/UserWidget.h"
#include "TextBlock.h"
#include "ProgressBar.h"
#include "WidgetTree.h"
#include "TEST_Pickup.h"
#include "TEST_Interactive.h"
#include "TESTGameMode.h"


DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// Shooting character with ability to pickup objects and store some in inventory
// Work online


ATESTCharacter::ATESTCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	Mesh1P->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("StaticMesh'/Game/Geometry/Meshes/1M_Cube.1M_Cube'"));
	MeshHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CharacterHeadMesh"));
	MeshHead->SetOwnerNoSee(true);
	MeshHead->SetupAttachment(FirstPersonCameraComponent);
	if (DefaultMesh.Succeeded()) 
	{
		MeshHead->SetStaticMesh(DefaultMesh.Object);
		MeshHead->bCastDynamicShadow = false;
		MeshHead->CastShadow = false;
		Mesh1P->RelativeLocation = FVector(-10.0f, 10.0f, -0.3f);
		Mesh1P->RelativeScale3D = FVector(0.3, 0.3, 0.3);
		Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
		// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.0f, 70.0f, 2.5f));
	
	// Set start values for players
	FireRate = 1.0f;
	bIsFiringWeapon = false;

	MaxHealth = 100;
	CurrentHealth = 50;
	CurrentAmmo = 10;

	bPickup = false;
	BackPackItem = nullptr;
	BackpackItemName = "Empty";
}

// Replicates variables
void ATESTCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATESTCharacter, MaxHealth);
	DOREPLIFETIME(ATESTCharacter, CurrentHealth);
	DOREPLIFETIME(ATESTCharacter, BackPackItem);
}

void ATESTCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));
}

//////////////////////////////////////////////////////////////////////////
// Input

void ATESTCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ATESTCharacter::StartFire);

	// Bind interaction event
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &ATESTCharacter::Interaction);
	PlayerInputComponent->BindAction("DropItem", IE_Released, this, &ATESTCharacter::DropItem);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ATESTCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATESTCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
}
// Blueprints Functions
int ATESTCharacter::GetAmmo()
{
	return CurrentAmmo;
}

int ATESTCharacter::GetHealth()
{
	return CurrentHealth;
}

int ATESTCharacter::GetMaxHealth()
{
	return MaxHealth;
}

FString ATESTCharacter::GetInteractionMessage()
{
	return InteractionMessage;
}

FString ATESTCharacter::GetBackpackItemName()
{
	return BackpackItemName;
}
//

void ATESTCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Get Camera Location and Forward Vector to cast ray
	FHitResult Hit;
	FVector Start = FirstPersonCameraComponent->GetComponentLocation();

	FVector ForwardVector = FirstPersonCameraComponent->GetForwardVector();
	FVector UpVector = FirstPersonCameraComponent->GetUpVector();
	// Forward Vector is multipled by lenght of ray
	FVector End = ((ForwardVector * 250.f) + Start);
	FCollisionQueryParams CollisionParams;
	// Ignore owner
	CollisionParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, CollisionParams))
	{
		// If ray block on object check if it is interactive object, then if
		// it is pickable and do proper actions
		if (Hit.GetActor()->GetClass()->IsChildOf(ATEST_Interactive::StaticClass()))
		{
			PointingItem = Cast<ATEST_Interactive>(Hit.GetActor());
			// Set item message to display
			InteractionMessage = FString(TEXT("Press F to ")) + PointingItem->message;
			if (Hit.GetActor()->GetClass()->IsChildOf(ATEST_Pickup::StaticClass()))
			{
				// If it is Pickable set flag and pass Actor to ItemHolder
				bPickup = true;
				ItemHolder = Hit.GetActor()->GetClass();
			}
		}
		else
		{
			// If it isn't interactive object, clear all
			// this prevents to store data if after pointing
			// ray will be block by non interactive object
			InteractionMessage = "";
			PointingItem = NULL;
			ItemHolder = nullptr;
			bPickup = false;
		}
	}
	else
	{
		// If it isn't object, clear all
		// this prevents to store data if after pointing
		// ray will be block by anything
		InteractionMessage = "";
		PointingItem = NULL;
		ItemHolder = nullptr;
		bPickup = false;
	}

}

void ATESTCharacter::StartFire()
{
	// Prevents too fast fire and if player have no ammo
	if (!bIsFiringWeapon && CurrentAmmo != 0) {
		bIsFiringWeapon = true;
		UWorld* World = GetWorld();
		// Set timer to next fire
		World->GetTimerManager().SetTimer(FiringTimer, this, &ATESTCharacter::StopFire, FireRate, false);
		OnFire();
		CurrentAmmo--;
		// try and play the sound if specified
		if (FireSound != NULL)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
		}

		// try and play a firing animation if specified
		if (FireAnimation != NULL)
		{
			// Get the animation object for the arms mesh
			UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
			if (AnimInstance != NULL)
			{
				AnimInstance->Montage_Play(FireAnimation, 1.0f);
			}
		}
	}
}

// Prevents too fast fire
void ATESTCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

// Server fire function
void ATESTCharacter::OnFire_Implementation()
{
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{	// Set spawn location, rotation and parameters to projectile fired by player
			FVector spawnLocation = FP_MuzzleLocation->GetComponentLocation();
			FRotator spawnRotation = GetControlRotation();

			FActorSpawnParameters spawnParameters;
			spawnParameters.Instigator = Instigator;
			spawnParameters.Owner = this;
			FCollisionQueryParams CollisionParams;

			// Spawn Projectile on all clients
			ATESTProjectile* spawnedProjectile = GetWorld()->SpawnActor<ATESTProjectile>(spawnLocation, spawnRotation, spawnParameters);
		}
	}
}

void ATESTCharacter::DropItem()
{
	// Drop item if is in "Backpack"
	if (BackPackItem != nullptr)
	{
		OnDropItem(BackPackItem);
		BackpackItemName = "Empty";
	}
}

// Pass item to server
void ATESTCharacter::ServerTakeItem_Implementation(TSubclassOf<ATEST_Interactive> Item)
{
	// If in Backpack is item drop it than take new
	if (BackPackItem != nullptr)
	{
		OnDropItem(BackPackItem);
	}
	BackPackItem = Item;
}

// Server function to spawn item after drop
void ATESTCharacter::OnDropItem_Implementation(TSubclassOf<ATEST_Interactive> Item)
{
	FVector spawnLocation = FP_MuzzleLocation->GetComponentLocation();
	FActorSpawnParameters spawnParameters;
	ATEST_Pickup* spawnItem = GetWorld()->SpawnActor<ATEST_Pickup>(Item, spawnLocation, FRotator::ZeroRotator, spawnParameters);
	BackPackItem = nullptr;
}



void ATESTCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ATESTCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ATESTCharacter::Interaction()
{
	// Handle item interaction and set name and 
	//pass it to server if is pickable
	if (bPickup)
	{
		BackpackItemName = PointingItem->Name;
		ServerTakeItem(ItemHolder);
	}
	ServerInteraction(PointingItem);
}

// Decrease Health after hit
float ATESTCharacter::TakeDamage(float DamageTaken, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	UpdateHealth(-DamageTaken);
	return DamageTaken;
}

// Server handling item interactions
void ATESTCharacter::ServerInteraction_Implementation(ATEST_Interactive* PItem)
{
	PointingItem = PItem;
	if (PointingItem && Role == ROLE_Authority)
	{
		PointingItem->InteractBy(this);
	}
}

void ATESTCharacter::UpdateHealth(int HealthChange)
{
		// Increase or decrease current health
		CurrentHealth += HealthChange;
		// Prevents to overlap health values
		if (CurrentHealth < 0) { CurrentHealth = 0; }
		if (CurrentHealth > MaxHealth) { CurrentHealth = 100; }
}