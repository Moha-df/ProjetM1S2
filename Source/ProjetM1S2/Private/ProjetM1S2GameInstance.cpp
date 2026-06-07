#include "ProjetM1S2GameInstance.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Online/OnlineSessionNames.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "MoviePlayer.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

void UProjetM1S2GameInstance::Init()
{
    Super::Init();

    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] No OnlineSubsystem at Init."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[GameInstance] OSS active: %s"), *OnlineSub->GetSubsystemName().ToString());

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Session interface invalid at Init."));
        return;
    }

    // S'abonner aux 3 événements qu'on veut traiter.
    CreateSessionCompleteHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(
            this, &UProjetM1S2GameInstance::OnCreateSessionComplete));

    InviteAcceptedHandle = Sessions->AddOnSessionUserInviteAcceptedDelegate_Handle(
        FOnSessionUserInviteAcceptedDelegate::CreateUObject(
            this, &UProjetM1S2GameInstance::OnSessionUserInviteAccepted));

    JoinSessionCompleteHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(
            this, &UProjetM1S2GameInstance::OnJoinSessionComplete));
    
    DestroySessionCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
    FOnDestroySessionCompleteDelegate::CreateUObject(
        this, &UProjetM1S2GameInstance::OnDestroySessionComplete));

    UE_LOG(LogTemp, Log, TEXT("[GameInstance] Session delegates registered."));

    // Délégués de chargement de map — pilotent l'écran de chargement.
    // PreLoadMap : juste avant que LoadMap démarre (l'ancien monde va être détruit).
    // PostLoadMapWithWorld : nouveau monde prêt (mais le pawn peut ne pas l'être encore).
    PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(
        this, &UProjetM1S2GameInstance::OnPreLoadMap);
    PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this, &UProjetM1S2GameInstance::OnPostLoadMap);

    // Si un voyage échoue (map introuvable, connexion au host perdue...), on
    // ne veut surtout pas laisser le joueur coincé sous l'écran de chargement.
    if (GEngine)
    {
        TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(
            this, &UProjetM1S2GameInstance::HandleTravelFailure);
        NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(
            this, &UProjetM1S2GameInstance::HandleNetworkFailure);
    }
}

void UProjetM1S2GameInstance::Shutdown()
{
    // Toujours unbind les délégués proprement — sinon callbacks sur GameInstance morte = crash.
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (OnlineSub)
    {
        IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
        if (Sessions.IsValid())
        {
            Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
            Sessions->ClearOnSessionUserInviteAcceptedDelegate_Handle(InviteAcceptedHandle);
            Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
            Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteHandle);
        }
    }

    // Cleanup chargement.
    FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
    if (GEngine)
    {
        GEngine->OnTravelFailure().Remove(TravelFailureHandle);
        GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PawnReadyTimerHandle);
        World->GetTimerManager().ClearTimer(LoadingTimeoutHandle);
    }

    Super::Shutdown();
}

void UProjetM1S2GameInstance::HostGame()
{
    // Pour tester rapidement depuis la console : "HostGame" (touche ~).
    HostSession();
}

void UProjetM1S2GameInstance::HostSession(int32 MaxPlayers)
{
    // Loading instantané dès le clic, avant même de contacter Steam.
    StartLoadingFlow();

    // Nouveau flux de host : on autorise une (1) nouvelle tentative si le listen
    // échoue parce que le socket Steam de l'ancien serveur n'est pas encore libéré.
    bHostFlowActive = true;
    HostListenRetriesRemaining = 1;

    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] HostSession: no OSS."));
        FinishLoading(); // rien ne va se charger : on retire le loading.
        return;
    }

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] HostSession: invalid session interface."));
        FinishLoading();
        return;
    }

    IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] HostSession: invalid identity interface."));
        FinishLoading();
        return;
    }

    const FUniqueNetIdPtr LocalUserId = Identity->GetUniquePlayerId(0);
    if (!LocalUserId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] HostSession: invalid local user id."));
        FinishLoading();
        return;
    }

    StartHostSequence(MaxPlayers);
}

