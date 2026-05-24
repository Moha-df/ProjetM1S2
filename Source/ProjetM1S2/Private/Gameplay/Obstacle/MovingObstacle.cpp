// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Obstacle/MovingObstacle.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// Sets default values
AMovingObstacle::AMovingObstacle()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	bReplicates = true;
	SetReplicatingMovement(false);
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	
	Mesh->SetMobility(EComponentMobility::Movable);
	RootComponent->SetMobility(EComponentMobility::Movable);
}

void AMovingObstacle::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
	DOREPLIFETIME(AMovingObstacle, ReplicatedStartLocation);
	DOREPLIFETIME(AMovingObstacle, ReplicatedDirection);
	DOREPLIFETIME(AMovingObstacle, ServerStartTime);
}

void AMovingObstacle::OnRep_ServerStartTime()
{
	// À ce stade, ReplicatedStartLocation et ReplicatedDirection sont
	// quasi certainement déjà arrivés (envoyés dans le même bunch initial),
	// mais on se protège quand même.
	if (!ReplicatedStartLocation.IsZero() || !ReplicatedDirection.IsZero())
	{
		bHasReceivedInitialData = true;
		SetActorLocation(ReplicatedStartLocation);
		
		FColor DebugColor = HasAuthority() ? FColor::Red : FColor::Blue;
		GEngine->AddOnScreenDebugMessage(-1, 0.f, DebugColor,
			FString::Printf(TEXT("[%s] bhasReceivedInitialData grace a onrep = true"),
				HasAuthority() ? TEXT("SRV") : TEXT("CLI"))
		);
		
	}
}

// Called when the game starts or when spawned
void AMovingObstacle::SetupDynamicMaterials()
{
	if (!Mesh || DynamicMaterials.Num() > 0)
	{
		return;
	}

	const int32 NumMaterials = Mesh->GetNumMaterials();
	DynamicMaterials.Reserve(NumMaterials);
	for (int32 i = 0; i < NumMaterials; ++i)
	{
		if (UMaterialInterface* BaseMat = Mesh->GetMaterial(i))
		{
			UMaterialInstanceDynamic* MID = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(i, BaseMat);
			if (MID)
			{
				DynamicMaterials.Add(MID);
			}
		}
	}
}

void AMovingObstacle::SetOpacity(float Value)
{
	for (UMaterialInstanceDynamic* MID : DynamicMaterials)
	{
		if (MID)
		{
			MID->SetScalarParameterValue(OpacityParameterName, Value);
		}
	}
}

void AMovingObstacle::FinishFade()
{
	bFadeFinished = true;
	SetOpacity(1.f);
	if (Mesh)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

void AMovingObstacle::BeginPlay()
{
	Super::BeginPlay();

	SetupDynamicMaterials();
	SetOpacity(0.f);
	if (Mesh)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (HasAuthority())
	{
		ReplicatedStartLocation = GetActorLocation();
		ReplicatedDirection = GetActorForwardVector();
		
		if (AGameStateBase* GS = GetWorld()->GetGameState())
		{
			ServerStartTime = GS->GetServerWorldTimeSeconds();
		}else
		{
			ServerStartTime = GetWorld()->GetTimeSeconds();
		}
		bHasReceivedInitialData = true;
	}
}


// Called every frame
void AMovingObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bHasReceivedInitialData)
	{
		FColor DebugColor = HasAuthority() ? FColor::Red : FColor::Blue;
		GEngine->AddOnScreenDebugMessage(-1, 0.f, DebugColor,
			FString::Printf(TEXT("[%s] bhasReceivedInitialData = false"),
				HasAuthority() ? TEXT("SRV") : TEXT("CLI"))
		);

		
		return; // On n'a pas encore les données nécessaires pour calculer la position
	}
	
	FColor DebugColor = HasAuthority() ? FColor::Red : FColor::Blue;
	
	float CurrentTime = 0.f;
	
	if (AGameStateBase* GS = GetWorld()->GetGameState())
	{
		CurrentTime = GS->GetServerWorldTimeSeconds();
	}else
	{
		CurrentTime = GetWorld()->GetTimeSeconds();
	}
	
	float ElapsedTime = CurrentTime - ServerStartTime;
	
	if (ElapsedTime < 0.f) ElapsedTime = 0.f;
	
	FVector NewLocation = ReplicatedStartLocation + (ReplicatedDirection * Speed * ElapsedTime);
	SetActorLocation(NewLocation, true);

	const float Distance = FVector::Dist(ReplicatedStartLocation, NewLocation);
	const float FadeOutStartDistance = MaxDistance - FadeOutDistance;

	if (FadeOutDistance > 0.f && Distance >= FadeOutStartDistance)
	{
		const float FadeOutProgress = FMath::Clamp((Distance - FadeOutStartDistance) / FadeOutDistance, 0.f, 1.f);
		SetOpacity(1.f - FadeOutProgress);

		if (!bFadeOutCollisionDisabled && FadeOutProgress >= 0.5f)
		{
			bFadeOutCollisionDisabled = true;
			if (Mesh)
			{
				Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
	else if (!bFadeFinished)
	{
		if (SpawnFadeDuration <= 0.f || ElapsedTime >= SpawnFadeDuration)
		{
			FinishFade();
		}
		else
		{
			SetOpacity(ElapsedTime / SpawnFadeDuration);
		}
	}

	if (HasAuthority())
	{
		if (Distance >= MaxDistance)
		{
			Destroy();
		}
	}
	
}

