// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObstacleSpawner.generated.h"

UCLASS()
class PROJETM1S2_API AObstacleSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AObstacleSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


public:	

	UPROPERTY(EditAnywhere, Category = "Spawner")
	TArray<TSubclassOf<AActor>> ObstacleClasses;
	
	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0.1"))
	float SpawnInterval = 3.f;
	
	UPROPERTY(EditAnywhere, Category = "Spawner")
	bool bRandomSelection = true;
	
	UPROPERTY(EditAnywhere, Category = "Spawner")
	float InitialDelay = 1.f;

	
private:
	void SpawnObstacle();
	
	FTimerHandle SpawnTimerHandle;

	int32 CurrentIndex = 0;
};