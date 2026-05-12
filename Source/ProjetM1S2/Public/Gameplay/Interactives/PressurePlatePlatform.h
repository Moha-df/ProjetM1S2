#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PressurePlatePlatform.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPrimitiveComponent;

/**
 * Networked actor combining a pressure plate and a movable platform.
 * Server detects overlaps and replicates the activation state.
 * Each machine interpolates the platform locally for smooth movement.
 */
UCLASS()
class PROJETM1S2_API APressurePlatePlatform : public AActor
{
    GENERATED_BODY()

public:
    APressurePlatePlatform();

    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components") 
    TObjectPtr<UStaticMeshComponent> PressurePlateMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> DetectionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> PlatformMesh;
    
    /** Visual marker for the platform's target position. Hidden at runtime. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> PlatformTargetMesh;

    /** Interpolation speed for platform movement. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform Behavior",
              meta = (ClampMin = "0.1", ClampMax = "20.0"))
    float InterpSpeed = 3.f;

private:
    FVector PlatformStartLocation;

    /** Replicated activation state. Source of truth: server. */
    UPROPERTY(ReplicatedUsing = OnRep_PlateActive)
    bool bIsPlateActive = false;

    /** Counter of overlapping characters. Server-only. */
    int32 OverlappingCharactersCount = 0;
    
    FVector PlatformActiveLocation;

    UFUNCTION()
    void OnRep_PlateActive();

    UFUNCTION()
    void OnPlateBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                              bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnPlateEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};