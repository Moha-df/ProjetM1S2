// MainMenuGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

/**
 * GameMode dédié au menu principal.
 * Pas de gameplay : pas de pawn joueur par défaut, juste un PlayerController
 * qui affiche le widget de menu.
 */
UCLASS()
class PROJETM1S2_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();
};