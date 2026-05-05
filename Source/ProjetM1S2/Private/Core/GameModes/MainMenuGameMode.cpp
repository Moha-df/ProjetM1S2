#include "Core/GameModes/MainMenuGameMode.h"
#include "Core/PlayerController/MainMenuController.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	// Pas de pawn par défaut : dans un menu, le joueur n'incarne rien.
	// Conséquence : le PlayerController n'aura pas de Pawn à possess,
	// donc il cherchera automatiquement une CameraActor avec auto-activation
	// dans le level pour fournir une vue.
	DefaultPawnClass = nullptr;

	// On utilise notre PlayerController custom qui affichera le widget.
	PlayerControllerClass = AMainMenuController::StaticClass();
}