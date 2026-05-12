#include "Gameplay/Interactives/PressurePlatePlatform.h"

#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

APressurePlatePlatform::APressurePlatePlatform()
{
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    PressurePlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PressurePlateMesh"));
    PressurePlateMesh->SetupAttachment(RootComponent);
    PressurePlateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    DetectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DetectionBox"));
    DetectionBox->SetupAttachment(RootComponent);
    DetectionBox->SetBoxExtent(FVector(50.f, 50.f, 30.f));
    DetectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DetectionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
    DetectionBox->SetGenerateOverlapEvents(true);

    PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
    PlatformMesh->SetupAttachment(RootComponent);
    PlatformMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    PlatformMesh->SetCollisionResponseToAllChannels(ECR_Block);
    
    PlatformTargetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformTargetMesh"));
    PlatformTargetMesh->SetupAttachment(RootComponent);
    PlatformTargetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlatformTargetMesh->SetHiddenInGame(true);
}

void APressurePlatePlatform::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(APressurePlatePlatform, bIsPlateActive);
}

void APressurePlatePlatform::BeginPlay()
{
    Super::BeginPlay();

    PlatformStartLocation = PlatformMesh->GetRelativeLocation();
    PlatformActiveLocation = PlatformTargetMesh->GetRelativeLocation();

    PlatformTargetMesh->SetVisibility(false);
    PlatformTargetMesh->SetHiddenInGame(true);

    if (HasAuthority())
    {
        DetectionBox->OnComponentBeginOverlap.AddDynamic(this, &APressurePlatePlatform::OnPlateBeginOverlap);
        DetectionBox->OnComponentEndOverlap.AddDynamic(this, &APressurePlatePlatform::OnPlateEndOverlap);
    }
}

void APressurePlatePlatform::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const FVector TargetLocation = bIsPlateActive ? PlatformActiveLocation : PlatformStartLocation;

    const FVector CurrentLocation = PlatformMesh->GetRelativeLocation();
    const FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, InterpSpeed);
    PlatformMesh->SetRelativeLocation(NewLocation);
}

void APressurePlatePlatform::OnRep_PlateActive()
{
    // Reserved for client-side feedback (sound, VFX, etc.).
}

void APressurePlatePlatform::OnPlateBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                                   bool bFromSweep, const FHitResult& SweepResult)
{
    if (!HasAuthority()) { return; }
    if (!OtherActor || !OtherActor->IsA(ACharacter::StaticClass())) { return; }

    ++OverlappingCharactersCount;
    if (OverlappingCharactersCount > 0 && !bIsPlateActive)
    {
        bIsPlateActive = true;
    }
}

void APressurePlatePlatform::OnPlateEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!HasAuthority()) { return; }
    if (!OtherActor || !OtherActor->IsA(ACharacter::StaticClass())) { return; }

    OverlappingCharactersCount = FMath::Max(0, OverlappingCharactersCount - 1);
    if (OverlappingCharactersCount == 0 && bIsPlateActive)
    {
        bIsPlateActive = false;
    }
}