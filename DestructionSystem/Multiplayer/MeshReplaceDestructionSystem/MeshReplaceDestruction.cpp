// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshReplaceDestruction.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "TESTProjectile.h"

// Sets default values
ATEST_Destructable::ATEST_Destructable()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	// Base mesh which will store whole mesh
	SolidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMeshComp"));
	SolidMesh->SetMobility(EComponentMobility::Static);
	SolidMesh->OnComponentHit.AddDynamic(this, &ATEST_Destructable::OnHit);

	SolidMesh->SetCollisionObjectType(ECC_WorldDynamic);
	SolidMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SolidMesh->SetCollisionResponseToAllChannels(ECR_Block);
	SolidMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	SolidMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	SolidMesh->SetCanEverAffectNavigation(false);

	DamagedMaterial = CreateDefaultSubobject<UMaterialInterface>(TEXT("DamagedMaterial"));

	RootComponent = SolidMesh;
}

void ATEST_Destructable::ConfigurePartsOnStart()
{
	// Get all parts and set initial values
	for (UStaticMeshComponent* Comp : GetPartsComponents())
	{
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetMobility(EComponentMobility::Static);
		Comp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
		Comp->CanCharacterStepUpOn = ECB_No;
		Comp->SetHiddenInGame(true); // hide parts on start
	}
}

TArray<UStaticMeshComponent*> ATEST_Destructable::GetPartsComponents()
{
	// Function to find parts if array of components is empty
	if (PartsComponents.Num() == 0)
	{
		// Store all static mesh components 
		TInlineComponentArray<UStaticMeshComponent*> ComponentsByClass; 
		GetComponents(ComponentsByClass);

		// Store all static mesh components with "part" tag which are our parts
		TArray<UStaticMeshComponent*> Parts;
		Parts.Reserve(Parts.Num());
		for (UStaticMeshComponent* Component : ComponentsByClass)
		{
			if (Component->ComponentHasTag(TEXT("part")))
			{
				Parts.Push(Component);
			}
		}
		PartsComponents = Parts;
	}
	return PartsComponents;
}

void ATEST_Destructable::ShowParts(FVector DealerLocation)
{
	// Add impulse to throw away parts after destroy
	float ImpulseStrength = -500.f;
	FVector Impulse = (DealerLocation - GetActorLocation()).GetSafeNormal() * ImpulseStrength;
	for (UStaticMeshComponent* Comp : GetPartsComponents())
	{
		Comp->SetMobility(EComponentMobility::Movable);
		Comp->SetHiddenInGame(false);
		Comp->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		Comp->SetSimulatePhysics(true);
		Comp->DetachFromParent();
		Comp->AddImpulse(Impulse, NAME_None, true); // Fire impulse
	}
}

void ATEST_Destructable::Break(AActor* InBreakingActor)
{
	// If destroyed do nothing
	if (bDestroyed)
		return;

	bDestroyed = true;
	// Hide base mesh
	SolidMesh->SetHiddenInGame(true);
	SolidMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShowParts(InBreakingActor->GetActorLocation());
	// Destroy mesh after detach from parent
	SolidMesh->DestroyComponent();
	UWorld* World = GetWorld();
	World->GetTimerManager().SetTimer(DestroyTimer, this, &ATEST_Destructable::DestroyParts, DestroyTime, false);
}

void ATEST_Destructable::DestroyParts()
{
	this->Destroy();
}

void ATEST_Destructable::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Response only on projectile
	if (OtherActor->GetClass()->IsChildOf(ATESTProjectile::StaticClass()))
	{
		// Check if is damaged, depending on this state, call functions and play a sound
		if (bDamaged)
		{
			if (BreakSound != NULL)
			{
				UGameplayStatics::PlaySoundAtLocation(this, BreakSound, GetActorLocation());
			}
			FVector SpawnLocation = RootComponent->GetComponentLocation();
			FRotator SpawnRotation = RootComponent->GetComponentRotation();
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleEmitter, SpawnLocation, SpawnRotation);
			Break(OtherActor);
		}
		else 
		{
			if (DamageSound != NULL)
			{
				UGameplayStatics::PlaySoundAtLocation(this, DamageSound, GetActorLocation());
			}
			SolidMesh->SetMaterial(0, DamagedMaterial);
			bDamaged = true;
		}
	}
}

// Called when the game starts or when spawned
void ATEST_Destructable::BeginPlay()
{
	Super::BeginPlay();
	ConfigurePartsOnStart();
}