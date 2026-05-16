// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Obstacle/MovingObstacle.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"

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
void AMovingObstacle::BeginPlay()
{
	Super::BeginPlay();
	
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
	GEngine->AddOnScreenDebugMessage(-1, 0.f, DebugColor,
		FString::Printf(TEXT("[%s] bhasReceivedInitialData = true"),
			HasAuthority() ? TEXT("SRV") : TEXT("CLI"))
	);
	
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
	
	if (HasAuthority())
	{
		float Distance = FVector::Dist(ReplicatedStartLocation, NewLocation);
		if (Distance >= MaxDistance)
		{
			Destroy();
		}
	}
	
}

