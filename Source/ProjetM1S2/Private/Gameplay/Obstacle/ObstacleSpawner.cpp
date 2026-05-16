// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Obstacle/ObstacleSpawner.h"

// Sets default values
AObstacleSpawner::AObstacleSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = false;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

// Called when the game starts or when spawned
void AObstacleSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (!HasAuthority())
	{
		return;
	}
	
	if (ObstacleClasses.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ObstacleSpawner: ObstacleClasses est vide, aucun spawn ne sera effectué."));
		return;
	}
	
	GetWorld()->GetTimerManager().SetTimer(
	   SpawnTimerHandle,
	   this,
	   &AObstacleSpawner::SpawnObstacle,
	   SpawnInterval,
	   true,           // bLoop = true : ça se répète
	   InitialDelay    // délai avant le premier appel
   );
	
}

void AObstacleSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Nettoie proprement le timer quand le spawner est détruit ou la map quittée
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AObstacleSpawner::SpawnObstacle()
{
	// Double sécurité : on doit toujours être sur le serveur ici
	if (!HasAuthority() || ObstacleClasses.Num() == 0)
	{
		return;
	}

	// Sélection de la classe à spawn
	TSubclassOf<AActor> ClassToSpawn = nullptr;

	if (bRandomSelection)
	{
		const int32 RandomIndex = FMath::RandRange(0, ObstacleClasses.Num() - 1);
		ClassToSpawn = ObstacleClasses[RandomIndex];
	}
	else
	{
		ClassToSpawn = ObstacleClasses[CurrentIndex];
		CurrentIndex = (CurrentIndex + 1) % ObstacleClasses.Num(); // boucle dans la liste
	}

	// Vérifie que la classe n'est pas nulle (au cas où l'utilisateur aurait laissé un slot vide dans l'éditeur)
	if (!ClassToSpawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ObstacleSpawner: classe nulle dans ObstacleClasses, spawn annulé."));
		return;
	}

	// Spawn à la position et rotation du spawner
	const FVector SpawnLocation = GetActorLocation();
	const FRotator SpawnRotation = GetActorRotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.Owner = this;

	AActor* SpawnedObstacle = GetWorld()->SpawnActor<AActor>(
		ClassToSpawn,
		SpawnLocation,
		SpawnRotation,
		SpawnParams
	);

	if (SpawnedObstacle)
	{
		UE_LOG(LogTemp, Log, TEXT("ObstacleSpawner: spawn de %s à %s"),
			*SpawnedObstacle->GetName(), *SpawnLocation.ToString());
	}
}


