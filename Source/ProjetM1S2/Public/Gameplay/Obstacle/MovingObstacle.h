// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MovingObstacle.generated.h"

UCLASS()
class PROJETM1S2_API AMovingObstacle : public AActor
{
	GENERATED_BODY()
public:	
	// Sets default values for this actor's properties
	AMovingObstacle();
	
	// Override pour déclarer les propriétés répliquées
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
private:
	UPROPERTY(EditAnywhere)
	UStaticMeshComponent* Mesh;
	
	UPROPERTY(Replicated)
	FVector ReplicatedStartLocation;
	
	UPROPERTY(Replicated)
	FVector ReplicatedDirection;
	
	UPROPERTY(ReplicatedUsing = OnRep_ServerStartTime)
	float ServerStartTime = -10.f;
	
	UFUNCTION()
	void OnRep_ServerStartTime();
	
	bool bHasReceivedInitialData = false;
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	float Speed = 1000.f;
	
	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxDistance = 10000.f;
};