void UProjetM1S2GameInstance::StartHostSequence(int32 MaxPlayers)
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub) { FinishLoading(); return; }

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid()) { FinishLoading(); return; }

    // Si une session existe déjà, on la détruit d'abord, MAIS DestroySession est
    // asynchrone : il faut attendre sa fin avant de recréer, sinon le socket
    // d'écoute Steam (P2P vport 17777) n'est pas encore relâché → le nouveau
    // listen échoue ("Already have a listen socket on P2P vport 17777") et le
    // moteur renvoie au menu. On mémorise donc l'intention de host et on relance
    // CreateSession dans OnDestroySessionComplete.
    if (Sessions->GetNamedSession(NAME_GameSession))
    {
        UE_LOG(LogTemp, Log, TEXT("[GameInstance] Existing session found — destroying first, will host after."));
        bHostAfterDestroy = true;
        PendingHostMaxPlayers = MaxPlayers;
        Sessions->DestroySession(NAME_GameSession);
        return;
    }

    CreateAndStartSession(MaxPlayers);
}

void UProjetM1S2GameInstance::CreateAndStartSession(int32 MaxPlayers)
{
    // Mémorisé pour qu'un éventuel retry (échec de listen) puisse relancer
    // la création avec le même nombre de joueurs.
    PendingHostMaxPlayers = MaxPlayers;

    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub) { FinishLoading(); return; }

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid()) { FinishLoading(); return; }

    IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
    if (!Identity.IsValid()) { FinishLoading(); return; }

    const FUniqueNetIdPtr LocalUserId = Identity->GetUniquePlayerId(0);
    if (!LocalUserId.IsValid()) { FinishLoading(); return; }

    // === Paramètres de la session ===
    FOnlineSessionSettings SessionSettings;
    SessionSettings.NumPublicConnections = MaxPlayers;     // Slots publics (joinables)
    SessionSettings.NumPrivateConnections = 0;             // Slots invite-only
    SessionSettings.bShouldAdvertise = true;               // Visible dans browser de session
    SessionSettings.bAllowJoinInProgress = true;           // Rejoindre en cours de partie
    SessionSettings.bIsLANMatch = false;                   // Internet, pas LAN
    SessionSettings.bUsesPresence = true;                  // CRITIQUE pour Steam — sinon invites cassées
    SessionSettings.bUseLobbiesIfAvailable = true;
    SessionSettings.bAllowJoinViaPresence = true;          // Join via "Join Game" Steam
    SessionSettings.bAllowJoinViaPresenceFriendsOnly = false;
    SessionSettings.bAllowInvites = true;                  // CRITIQUE pour invites
    SessionSettings.bAntiCheatProtected = false;

    UE_LOG(LogTemp, Log, TEXT("[GameInstance] Creating session for %d players..."), MaxPlayers);

    Sessions->CreateSession(*LocalUserId, NAME_GameSession, SessionSettings);
}

void UProjetM1S2GameInstance::DestroyCurrentSession()
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub) return;

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession))
    {
        Sessions->DestroySession(NAME_GameSession);
        UE_LOG(LogTemp, Log, TEXT("[GameInstance] Session destroyed."));
    }
}

void UProjetM1S2GameInstance::LeaveSessionAndReturnToMainMenu()
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] LeaveSession: no OSS."));
        // Pas de Steam dispo : on retourne quand même au menu directement.
        UGameplayStatics::OpenLevel(GetWorld(), FName(*MainMenuMapName));
        return;
    }

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] LeaveSession: invalid session interface."));
        UGameplayStatics::OpenLevel(GetWorld(), FName(*MainMenuMapName));
        return;
    }

    // Vérifier qu'on a bien une session active.
    FNamedOnlineSession* CurrentSession = Sessions->GetNamedSession(NAME_GameSession);
    if (!CurrentSession)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameInstance] LeaveSession: no active session, returning to menu directly."));
        UGameplayStatics::OpenLevel(GetWorld(), FName(*MainMenuMapName));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[GameInstance] Destroying session before returning to menu..."));

    // DestroySession est async. Le travel vers le menu se fait dans OnDestroySessionComplete.
    Sessions->DestroySession(NAME_GameSession);
}

