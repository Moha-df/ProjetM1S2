// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Interactives/CheckpointSpawnMarker.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProjetM1S2Character.h"

ACheckpointSpawnMarker::ACheckpointSpawnMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	RespawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(SceneRoot);

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(SceneRoot);
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(SceneRoot);
	TriggerVolume->SetBoxExtent(FVector(60.0f, 60.0f, 60.0f));
	TriggerVolume->SetCollisionProfileName(TEXT("Trigger"));
}

void ACheckpointSpawnMarker::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ACheckpointSpawnMarker::HandleOverlapBegin);
	}
}

void ACheckpointSpawnMarker::HandleOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AProjetM1S2Character* Character = Cast<AProjetM1S2Character>(OtherActor);
	if (!Character || !RespawnPoint)
	{
		return;
	}

	Character->SetCheckpointTransform(RespawnPoint->GetComponentTransform());
}
