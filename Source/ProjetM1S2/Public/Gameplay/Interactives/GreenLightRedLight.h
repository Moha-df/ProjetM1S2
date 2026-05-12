// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "GreenLightRedLight.generated.h"

class USceneComponent;
class UPrimitiveComponent;
class UBoxComponent;
class UPointLightComponent;
class AProjetM1S2Character;

UCLASS()
class PROJETM1S2_API AGreenLightRedLight : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGreenLightRedLight();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UBoxComponent* RoomTrigger;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UPointLightComponent* StateLight;

	UPROPERTY(EditAnywhere, Category = "GreenLightRedLight")
	float GreenDuration = 3.0f;

	UPROPERTY(EditAnywhere, Category = "GreenLightRedLight")
	float RedDuration = 2.0f;

	UPROPERTY(EditAnywhere, Category = "GreenLightRedLight")
	float MovementTolerance = 8.0f;

	UPROPERTY(EditAnywhere, Category = "GreenLightRedLight")
	FLinearColor GreenLightColor = FLinearColor(0.1f, 1.0f, 0.2f);

	UPROPERTY(EditAnywhere, Category = "GreenLightRedLight")
	FLinearColor RedLightColor = FLinearColor(1.0f, 0.15f, 0.1f);

	bool bIsGreenLight = true;
	FTimerHandle LightTimerHandle;

	TSet<TWeakObjectPtr<AProjetM1S2Character>> TrackedCharacters;
	TMap<TWeakObjectPtr<AProjetM1S2Character>, FVector> RedLightStartLocations;

	void SetLightState(bool bGreen);
	void SwitchLightState();
	void CacheRedLightStartLocations();

	UFUNCTION()
	void HandleOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
