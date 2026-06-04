// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Interactives/Endgame.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

// Sets default values
AEndgame::AEndgame()
{
	PrimaryActorTick.bCanEverTick = false;

	RoomTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RoomTrigger"));
	RoomTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RoomTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	RoomTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RoomTrigger->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
	RootComponent = RoomTrigger;

	DiscoLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("DiscoLight"));
	DiscoLight->SetupAttachment(RootComponent);
	DiscoLight->Intensity = 5000.0f;
	DiscoLight->AttenuationRadius = 600.0f;
	DiscoLight->bUseTemperature = false;

	RoomMusic = CreateDefaultSubobject<UAudioComponent>(TEXT("RoomMusic"));
	RoomMusic->SetupAttachment(RootComponent);
	RoomMusic->bAutoActivate = false;

	DiscoInterval = 0.3f;
	CurrentDiscoIndex = 0;
	OverlapCount = 0;
	bHideCreditsOnExit = true;
	bStopMusicOnExit = true;

	ProjectName = FText::FromString(TEXT("ProjetM1S2"));
	CreditNames = {
		FText::FromString(TEXT("Mohamed DEFRANSESCHI")),
		FText::FromString(TEXT("Omar ELSEMRY"))
	};

	DiscoColors = {
		FLinearColor::Red,
		FLinearColor::Green,
		FLinearColor::Blue,
		FLinearColor(1.0f, 0.0f, 1.0f),
		FLinearColor(0.0f, 1.0f, 1.0f),
		FLinearColor(1.0f, 1.0f, 0.0f)
	};
}

// Called when the game starts or when spawned
void AEndgame::BeginPlay()
{
	Super::BeginPlay();

	RoomTrigger->OnComponentBeginOverlap.AddDynamic(this, &AEndgame::HandleBeginOverlap);
	RoomTrigger->OnComponentEndOverlap.AddDynamic(this, &AEndgame::HandleEndOverlap);

	if (DiscoColors.Num() == 0)
	{
		DiscoColors.Add(FLinearColor::White);
	}

	DiscoLight->SetLightColor(DiscoColors[0]);
}

void AEndgame::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || !Cast<APawn>(OtherActor))
	{
		return;
	}

	++OverlapCount;
	if (OverlapCount == 1)
	{
		StartRoomEffects(OtherActor);
	}
}

void AEndgame::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor || !Cast<APawn>(OtherActor))
	{
		return;
	}

	OverlapCount = FMath::Max(0, OverlapCount - 1);
	if (OverlapCount == 0)
	{
		StopRoomEffects();
	}
}

void AEndgame::StartRoomEffects(AActor* TriggeringActor)
{
	GetWorldTimerManager().SetTimer(DiscoTimerHandle, this, &AEndgame::AdvanceDiscoColor, DiscoInterval, true);

	if (RoomMusic && RoomMusic->Sound)
	{
		RoomMusic->Play();
	}

	APawn* Pawn = Cast<APawn>(TriggeringActor);
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (PlayerController && CreditsWidgetClass)
	{
		ActiveCreditsWidget = CreateWidget<UUserWidget>(PlayerController, CreditsWidgetClass);
		if (ActiveCreditsWidget)
		{
			ActiveCreditsWidget->AddToViewport();
			OnCreditsShown(ActiveCreditsWidget);
		}
	}
}

void AEndgame::StopRoomEffects()
{
	GetWorldTimerManager().ClearTimer(DiscoTimerHandle);

	if (bStopMusicOnExit && RoomMusic && RoomMusic->IsPlaying())
	{
		RoomMusic->Stop();
	}

	if (bHideCreditsOnExit && ActiveCreditsWidget)
	{
		ActiveCreditsWidget->RemoveFromParent();
		ActiveCreditsWidget = nullptr;
	}
}

void AEndgame::AdvanceDiscoColor()
{
	if (DiscoColors.Num() == 0)
	{
		return;
	}

	CurrentDiscoIndex = (CurrentDiscoIndex + 1) % DiscoColors.Num();
	DiscoLight->SetLightColor(DiscoColors[CurrentDiscoIndex]);
}
