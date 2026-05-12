#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "FriendListWidget.generated.h"

class UVerticalBox;
class UScrollBox;
class UTextBlock;
class UFriendEntryWidget;

/**
 * Widget qui affiche la liste complète des amis Steam.
 * Se rafraîchit automatiquement à intervalle régulier.
 *
 * Hiérarchie UMG attendue :
 *   RootSizeBox (SizeBox)
 *     └─ RootBorder (Border)
 *          └─ MainVerticalBox (VerticalBox)
 *               ├─ HeaderBorder (Border)
 *               │    └─ TitleText (TextBlock)
 *               └─ FriendsScrollBox (ScrollBox)
 *                    └─ FriendContainer (VerticalBox)
 */
UCLASS()
class PROJETM1S2_API UFriendListWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Force un rafraîchissement immédiat de la liste. Utile depuis BP. */
    UFUNCTION(BlueprintCallable, Category = "Friends")
    void RefreshFriendList();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Widget container où on spawn les FriendEntryWidget. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> FriendContainer;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TitleText;

    /**
     * Classe du widget d'entrée à instancier.
     * Exposée pour qu'on puisse l'assigner au WBP_FriendEntry dans le BP enfant.
     * C'est plus flexible que hardcoder la classe en C++.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Friends")
    TSubclassOf<UFriendEntryWidget> FriendEntryWidgetClass;

    /** Intervalle (en secondes) entre deux refresh automatiques. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Friends", meta = (ClampMin = "5.0"))
    float RefreshInterval = 30.0f;

private:
    /** Demande à Steam la liste des amis. */
    void RequestFriendsList();

    /** Callback appelé par l'OSS quand la liste est prête. */
    void OnFriendsListReadComplete(int32 LocalUserNum, bool bWasSuccessful,
                                   const FString& ListName, const FString& ErrorStr);

    /** Reconstruit le contenu UI à partir des amis cached par l'OSS. */
    void PopulateFriendList();

    /** Vide le container avant un refresh. */
    void ClearFriendList();

    /** Timer pour le refresh périodique. */
    FTimerHandle RefreshTimerHandle;
};