void UProjetM1S2GameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    // Ce callback est partagé entre deux flux :
    //   - Host différé : on a détruit l'ancienne session pour pouvoir en recréer
    //     une (le socket d'écoute Steam vport 17777 est maintenant relâché).
    //   - Quitter la partie : on revient au menu principal.
    if (bHostAfterDestroy)
    {
        bHostAfterDestroy = false;

        if (!bWasSuccessful)
        {
            // La destruction a échoué : le socket d'écoute peut encore être tenu.
            // On tente quand même la création — si le listen échoue, OnCreateSessionComplete
            // / HandleNetworkFailure retireront le loading proprement.
            UE_LOG(LogTemp, Warning, TEXT("[GameInstance] DestroySession failed before deferred host — attempting CreateSession anyway."));
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("[GameInstance] Previous session destroyed — now creating the new host session."));
        }

        CreateAndStartSession(PendingHostMaxPlayers);
        return;
    }

    if (bWasSuccessful)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameInstance] Session destroyed successfully. Returning to main menu."));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] DestroySession failed, but returning to main menu anyway."));
    }

    // Dans les deux cas, on retourne au menu — on ne veut pas bloquer le joueur si Steam galère.
    UGameplayStatics::OpenLevel(GetWorld(), FName(*MainMenuMapName));
}

void UProjetM1S2GameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (!bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] CreateSession FAILED."));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Echec création session"));
        }
        FinishLoading(); // pas de map à charger : on retire le loading.
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[GameInstance] Session created. Loading map: %s"), *LobbyMapName);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Session créée !"));
    }

    // OpenLevel avec bAbsolute=true pour vraiment changer de map, et options="listen"
    // pour démarrer le jeu en mode "listen server" (le host est aussi serveur).
    UGameplayStatics::OpenLevel(GetWorld(), FName(*LobbyMapName), true, TEXT("listen"));
}

void UProjetM1S2GameInstance::OnSessionUserInviteAccepted(const bool bWasSuccessful, const int32 ControllerId,
                                                           FUniqueNetIdPtr UserId,
                                                           const FOnlineSessionSearchResult& InviteResult)
{
    if (!bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Invite accepted but not successful."));
        return;
    }

    if (!UserId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Invite accepted but UserId invalid."));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[GameInstance] Invite accepted, joining session..."));

    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub) return;

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid()) return;

    Sessions->JoinSession(*UserId, NAME_GameSession, InviteResult);
}

void UProjetM1S2GameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (Result != EOnJoinSessionCompleteResult::Success)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] JoinSession failed with result %d"), (int32)Result);
        FinishLoading(); // échec du join : on retire le loading.
        return;
    }

    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub) { FinishLoading(); return; }

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid()) { FinishLoading(); return; }

    // GetResolvedConnectString donne l'adresse de connexion (IP:port ou Steam ID).
    FString ConnectString;
    if (!Sessions->GetResolvedConnectString(NAME_GameSession, ConnectString))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Failed to resolve connect string."));
        FinishLoading();
        return;
    }

    APlayerController* PC = GetFirstLocalPlayerController();
    if (!PC) { FinishLoading(); return; }

    UE_LOG(LogTemp, Log, TEXT("[GameInstance] Traveling to %s"), *ConnectString);
    PC->ClientTravel(ConnectString, TRAVEL_Absolute);
}

// =====================================================================
//  Écran de chargement
//
//  Le chargement se déroule en deux phases, car aucune technique unique
//  ne couvre tout :
//
//   1) Phase "chargement de map" (OpenLevel / ClientTravel) : le monde est
//      détruit puis rechargé, le game thread est bloqué. Un widget UMG
//      normal ne peut PAS s'afficher ici → on utilise le MoviePlayer, une
//      couche Slate qui rend par-dessus tout et survit au LoadMap.
//
//   2) Phase "post-chargement" : le nouveau monde existe et tourne
//      normalement, mais le pawn local n'est pas encore prêt (surtout côté
//      client, où il arrive via réplication réseau). Ici un widget UMG
//      classique fonctionne très bien → on l'affiche et on attend que le
//      pawn apparaisse pour tout retirer.
// =====================================================================

void UProjetM1S2GameInstance::StartLoadingFlow()
{
    // Idempotent : si un flow est déjà en cours, on ne réaffiche pas.
    if (bLoadingFlowActive)
    {
        return;
    }

    bLoadingFlowActive = true;

    // Affiche immédiatement le widget UMG dans le monde courant (menu).
    // Il couvre le délai entre le clic et le début du LoadMap (création/join
    // de session async). Il sera détruit avec le monde du menu, mais le
    // MoviePlayer prendra le relais pendant le LoadMap.
    ShowLoadingWidget();

    UE_LOG(LogTemp, Log, TEXT("[Loading] Flow started."));
}

