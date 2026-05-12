// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "CheckpointSpawnMarker.generated.h"

class USceneComponent;
class UBoxComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class AProjetM1S2Character;

UCLASS()
class PROJETM1S2_API ACheckpointSpawnMarker : public AActor
{
	GENERATED_BODY()

public:
	ACheckpointSpawnMarker();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent* MarkerMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* TriggerVolume;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* RespawnPoint;

	UFUNCTION()
	void HandleOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);
};

