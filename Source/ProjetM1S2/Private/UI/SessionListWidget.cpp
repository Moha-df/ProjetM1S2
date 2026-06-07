#include "UI/SessionListWidget.h"
#include "UI/SessionEntryWidget.h"

#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"

void USessionListWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Premier refresh à l'ouverture du widget.
    RefreshSessionList();

    // Refresh périodique via le TimerManager du World (auto-cleanup si le
    // world meurt, et pas de coût par frame comme un Tick()).
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            RefreshTimerHandle,
            this,
            &USessionListWidget::RefreshSessionList,
            RefreshInterval,
            true // bLoop
        );
    }
}

void USessionListWidget::NativeDestruct()
{
    // Cleanup du timer — sinon il continue de tourner après le retrait du widget.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RefreshTimerHandle);
    }

    // Unbind du délégué de recherche s'il est encore enregistré.
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (OnlineSub && FindSessionsCompleteHandle.IsValid())
    {
        IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
        if (Sessions.IsValid())
        {
            Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
        }
    }

    Super::NativeDestruct();
}

void USessionListWidget::RefreshSessionList()
{
    RequestSessions();
}

void USessionListWidget::RequestSessions()
{
    // On évite de lancer une nouvelle recherche si une est déjà en cours
    // (le timer pourrait sinon en empiler plusieurs).
    if (bSearchInProgress)
    {
        return;
    }

    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionList] No OnlineSubsystem."));
        return;
    }

    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionList] Session interface invalid."));
        return;
    }

    // === Paramètres de recherche ===
    // On doit refléter les settings utilisés à la création de session
    // (voir UProjetM1S2GameInstance::HostSession) : presence + lobbies.
    SessionSearch = MakeShareable(new FOnlineSessionSearch());
    SessionSearch->bIsLanQuery = false;                 // Internet, pas LAN
    SessionSearch->MaxSearchResults = MaxSearchResults;
    // Le host crée un lobby Steam (bUseLobbiesIfAvailable=true), donc on
    // cherche dans les lobbies. C'est suffisant sur Steam.
    SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

    // On (re)bind le délégué de complétion juste avant de lancer la recherche.
    if (FindSessionsCompleteHandle.IsValid())
    {
        Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
    }
    FindSessionsCompleteHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(
            this, &USessionListWidget::OnFindSessionsComplete));

    bSearchInProgress = true;

    UE_LOG(LogTemp, Log, TEXT("[SessionList] Searching for sessions..."));
    Sessions->FindSessions(0, SessionSearch.ToSharedRef());
}

void USessionListWidget::OnFindSessionsComplete(bool bWasSuccessful)
{
    bSearchInProgress = false;

    // Unbind tout de suite — on re-bind à chaque recherche.
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (OnlineSub && FindSessionsCompleteHandle.IsValid())
    {
        IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
        if (Sessions.IsValid())
        {
            Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
        }
        FindSessionsCompleteHandle.Reset();
    }

    if (!bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionList] FindSessions failed."));
        return;
    }

    PopulateSessionList();
}

void USessionListWidget::ClearSessionList()
{
    if (SessionContainer)
    {
        SessionContainer->ClearChildren();
    }
}

void USessionListWidget::PopulateSessionList()
{
    if (!SessionContainer || !SessionEntryWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionList] Missing SessionContainer or EntryClass."));
        return;
    }

    if (!SessionSearch.IsValid())
    {
        return;
    }

    // Vider l'ancien contenu APRÈS avoir reçu les nouvelles données
    // (évite un flash visuel vide entre le clear et le repopulate).
    ClearSessionList();

    int32 ShownCount = 0;

    for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
    {
        if (!Result.IsValid())
        {
            continue;
        }

        const FOnlineSession& Session = Result.Session;

        // Calcul du nombre de joueurs actuels.
        // NumPublicConnections = slots totaux ; NumOpenPublicConnections = slots libres.
        const int32 MaxPlayers = Session.SessionSettings.NumPublicConnections;
        const int32 OpenSlots = Result.Session.NumOpenPublicConnections;
        const int32 CurrentPlayers = FMath::Max(0, MaxPlayers - OpenSlots);

        // Filtre : on ignore les parties pleines (pas de place pour rejoindre).
        if (OpenSlots <= 0)
        {
            continue;
        }

        // Filtre optionnel : ne montrer que les parties avec exactement 1 joueur.
        if (bOnlyShowSinglePlayerSessions && CurrentPlayers != 1)
        {
            continue;
        }

        USessionEntryWidget* EntryWidget = CreateWidget<USessionEntryWidget>(this, SessionEntryWidgetClass);
        if (!EntryWidget)
        {
            continue;
        }

        // Nom du host. OwningUserName est rempli par Steam ; fallback si vide.
        FString HostName = Session.OwningUserName;
        if (HostName.IsEmpty())
        {
            HostName = TEXT("Partie inconnue");
        }

        EntryWidget->SetupSession(HostName, CurrentPlayers, MaxPlayers, Result);
        SessionContainer->AddChild(EntryWidget);
        ++ShownCount;
    }

    // Gestion du texte "aucune partie".
    if (EmptyText)
    {
        EmptyText->SetVisibility(ShownCount > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    UE_LOG(LogTemp, Log, TEXT("[SessionList] Found %d sessions, displayed %d."),
           SessionSearch->SearchResults.Num(), ShownCount);
}
