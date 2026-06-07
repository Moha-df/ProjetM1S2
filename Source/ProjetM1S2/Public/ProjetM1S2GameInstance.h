#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h" // ETravelFailure / ENetworkFailure
#include "Interfaces/OnlineSessionInterface.h"
#include "ProjetM1S2GameInstance.generated.h"

class UUserWidget;
class UNetDriver;

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

    /**
     * Démarre l'écran de chargement et le garde affiché jusqu'à ce que le
     * joueur soit réellement prêt à se déplacer (pawn possédé dans la map finale).
     *
     * À appeler INSTANTANÉMENT au clic :
     *   - HostSession l'appelle déjà tout seul.
     *   - Pour le join, USessionEntryWidget l'appelle avant JoinSession.
     *
     * Le visuel utilisé est LoadingWidgetClass. Le rendu survit au chargement
     * de map grâce au MoviePlayer (couche Slate), puis un widget UMG normal
     * prend le relais le temps que le pawn apparaisse.
     */
    UFUNCTION(BlueprintCallable, Category = "Loading")
    void StartLoadingFlow();

protected:
    /**
     * Nom de la map à charger après création de session.
     * À configurer dans le BP enfant (BP_ProjetM1S2GameInstance) ou laisser tel quel.
     * Format : "/Game/Maps/TaMap" (sans extension .umap).
     */
    UPROPERTY(EditDefaultsOnly, Category = "Sessions")
    FString LobbyMapName = TEXT("/Game/FirstPerson/Lvl_FirstPerson");
    
    /** Nom de la map de menu principal. */
    UPROPERTY(EditDefaultsOnly, Category = "Sessions")
    FString MainMenuMapName = TEXT("/Game/Maps/L_MainMenu");

    // === Écran de chargement ===

    /**
     * Widget UMG plein écran affiché pendant le chargement.
     * À régler sur ton WBP_LoadingScreen dans le BP enfant (BP_ProjetM1S2GameInstance).
     * Si laissé vide, aucun visuel ne sera affiché.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Loading")
    TSubclassOf<UUserWidget> LoadingWidgetClass;

    /** Z-order du widget de chargement (élevé = au-dessus de tout le reste). */
    UPROPERTY(EditDefaultsOnly, Category = "Loading")
    int32 LoadingWidgetZOrder = 1000;

    /**
     * Sécurité anti-blocage : on retire le loading au bout de ce délai même si
     * le pawn n'apparaît jamais (ex: échec réseau silencieux).
     */
    UPROPERTY(EditDefaultsOnly, Category = "Loading", meta = (ClampMin = "5.0"))
    float LoadingTimeoutSeconds = 30.0f;

private:
    /**
     * Décide comment (re)lancer un host : s'il reste une session enregistrée,
     * on la détruit d'abord (host différé via OnDestroySessionComplete) ; sinon
     * on crée directement. Partagé par HostSession et le retry sur échec de
     * listen, SANS toucher au budget de retry (pour éviter une boucle infinie).
     */
    void StartHostSequence(int32 MaxPlayers);

    /**
     * Crée effectivement la session et lance la map en listen.
     * Extrait de HostSession pour pouvoir être rappelé une fois qu'une éventuelle
     * session précédente a fini d'être détruite (cf. OnDestroySessionComplete).
     */
    void CreateAndStartSession(int32 MaxPlayers);

    /**
     * True quand une destruction de session est en cours UNIQUEMENT pour
     * pouvoir re-créer une session host juste après (host différé).
     * Distingue le cas "host" du cas "quitter vers le menu" dans
     * OnDestroySessionComplete, qui est partagé entre les deux.
     */
    bool bHostAfterDestroy = false;

    /** MaxPlayers mémorisé pour le host différé après destruction. */
    int32 PendingHostMaxPlayers = 4;

    /**
     * True pendant tout un flux de host (du clic jusqu'à ce que le listen
     * réussisse ou échoue définitivement). Sert à ne déclencher le retry
     * sur échec de listen QUE pour un host (pas pour un join).
     */
    bool bHostFlowActive = false;

    /**
     * Nombre de tentatives de host restantes après un échec de listen
     * ("Already have a listen socket on P2P vport 17777"). Le socket Steam de
     * l'ancien serveur n'est libéré qu'au moment où la tentative échouée
     * retourne au menu ; une seule nouvelle tentative suffit alors.
     */
    int32 HostListenRetriesRemaining = 0;

    /**
     * Armé par HandleNetworkFailure quand un listen échoue : on attend que le
     * moteur soit retombé sur une map (typiquement le menu, ce qui libère le
     * socket Steam) avant de relancer le host depuis OnPostLoadMap.
     */
    bool bRetryHostPending = false;

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

    // === Écran de chargement (implémentation) ===

    /** Bound à FCoreUObjectDelegates::PreLoadMap — prépare le MoviePlayer. */
    void OnPreLoadMap(const FString& MapName);

    /** Bound à FCoreUObjectDelegates::PostLoadMapWithWorld — passe au widget UMG. */
    void OnPostLoadMap(UWorld* LoadedWorld);

    /** Crée un widget de chargement (depuis le PC local si dispo, sinon la GameInstance). */
    UUserWidget* CreateLoadingWidget();

    /** Ajoute le widget UMG de chargement au viewport. */
    void ShowLoadingWidget();

    /** Retire le widget UMG de chargement du viewport. */
    void RemoveLoadingWidget();

    /** Vérifié périodiquement après le chargement : le pawn local est-il prêt ? */
    void CheckPawnReady();

    /** Termine proprement le flow de chargement (retire tout, stop movie, reset). */
    void FinishLoading();

    /** Échec de voyage (map introuvable, etc.) → on retire le loading. */
    void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

    /** Échec réseau (connexion au host perdue, etc.) → on retire le loading. */
    void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);

    /** Widget UMG actuellement affiché (phase post-chargement). */
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> ActiveLoadingWidget;

    /** Widget UMG wrappé dans le MoviePlayer (phase chargement de map). */
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> MovieLoadingWidget;

    /** True entre le clic et le moment où le pawn est prêt. */
    bool bLoadingFlowActive = false;

    FTimerHandle PawnReadyTimerHandle;
    FTimerHandle LoadingTimeoutHandle;
    FDelegateHandle PreLoadMapHandle;
    FDelegateHandle PostLoadMapHandle;
    FDelegateHandle TravelFailureHandle;
    FDelegateHandle NetworkFailureHandle;
};