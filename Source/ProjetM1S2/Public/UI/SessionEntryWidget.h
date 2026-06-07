#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
// OnlineSessionSettings.h contient la DÉFINITION complète de
// FOnlineSessionSearchResult (l'interface ne fait que la forward-declarer),
// nécessaire car on en stocke une copie par valeur ci-dessous.
#include "OnlineSessionSettings.h"
#include "SessionEntryWidget.generated.h"

// Forward declarations — on évite d'inclure les headers complets ici
// pour réduire le temps de compilation. C'est une best practice Unreal.
class UTextBlock;
class UButton;
class USizeBox;

/**
 * Widget représentant une seule partie (session) joignable dans la liste.
 * Conçu pour être instancié par USessionListWidget.
 *
 * La hiérarchie UMG attendue (à reproduire dans le BP enfant) :
 *   RootSizeBox (SizeBox)
 *     └─ HorizontalBox
 *          ├─ SessionNameText (TextBlock)   -> nom du host
 *          ├─ PlayerCountText (TextBlock)   -> "1/4"
 *          └─ JoinSizeBox (SizeBox)
 *               └─ JoinButton (Button)
 */
UCLASS()
class PROJETM1S2_API USessionEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * Initialise l'entrée avec les données d'une session trouvée.
     * Appelée par USessionListWidget après création du widget.
     *
     * @param InHostName       Nom du joueur qui héberge.
     * @param InCurrentPlayers Nombre de joueurs actuellement dans la partie.
     * @param InMaxPlayers     Nombre maximum de joueurs.
     * @param InResult         Le résultat de recherche brut, nécessaire pour rejoindre.
     */
    void SetupSession(const FString& InHostName, int32 InCurrentPlayers, int32 InMaxPlayers,
                      const FOnlineSessionSearchResult& InResult);

protected:
    // NativeConstruct = équivalent C++ du "Event Construct" en Blueprint.
    virtual void NativeConstruct() override;

    // === Widgets bindés depuis le Blueprint enfant ===
    // meta = (BindWidget) force le BP à avoir un widget enfant de ce type
    // et de ce nom exact. Sinon : erreur de compilation BP.

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SessionNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> PlayerCountText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> JoinButton;

    // SizeBox optionnel — bind si on veut y toucher en C++.
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> JoinSizeBox;

private:
    /** Callback du bouton "Rejoindre". */
    UFUNCTION()
    void OnJoinClicked();

    /** Données affichées. */
    FString HostName;

    /**
     * Le résultat de recherche brut.
     * On en garde une COPIE car JoinSession a besoin de l'objet complet
     * (adresse de connexion, settings, etc.), pas juste d'un ID.
     * FOnlineSessionSearchResult est copiable, donc c'est sûr.
     */
    FOnlineSessionSearchResult SearchResult;
};
