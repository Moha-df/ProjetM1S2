// MainMenuController.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuController.generated.h"

class UUserWidget;

/**
 * PlayerController du menu principal.
 * Son rôle : afficher le widget de menu et configurer l'input en mode UI uniquement.
 */
UCLASS()
class PROJETM1S2_API AMainMenuController : public APlayerController
{
	GENERATED_BODY()

public:
	AMainMenuController();

protected:
	virtual void BeginPlay() override;

	/** 
	 * Classe de widget à afficher au démarrage du menu.
	 * On l'expose à l'éditeur pour assigner WBP_MainMenu via le Blueprint dérivé.
	 * On utilise TSubclassOf<UUserWidget> et non un pointeur direct : 
	 * c'est une référence au TYPE de widget, pas une instance.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	/** L'instance du widget créée au runtime. */
	UPROPERTY()
	TObjectPtr<UUserWidget> MainMenuWidget;
};