#include "UI/FriendEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemUtils.h"

void UFriendEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // On bind le clic du bouton UNE seule fois ici.
    // Important : on vérifie d'abord IsAlreadyBound pour éviter
    // les doubles bindings si NativeConstruct est rappelé.
    if (InviteButton && !InviteButton->OnClicked.IsAlreadyBound(this, &UFriendEntryWidget::OnInviteClicked))
    {
        InviteButton->OnClicked.AddDynamic(this, &UFriendEntryWidget::OnInviteClicked);
    }
}

void UFriendEntryWidget::SetupFriend(const FString& InFriendId, const FString& InDisplayName, bool bInIsOnline, UTexture2D* InAvatar)
{
    FriendId = InFriendId;
    DisplayName = InDisplayName;
    bIsOnline = bInIsOnline;

    if (FriendNameText)
    {
        FriendNameText->SetText(FText::FromString(DisplayName));
    }
    
    // === Gestion de l'avatar ===
    // Si on a une texture valide, on l'assigne. Sinon, on laisse le brush
    // par défaut défini dans le BP (image placeholder). C'est plus propre
    // que de mettre du code conditionnel dans le BP.
    if (AvatarImage && InAvatar)
    {
        // SetBrushFromTexture gère automatiquement la création du brush
        // avec les bonnes dimensions. Le second paramètre (bMatchSize) à false
        // veut dire qu'on garde la taille définie par le SizeBox parent,
        // sinon l'image se redimensionnerait selon les pixels de la texture.
        AvatarImage->SetBrushFromTexture(InAvatar, false);
    }

    // Effet visuel : si offline, on réduit l'opacité du widget entier.
    // SetRenderOpacity affecte tout le widget et ses enfants. C'est plus
    // performant que de toucher chaque sous-widget individuellement.
    SetRenderOpacity(bIsOnline ? 1.0f : 0.4f);

    // On désactive aussi le bouton si offline — pas de sens d'inviter quelqu'un offline.
    if (InviteButton)
    {
        InviteButton->SetIsEnabled(bIsOnline);
    }
}

void UFriendEntryWidget::OnInviteClicked()
{
    // 1. Récupérer le sous-système online (Steam si configuré dans DefaultEngine.ini).
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FriendEntry] No OnlineSubsystem available."));
        return;
    }

    // 2. Récupérer l'interface Session (c'est elle qui gère les invitations).
    IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
    if (!Sessions.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[FriendEntry] Session interface invalid."));
        return;
    }

    // 3. Récupérer l'ID local du joueur (celui qui invite).
    IOnlineIdentityPtr Identity = OnlineSub->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[FriendEntry] Identity interface invalid."));
        return;
    }

    const FUniqueNetIdPtr LocalUserId = Identity->GetUniquePlayerId(0); // 0 = premier local player
    if (!LocalUserId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[FriendEntry] Local user ID invalid."));
        return;
    }

    // 4. Convertir l'ID string de l'ami en FUniqueNetId.
    const FUniqueNetIdPtr FriendNetId = Identity->CreateUniquePlayerId(FriendId);
    if (!FriendNetId.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[FriendEntry] Could not create friend net id from %s"), *FriendId);
        return;
    }

    // 5. Envoyer l'invitation.
    // NAME_GameSession est le nom standard de la session de gameplay dans UE5.
    const bool bInviteSent = Sessions->SendSessionInviteToFriend(*LocalUserId, NAME_GameSession, *FriendNetId);

    // 6. Print de confirmation (comme tu l'as demandé).
    if (bInviteSent)
    {
        UE_LOG(LogTemp, Log, TEXT("[FriendEntry] Invite sent to %s (%s)"), *DisplayName, *FriendId);

        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
                FString::Printf(TEXT("Invitation envoyée à %s"), *DisplayName));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[FriendEntry] Failed to send invite to %s"), *DisplayName);
        
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
                FString::Printf(TEXT("Failed to send invite à %s"), *DisplayName));
        }
    }
}