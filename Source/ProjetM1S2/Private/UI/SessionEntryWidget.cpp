#include "UI/SessionEntryWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemUtils.h"

#include "ProjetM1S2GameInstance.h"

void USessionEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // On bind le clic du bouton UNE seule fois ici.
    // On vérifie d'abord IsAlreadyBound pour éviter les doubles bindings
    // si NativeConstruct est rappelé.
    if (JoinButton && !JoinButton->OnClicked.IsAlreadyBound(this, &USessionEntryWidget::OnJoinClicked))
    {
        JoinButton->OnClicked.AddDynamic(this, &USessionEntryWidget::OnJoinClicked);
    }
}

void USessionEntryWidget::SetupSession(const FString& InHostName, int32 InCurrentPlayers, int32 InMaxPlayers,
                                       const FOnlineSessionSearchResult& InResult)
{
    HostName = InHostName;
    SearchResult = InResult;

    if (SessionNameText)
    {
        SessionNameText->SetText(FText::FromString(HostName));
    }

    if (PlayerCountText)
    {
        // Affiche "1/4" par exemple.
        PlayerCountText->SetText(FText::FromString(
            FString::Printf(TEXT("%d/%d"), InCurrentPlayers, InMaxPlayers)));
    }
}

void USessionEntryWidget::OnJoinClicked()
{
    // 1. Récupérer le sous-système online (Steam si configuré dans DefaultEngine.ini).
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionEntry] No OnlineSubsystem available."));
        return;
    }

    // 2. Récupérer l'interface Session.
    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionEntry] Session interface invalid."));
        return;
    }

    // 3. Récupérer l'ID local du joueur (celui qui rejoint).
    IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionEntry] Identity interface invalid."));
        return;
    }

    const FUniqueNetIdPtr LocalUserId = Identity->GetUniquePlayerId(0); // 0 = premier local player
    if (!LocalUserId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionEntry] Local user ID invalid."));
        return;
    }

    // 4. Vérifier que le résultat est toujours valide (la session a pu disparaître).
    if (!SearchResult.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionEntry] Search result no longer valid."));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("Partie indisponible"));
        }
        return;
    }

    // 5. Rejoindre la session.
    // NAME_GameSession est le nom standard de la session de gameplay dans UE5.
    // IMPORTANT : on ne gère PAS la complétion ici. Le GameInstance
    // (UProjetM1S2GameInstance) est déjà abonné à OnJoinSessionComplete et
    // s'occupe du ClientTravel vers le host. On réutilise donc sa logique.
    UE_LOG(LogTemp, Log, TEXT("[SessionEntry] Joining session hosted by %s..."), *HostName);

    // Loading instantané dès le clic (avant le join async + le ClientTravel).
    // Le GameInstance gère tout l'écran de chargement jusqu'à ce que le pawn
    // soit prêt dans la partie du host.
    if (UProjetM1S2GameInstance* GI = GetGameInstance<UProjetM1S2GameInstance>())
    {
        GI->StartLoadingFlow();
    }

    Sessions->JoinSession(*LocalUserId, NAME_GameSession, SearchResult);
}
