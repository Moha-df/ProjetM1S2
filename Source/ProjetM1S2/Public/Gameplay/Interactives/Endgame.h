// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Endgame.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UAudioComponent;
class UUserWidget;

UCLASS()
class PROJETM1S2_API AEndgame : public AActor
{
	GENERATED_BODY()

public:
	AEndgame();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void AdvanceDiscoColor();
	void StartRoomEffects(AActor* TriggeringActor);
	void StopRoomEffects();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* RoomTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UPointLightComponent* DiscoLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAudioComponent* RoomMusic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disco")
	TArray<FLinearColor> DiscoColors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disco", meta = (ClampMin = "0.05"))
	float DiscoInterval;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Credits")
	FText ProjectName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Credits")
	TArray<FText> CreditNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Credits")
	TSubclassOf<UUserWidget> CreditsWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Credits")
	bool bHideCreditsOnExit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	bool bStopMusicOnExit;

	UFUNCTION(BlueprintImplementableEvent, Category = "Credits")
	void OnCreditsShown(UUserWidget* WidgetInstance);

private:
	FTimerHandle DiscoTimerHandle;
	int32 CurrentDiscoIndex;
	int32 OverlapCount;

	UPROPERTY()
	UUserWidget* ActiveCreditsWidget;
};
