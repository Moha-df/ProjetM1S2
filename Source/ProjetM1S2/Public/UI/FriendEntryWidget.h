#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FriendEntryWidget.generated.h"

// Forward declarations — on évite d'inclure les headers complets ici
// pour réduire le temps de compilation. C'est une best practice Unreal.
class UImage;
class UTextBlock;
class UButton;
class USizeBox;
class UTexture2D;
class FOnlineFriend;

/**
 * Widget représentant un seul ami dans la liste.
 * Conçu pour être instancié par UFriendListWidget.
 *
 * La hiérarchie UMG attendue (à reproduire dans le BP enfant) :
 *   RootSizeBox (SizeBox)
 *     └─ HorizontalBox
 *          ├─ AvatarSizeBox (SizeBox)
 *          │    └─ AvatarImage (Image)
 *          ├─ FriendNameText (TextBlock)
 *          └─ InviteSizeBox (SizeBox)
 *               └─ InviteButton (Button)
 */
UCLASS()
class PROJETM1S2_API UFriendEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * Initialise l'entrée avec les données d'un ami.
     * Appelée par UFriendListWidget après création du widget.
     *
     * @param InFriendId    L'ID Steam unique de l'ami (FUniqueNetId sous forme string).
     * @param InDisplayName Nom à afficher.
     * @param bInIsOnline   Si false, le widget sera grisé.
     */
    void SetupFriend(const FString& InFriendId, const FString& InDisplayName, bool bInIsOnline, UTexture2D* InAvatar);

    /** Getter pour permettre au FriendListWidget de trier les entrées. */
    FORCEINLINE bool IsOnline() const { return bIsOnline; }
    FORCEINLINE const FString& GetFriendId() const { return FriendId; }

protected:
    // NativeConstruct = équivalent C++ du "Event Construct" en Blueprint.
    // Appelé une fois quand le widget est ajouté à la viewport.
    virtual void NativeConstruct() override;

    // === Widgets bindés depuis le Blueprint enfant ===
    // meta = (BindWidget) force le BP à avoir un widget enfant de ce type
    // et de ce nom exact. Sinon : erreur de compilation BP.
    
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> AvatarImage;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> FriendNameText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> InviteButton;

    // SizeBox optionnels — on les bind si on veut y toucher en C++
    // (ex: ajuster la taille selon résolution). meta = (BindWidgetOptional)
    // les rend non-obligatoires côté BP.
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> AvatarSizeBox;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> InviteSizeBox;

private:
    /** Callback du bouton "Inviter". */
    UFUNCTION()
    void OnInviteClicked();

    /** Données de l'ami. */
    FString FriendId;
    FString DisplayName;
    bool bIsOnline = false;
};