void UProjetM1S2GameInstance::OnPreLoadMap(const FString& MapName)
{
    // On ne montre l'écran de chargement que pendant un flow host/join.
    // (Sinon il s'afficherait aussi au boot du jeu, au retour menu, etc.)
    if (!bLoadingFlowActive)
    {
        return;
    }

    // En éditeur (PIE), GetMoviePlayer() est un player nul : pas de movie.
    // C'est normal — le loading se voit surtout en build standalone/packaged.
    IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
    if (!MoviePlayer || !IsMoviePlayerEnabled() || !LoadingWidgetClass)
    {
        return;
    }

    // On crée un widget UMG et on le donne à la couche Slate du MoviePlayer.
    // Owner = la GameInstance car le monde courant est sur le point de mourir.
    MovieLoadingWidget = CreateWidget<UUserWidget>(this, LoadingWidgetClass);
    if (!MovieLoadingWidget)
    {
        return;
    }

    FLoadingScreenAttributes Attributes;
    // false : l'écran ne se ferme PAS tout seul à la fin du load — on contrôle
    // la fermeture nous-mêmes (dans OnPostLoadMap) pour enchaîner sur l'UMG.
    Attributes.bAutoCompleteWhenLoadingCompletes = false;
    Attributes.bMoviesAreSkippable = false;
    Attributes.MinimumLoadingScreenDisplayTime = -1.0f;
    Attributes.WidgetLoadingScreen = MovieLoadingWidget->TakeWidget();

    MoviePlayer->SetupLoadingScreen(Attributes);
    // Pas besoin d'appeler PlayMovie() : le moteur le déclenche
    // automatiquement pendant LoadMap car un loading screen est préparé.

    UE_LOG(LogTemp, Log, TEXT("[Loading] MoviePlayer loading screen prepared for %s"), *MapName);
}

void UProjetM1S2GameInstance::OnPostLoadMap(UWorld* LoadedWorld)
{
    if (!bLoadingFlowActive)
    {
        return;
    }

    // Retry de host après un échec de listen : le moteur vient de retomber sur
    // une map (le socket Steam de l'ancien serveur est désormais libéré). On
    // garde l'écran de chargement et on relance la séquence de host, sans
    // attendre de pawn (la map courante n'est qu'une étape intermédiaire).
    if (bRetryHostPending)
    {
        bRetryHostPending = false;

        // Coupe le MoviePlayer (le LoadMap intermédiaire est fini) ; l'UMG reste.
        ShowLoadingWidget();
        if (IGameMoviePlayer* MoviePlayer = GetMoviePlayer())
        {
            MoviePlayer->StopMovie();
        }
        MovieLoadingWidget = nullptr;

        UE_LOG(LogTemp, Log, TEXT("[Loading] Engine returned to a map after listen failure — retrying host."));
        StartHostSequence(PendingHostMaxPlayers);
        return;
    }

    // Le nouveau monde tourne normalement à partir d'ici.
    // 1) On affiche un widget UMG classique (le MoviePlayer ne peut plus
    //    être contrôlé une fois le load fini de façon fiable).
    ShowLoadingWidget();

    // 2) On coupe le MoviePlayer : l'UMG prend le relais sans coupure visible.
    if (IGameMoviePlayer* MoviePlayer = GetMoviePlayer())
    {
        MoviePlayer->StopMovie();
    }
    MovieLoadingWidget = nullptr; // plus référencé par le MoviePlayer.

    // 3) On attend que le pawn local soit réellement là (= prêt à se déplacer).
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            PawnReadyTimerHandle, this,
            &UProjetM1S2GameInstance::CheckPawnReady,
            0.1f, true); // check toutes les 100ms

        // Sécurité : on ne laisse jamais le joueur bloqué indéfiniment.
        World->GetTimerManager().SetTimer(
            LoadingTimeoutHandle, this,
            &UProjetM1S2GameInstance::FinishLoading,
            LoadingTimeoutSeconds, false);
    }

    UE_LOG(LogTemp, Log, TEXT("[Loading] Map loaded, waiting for local pawn..."));
}

