// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Interactives/GreenLightRedLight.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "ProjetM1S2Character.h"

// Sets default values
AGreenLightRedLight::AGreenLightRedLight()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;

	RoomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomTrigger"));
	RoomTrigger->SetupAttachment(SceneRoot);
	RoomTrigger->SetBoxExtent(FVector(400.0f, 400.0f, 200.0f));
	RoomTrigger->SetCollisionProfileName(TEXT("Trigger"));

	StateLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StateLight"));
	StateLight->SetupAttachment(SceneRoot);
	StateLight->Intensity = 8000.0f;
	StateLight->SetLightColor(GreenLightColor);
}

void AGreenLightRedLight::BeginPlay()
{
	Super::BeginPlay();

	if (RoomTrigger)
	{
		RoomTrigger->OnComponentBeginOverlap.AddDynamic(this, &AGreenLightRedLight::HandleOverlapBegin);
		RoomTrigger->OnComponentEndOverlap.AddDynamic(this, &AGreenLightRedLight::HandleOverlapEnd);
	}

	SetLightState(true);
	GetWorldTimerManager().SetTimer(LightTimerHandle, this, &AGreenLightRedLight::SwitchLightState, GreenDuration, false);
}

void AGreenLightRedLight::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsGreenLight)
	{
		return;
	}

	for (auto It = TrackedCharacters.CreateIterator(); It; ++It)
	{
		AProjetM1S2Character* Character = It->Get();
		if (!Character)
		{
			It.RemoveCurrent();
			continue;
		}

		const FVector CurrentLocation = Character->GetActorLocation();
		const FVector* StartLocation = RedLightStartLocations.Find(*It);
		if (!StartLocation)
		{
			RedLightStartLocations.Add(*It, CurrentLocation);
			continue;
		}

		const float DistanceSq = FVector::DistSquared(*StartLocation, CurrentLocation);
		if (DistanceSq > MovementTolerance * MovementTolerance)
		{
			Character->RespawnAtCheckpoint();
			RedLightStartLocations.FindOrAdd(*It) = Character->GetActorLocation();
		}
	}
}

void AGreenLightRedLight::SetLightState(bool bGreen)
{
	bIsGreenLight = bGreen;

	if (StateLight)
	{
		StateLight->SetLightColor(bIsGreenLight ? GreenLightColor : RedLightColor);
	}

	if (!bIsGreenLight)
	{
		CacheRedLightStartLocations();
	}
	else
	{
		RedLightStartLocations.Empty();
	}
}

void AGreenLightRedLight::SwitchLightState()
{
	SetLightState(!bIsGreenLight);

	const float NextDuration = bIsGreenLight ? GreenDuration : RedDuration;
	GetWorldTimerManager().SetTimer(LightTimerHandle, this, &AGreenLightRedLight::SwitchLightState, NextDuration, false);
}

void AGreenLightRedLight::CacheRedLightStartLocations()
{
	RedLightStartLocations.Empty();
	for (const TWeakObjectPtr<AProjetM1S2Character>& Character : TrackedCharacters)
	{
		if (Character.IsValid())
		{
			RedLightStartLocations.Add(Character, Character->GetActorLocation());
		}
	}
}

void AGreenLightRedLight::HandleOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AProjetM1S2Character* Character = Cast<AProjetM1S2Character>(OtherActor);
	if (!Character)
	{
		return;
	}

	TrackedCharacters.Add(Character);
	if (!bIsGreenLight)
	{
		RedLightStartLocations.Add(Character, Character->GetActorLocation());
	}
}

void AGreenLightRedLight::HandleOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AProjetM1S2Character* Character = Cast<AProjetM1S2Character>(OtherActor);
	if (!Character)
	{
		return;
	}

	TrackedCharacters.Remove(Character);
	RedLightStartLocations.Remove(Character);
}
