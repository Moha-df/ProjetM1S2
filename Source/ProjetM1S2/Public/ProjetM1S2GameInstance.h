#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "ProjetM1S2GameInstance.generated.h"

/**
 * GameInstance custom du projet.
 * Gère tout le cycle de vie multijoueur :
 *   - Création de session (host)
 *   - Acceptation d'invitations reçues
 *   - Join de session
 *   - Travel vers le serveur du host
 *
 * Vit toute la session de jeu (de l'ouverture à la fermeture du jeu),
 * ce qui en fait le bon endroit pour les systèmes online persistants.
 */
UCLASS()
class PROJETM1S2_API UProjetM1S2GameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    /** Init / Shutdown — équivalents BeginPlay / EndPlay pour la GameInstance */
    virtual void Init() override;
    virtual void Shutdown() override;

    /**
     * Crée une session multijoueur et charge la lobby map en mode listen.
     * @param MaxPlayers nombre maximum de joueurs (host inclus).
     */
    UFUNCTION(BlueprintCallable, Category = "Sessions")
    void HostSession(int32 MaxPlayers = 4);

    /** Détruit la session active. Appelable quand on quitte la partie. */
    UFUNCTION(BlueprintCallable, Category = "Sessions")
    void DestroyCurrentSession();

    /** Pour tester rapidement depuis la console UE (touche ~). Tape "HostGame". */
    UFUNCTION(Exec)
    void HostGame();
    
    /**
     * Quitte la session courante et retourne au menu principal.
     * - Si on est host : la session est détruite (les clients seront déconnectés).
     * - Si on est client : on se déconnecte simplement.
     * Dans les deux cas, on revient sur la MainMenu map.
     */
    UFUNCTION(BlueprintCallable, Category = "Sessions")
    void LeaveSessionAndReturnToMainMenu();

protected:
    /**
     * Nom de la map à charger après création de session.
     * À configurer dans le BP enfant (BP_ProjetM1S2GameInstance) ou laisser tel quel.
     * Format : "/Game/Maps/TaMap" (sans extension .umap).
     */
    UPROPERTY(EditDefaultsOnly, Category = "Sessions")
    FString LobbyMapName = TEXT("/Game/Maps/Lobby");
    
    /** Nom de la map de menu principal. */
    UPROPERTY(EditDefaultsOnly, Category = "Sessions")
    FString MainMenuMapName = TEXT("/Game/Maps/L_MainMenu");

private:
    // === Callbacks délégués OSS ===
    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId,
                                     FUniqueNetIdPtr UserId, 
                                     const FOnlineSessionSearchResult& InviteResult);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    
    /** Callback déclenché quand la destruction de session est terminée. */
    void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

    // === Handles pour pouvoir unbind proprement ===
    FDelegateHandle CreateSessionCompleteHandle;
    FDelegateHandle InviteAcceptedHandle;
    FDelegateHandle JoinSessionCompleteHandle;
    /** Handle pour le délégué de destruction de session. */
    FDelegateHandle DestroySessionCompleteHandle;
};