void UProjetM1S2GameInstance::CheckPawnReady()
{
    // "Prêt à se déplacer" = le PC local possède un pawn valide.
    // Côté client, c'est ce qui arrive en dernier (réplication réseau),
    // donc c'est le bon signal pour dire "le chargement est vraiment fini".
    APlayerController* PC = GetFirstLocalPlayerController();
    if (PC && PC->GetPawn() != nullptr)
    {
        UE_LOG(LogTemp, Log, TEXT("[Loading] Local pawn ready — finishing loading."));
        FinishLoading();
    }
}

void UProjetM1S2GameInstance::FinishLoading()
{
    bLoadingFlowActive = false;

    // Fin du flux : on remet à zéro l'état du host (succès ou échec définitif),
    // sinon le prochain host hériterait d'un retry déjà consommé ou en attente.
    bHostFlowActive = false;
    bRetryHostPending = false;
    HostListenRetriesRemaining = 0;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PawnReadyTimerHandle);
        World->GetTimerManager().ClearTimer(LoadingTimeoutHandle);
    }

    // Coupe le MoviePlayer au cas où on arrive ici sans être passé par OnPostLoadMap
    // (ex: échec de création/join avant tout LoadMap).
    if (IGameMoviePlayer* MoviePlayer = GetMoviePlayer())
    {
        MoviePlayer->StopMovie();
    }
    MovieLoadingWidget = nullptr;

    RemoveLoadingWidget();
}

UUserWidget* UProjetM1S2GameInstance::CreateLoadingWidget()
{
    if (!LoadingWidgetClass)
    {
        return nullptr;
    }

    // De préférence depuis le PC local (contexte joueur correct pour le viewport).
    // Sinon (pas encore de PC, ex: client en cours de connexion) : la GameInstance.
    if (APlayerController* PC = GetFirstLocalPlayerController())
    {
        return CreateWidget<UUserWidget>(PC, LoadingWidgetClass);
    }
    return CreateWidget<UUserWidget>(this, LoadingWidgetClass);
}

void UProjetM1S2GameInstance::ShowLoadingWidget()
{
    // Si un widget est déjà affiché, on ne le double pas.
    if (ActiveLoadingWidget && ActiveLoadingWidget->IsInViewport())
    {
        return;
    }

    RemoveLoadingWidget(); // nettoie un éventuel ancien widget orphelin.

    ActiveLoadingWidget = CreateLoadingWidget();
    if (ActiveLoadingWidget)
    {
        ActiveLoadingWidget->AddToViewport(LoadingWidgetZOrder);
    }
}

void UProjetM1S2GameInstance::RemoveLoadingWidget()
{
    if (ActiveLoadingWidget)
    {
        ActiveLoadingWidget->RemoveFromParent();
        ActiveLoadingWidget = nullptr;
    }
}

void UProjetM1S2GameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
    UE_LOG(LogTemp, Warning, TEXT("[Loading] Travel failure (%d): %s — removing loading screen."),
           (int32)FailureType, *ErrorString);
    FinishLoading();
}

void UProjetM1S2GameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
    // Cas spécifique : on hostait et le listen a échoué parce que le socket Steam
    // de l'ancien serveur (P2P vport 17777) n'était pas encore libéré. Le moteur
    // va retomber sur une map (menu) — ce qui libère justement le socket. On
    // arme alors UNE nouvelle tentative, déclenchée dans OnPostLoadMap.
    if (FailureType == ENetworkFailure::NetDriverListenFailure
        && bHostFlowActive
        && HostListenRetriesRemaining > 0)
    {
        HostListenRetriesRemaining--;
        bRetryHostPending = true;
        UE_LOG(LogTemp, Warning,
               TEXT("[Loading] Listen failed (socket busy?): %s — will retry hosting once the engine returns to a map. Keeping loading screen."),
               *ErrorString);
        // On NE retire PAS le loading : le retry enchaîne et le joueur ne voit
        // qu'un seul écran de chargement continu.
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[Loading] Network failure (%d): %s — removing loading screen."),
           (int32)FailureType, *ErrorString);
    bHostFlowActive = false;
    FinishLoading();
}