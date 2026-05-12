#include "UI/FriendListWidget.h"
#include "UI/FriendEntryWidget.h"

#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Interfaces/OnlinePresenceInterface.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"

#include "AdvancedSteamFriendsLibrary.h"

void UFriendListWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Premier refresh à l'ouverture du widget.
    RefreshFriendList();

    // Refresh périodique. On utilise le TimerManager du World plutôt qu'un
    // FTSTicker ou un Tick(), car c'est plus simple à gérer (auto-cleanup
    // si le world meurt) et performant (pas de Tick par frame).
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            RefreshTimerHandle,
            this,
            &UFriendListWidget::RefreshFriendList,
            RefreshInterval,
            true // bLoop
        );
    }
}

void UFriendListWidget::NativeDestruct()
{
    // Cleanup obligatoire — sinon le timer continue de tourner même après
    // que le widget soit retiré, ce qui est un memory leak / crash potentiel.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RefreshTimerHandle);
    }

    Super::NativeDestruct();
}

void UFriendListWidget::RefreshFriendList()
{
    RequestFriendsList();
}

void UFriendListWidget::RequestFriendsList()
{
    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FriendList] No OnlineSubsystem."));
        return;
    }

    IOnlineFriendsPtr Friends = OnlineSub->GetFriendsInterface();
    if (!Friends.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[FriendList] Friends interface invalid."));
        return;
    }

    // ReadFriendsList est ASYNCHRONE — Steam doit aller chercher les données.
    // On passe un délégué qui sera appelé quand c'est prêt.
    // EFriendsLists::Default = tous les amis Steam (online + offline).
    const FOnReadFriendsListComplete Delegate = FOnReadFriendsListComplete::CreateUObject(
        this, &UFriendListWidget::OnFriendsListReadComplete);

    Friends->ReadFriendsList(0, EFriendsLists::ToString(EFriendsLists::Default), Delegate);
}

void UFriendListWidget::OnFriendsListReadComplete(int32 LocalUserNum, bool bWasSuccessful,
                                                  const FString& ListName, const FString& ErrorStr)
{
    if (!bWasSuccessful)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FriendList] Read friends failed: %s"), *ErrorStr);
        return;
    }

    PopulateFriendList();
}

void UFriendListWidget::ClearFriendList()
{
    if (FriendContainer)
    {
        FriendContainer->ClearChildren();
    }
}

void UFriendListWidget::PopulateFriendList()
{
    if (!FriendContainer || !FriendEntryWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FriendList] Missing FriendContainer or EntryClass."));
        return;
    }

    IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
    if (!OnlineSub) return;

    IOnlineFriendsPtr Friends = OnlineSub->GetFriendsInterface();
    if (!Friends.IsValid()) return;

    // Récupérer la liste cached (après le ReadFriendsList).
    TArray<TSharedRef<FOnlineFriend>> FriendsArray;
    Friends->GetFriendsList(0, EFriendsLists::ToString(EFriendsLists::Default), FriendsArray);

    // === Tri : online d'abord (par nom), offline ensuite (par nom) ===
    // En prod on évite de re-trier à chaque refresh si la liste est énorme,
    // mais pour une friend list (rarement >200 amis) c'est négligeable.
    FriendsArray.Sort([](const TSharedRef<FOnlineFriend>& A, const TSharedRef<FOnlineFriend>& B)
    {
        const bool bAOnline = A->GetPresence().bIsOnline;
        const bool bBOnline = B->GetPresence().bIsOnline;

        if (bAOnline != bBOnline)
        {
            return bAOnline; // online en premier (true > false)
        }
        // Même statut → tri alphabétique
        return A->GetDisplayName().Compare(B->GetDisplayName(), ESearchCase::IgnoreCase) < 0;
    });

    // Vider l'ancien contenu APRÈS avoir reçu les nouvelles données.
    // Évite un flash visuel vide entre le clear et le repopulate.
    ClearFriendList();

    // Spawn les widgets d'entrée.
    for (const TSharedRef<FOnlineFriend>& Friend : FriendsArray)
    {
        UFriendEntryWidget* EntryWidget = CreateWidget<UFriendEntryWidget>(this, FriendEntryWidgetClass);
        if (!EntryWidget) continue;

        const FString FriendIdStr = Friend->GetUserId()->ToString();
        const FString DisplayName = Friend->GetDisplayName();
        const bool bIsOnline = Friend->GetPresence().bIsOnline;
        
        FBPUniqueNetId BPNetId;
        BPNetId.SetUniqueNetId(Friend->GetUserId());
        
        EBlueprintAsyncResultSwitch AvatarResult = EBlueprintAsyncResultSwitch::OnFailure;

        UTexture2D* AvatarTexture = UAdvancedSteamFriendsLibrary::GetSteamFriendAvatar(
            BPNetId, 
            AvatarResult, 
            SteamAvatarSize::SteamAvatar_Medium);
        
        switch (AvatarResult)
        {
        case EBlueprintAsyncResultSwitch::OnSuccess:
            // Avatar OK, rien à faire de spécial
            break;
        case EBlueprintAsyncResultSwitch::AsyncLoading:
            UE_LOG(LogTemp, Verbose, TEXT("[FriendList] Avatar for %s still loading, will retry next refresh."), 
                   *DisplayName);
            break;
        case EBlueprintAsyncResultSwitch::OnFailure:
            UE_LOG(LogTemp, Verbose, TEXT("[FriendList] Avatar for %s failed to load."), *DisplayName);
            break;
        }

        EntryWidget->SetupFriend(FriendIdStr, DisplayName, bIsOnline, AvatarTexture);
        FriendContainer->AddChild(EntryWidget);
    }

    UE_LOG(LogTemp, Log, TEXT("[FriendList] Populated %d friends."), FriendsArray.Num());
}