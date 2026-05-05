// MainMenuController.cpp
#include "Core/PlayerController/MainMenuController.h"
#include "Blueprint/UserWidget.h"

AMainMenuController::AMainMenuController()
{
	// Affiche le curseur souris : indispensable pour cliquer sur les boutons.
	bShowMouseCursor = true;
}

void AMainMenuController::BeginPlay()
{
	Super::BeginPlay();

	// Sécurité : on vérifie que la classe de widget a bien été assignée
	// dans le Blueprint dérivé. Sinon on log une erreur claire pour le debug.
	if (!MainMenuWidgetClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("MainMenuWidgetClass non assigné dans %s. "
				 "Assigne WBP_MainMenu dans le Blueprint dérivé du PlayerController."),
			*GetName());
		return;
	}

	// CreateWidget instancie le widget UMG.
	// Premier paramètre : l'owner (ici, le PlayerController lui-même).
	// Deuxième paramètre : la classe de widget à instancier.
	MainMenuWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);

	if (MainMenuWidget)
	{
		// AddToViewport ajoute le widget à l'écran avec un Z-order par défaut.
		MainMenuWidget->AddToViewport();

		// Configure l'input : seul l'UI capte les inputs.
		// FInputModeUIOnly est crucial pour un menu : empêche les clics "fantômes"
		// de passer à travers l'UI vers le monde 3D.
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MainMenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
	}
}