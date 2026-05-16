#include "ProjetM1S2GameInstance.h"

#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Online/OnlineSessionNames.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

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

    Super::Shutdown();
}

void UProjetM1S2GameInstance::HostGame()
{
    // Pour tester rapidement depuis la console : "HostGame" (touche ~).
    HostSession();
}

void UProjetM1S2GameInstance::HostSession(int32 MaxPlayers)
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] HostSession: no OSS."));
        return;
    }

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] HostSession: invalid session interface."));
        return;
    }

    IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] HostSession: invalid identity interface."));
        return;
    }

    const FUniqueNetIdPtr LocalUserId = Identity->GetUniquePlayerId(0);
    if (!LocalUserId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] HostSession: invalid local user id."));
        return;
    }

    // Si une session existe déjà, on la détruit d'abord (sinon CreateSession échoue).
    if (Sessions->GetNamedSession(NAME_GameSession))
    {
        UE_LOG(LogTemp, Log, TEXT("[GameInstance] Destroying existing session before creating new one."));
        Sessions->DestroySession(NAME_GameSession);
    }

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
        return;
    }

    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub) return;

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid()) return;

    // GetResolvedConnectString donne l'adresse de connexion (IP:port ou Steam ID).
    FString ConnectString;
    if (!Sessions->GetResolvedConnectString(NAME_GameSession, ConnectString))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Failed to resolve connect string."));
        return;
    }

    APlayerController* PC = GetFirstLocalPlayerController();
    if (!PC) return;

    UE_LOG(LogTemp, Log, TEXT("[GameInstance] Traveling to %s"), *ConnectString);
    PC->ClientTravel(ConnectString, TRAVEL_Absolute);
}