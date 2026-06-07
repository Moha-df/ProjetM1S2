#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SessionListWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class USessionEntryWidget;

/**
 * Widget qui affiche la liste des parties multijoueur joignables.
 * Cherche les sessions ouvertes via l'OnlineSubsystem (Steam) et n'affiche
 * que celles qui n'ont qu'un seul joueur (configurable).
 * Se rafraîchit automatiquement à intervalle régulier.
 *
 * Pendant pour USessionEntryWidget de ce que UFriendListWidget est pour
 * UFriendEntryWidget.
 *
 * Hiérarchie UMG attendue :
 *   RootSizeBox (SizeBox)
 *     └─ RootBorder (Border)
 *          └─ MainVerticalBox (VerticalBox)
 *               ├─ HeaderBorder (Border)
 *               │    └─ TitleText (TextBlock)
 *               └─ SessionsScrollBox (ScrollBox)
 *                    └─ SessionContainer (VerticalBox)
 */
UCLASS()
class PROJETM1S2_API USessionListWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Force un rafraîchissement immédiat de la liste. Utile depuis BP. */
    UFUNCTION(BlueprintCallable, Category = "Sessions")
    void RefreshSessionList();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Widget container où on spawn les SessionEntryWidget. */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UVerticalBox> SessionContainer;

    /** Titre optionnel ("Parties disponibles"). */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TitleText;

    /**
     * Texte optionnel affiché quand aucune partie n'est trouvée.
     * (ex: "Aucune partie disponible"). Caché s'il y a des résultats.
     */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> EmptyText;

    /**
     * Classe du widget d'entrée à instancier.
     * À assigner au WBP_SessionEntry dans le BP enfant.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sessions")
    TSubclassOf<USessionEntryWidget> SessionEntryWidgetClass;

    /** Intervalle (en secondes) entre deux refresh automatiques. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sessions", meta = (ClampMin = "5.0"))
    float RefreshInterval = 30.0f;

    /** Nombre maximum de sessions à demander à Steam. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sessions", meta = (ClampMin = "1"))
    int32 MaxSearchResults = 100;

    /**
     * Si true : n'affiche que les parties qui n'ont qu'un seul joueur.
     * Si false : affiche toutes les parties qui ont encore de la place.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sessions")
    bool bOnlyShowSinglePlayerSessions = true;

private:
    /** Lance la recherche de sessions auprès de l'OSS. */
    void RequestSessions();

    /** Callback appelé par l'OSS quand la recherche est terminée. */
    void OnFindSessionsComplete(bool bWasSuccessful);

    /** Reconstruit le contenu UI à partir des résultats de recherche. */
    void PopulateSessionList();

    /** Vide le container avant un refresh. */
    void ClearSessionList();

    /** Timer pour le refresh périodique. */
    FTimerHandle RefreshTimerHandle;

    /**
     * Objet qui contient les résultats de recherche.
     * Doit rester en vie tant qu'on veut rejoindre une de ses sessions,
     * d'où le membre partagé.
     */
    TSharedPtr<class FOnlineSessionSearch> SessionSearch;

    /** Handle du délégué de fin de recherche, pour unbind proprement. */
    FDelegateHandle FindSessionsCompleteHandle;

    /** Évite de lancer plusieurs recherches en parallèle. */
    bool bSearchInProgress = false;
};
