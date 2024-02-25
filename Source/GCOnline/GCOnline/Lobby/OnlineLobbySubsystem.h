// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "Type/OnlineServiceContextTypes.h"
#include "Type/OnlineLobbyAttributeTypes.h"
#include "Type/OnlineLobbyCreateTypes.h"
#include "Type/OnlineLobbyJoinTypes.h"
#include "Type/OnlineLobbySearchTypes.h"

// OSSv2
#include "Online/OnlineAsyncOpHandle.h"

#include "OnlineLobbySubsystem.generated.h"

///////////////////////////////////////////////////

namespace UE::Online
{
    using IOnlineServicesPtr = TSharedPtr<class IOnlineServices>;
    using ILobbiesPtr = TSharedPtr<class ILobbies>;

    template <typename OpType>

    class TOnlineResult;
}
using namespace UE::Online;

class UOnlineServiceSubsystem;
class ULobbyResult;

///////////////////////////////////////////////////

/**
 * Event triggered when the local user has requested to join a lobby from an external source, for example from a platform overlay.
 * Generally, the game should transition the player into the lobby.
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FLobbyMemberChangedDelegate
    , FName					/* LocalName */
    , int32					/* CurrentMembers */
    , int32					/* MaxMembers */);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLobbyMemberChangedDynamicDelegate
    , FName, LocalName
    , int32, CurrentMembers
    , int32, MaxMembers);


/**
 * Subsystem with features to extend the functionality of Online Servicies (OSSv2) and make it easier to use in projects
 * This subsystem handles the functionality to create and participate in online play matches, standby, and party lobbies.
 * 
 * Include OSSv2 Interface Feature:
 *  - lobbies Interface
 */
UCLASS(BlueprintType)
class GCONLINE_API UOnlineLobbySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UOnlineLobbySubsystem() {}

    ///////////////////////////////////////////////////////////////////////
    // Initialization
protected:
    //
    // True if this is a dedicated server, which doesn't require a LocalPlayer
    //
    bool bIsDedicatedServer{ false };

    TArray<FOnlineEventDelegateHandle> LobbyDelegateHandles;

    UPROPERTY(Transient)
    TObjectPtr<UOnlineServiceSubsystem> OnlineServiceSubsystem{ nullptr };

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
    void BindLobbiesDelegates();
    void UnbindLobbiesDelegates();

    /**
     * Returns lobbies interface of specific type, will return null if there is no type
     */
    ILobbiesPtr GetLobbiesInterface(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;


    //////////////////////////////////////////////////////////////////////
    // Lobby Events
protected:
    void HandleUserJoinLobbyRequest(const FUILobbyJoinRequested& EventParams);
    void HandleLobbyMemberJoined(const FLobbyMemberJoined& EventParams);
    void HandleLobbyMemberLeft(const FLobbyMemberLeft& EventParams);


    //////////////////////////////////////////////////////////////////////
    // Create Lobby
protected:
    UPROPERTY(Transient)
    TObjectPtr<ULobbyCreateRequest> OngoingCreateRequest{ nullptr };

public:
    /**
     * Creates a LobbyCreateRequest with default options for online games, this can be modified after creation
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual ULobbyCreateRequest* CreateOnlineLobbyCreateRequest();

    /**
     * Creates a new online game using the lobby request information
     */
    virtual bool CreateLobby(
        APlayerController* HostingPlayer
        , ULobbyCreateRequest* CreateRequest
        , FLobbyCreateCompleteDelegate Delegate = FLobbyCreateCompleteDelegate());

protected:
    void CreateOnlineLobbyInternal(
        ULocalPlayer* LocalPlayer
        , ULobbyCreateRequest* CreateRequest
        , FLobbyCreateCompleteDelegate Delegate = FLobbyCreateCompleteDelegate());

    virtual void HandleCreateOnlineLobbyComplete(
        const TOnlineResult<FCreateLobby>& CreateResult
        , FLobbyCreateCompleteDelegate Delegate);


    //////////////////////////////////////////////////////////////////////
    // Search Lobby 
protected:
	UPROPERTY(Transient)
    TObjectPtr<ULobbySearchRequest> OngoingSearchRequest{ nullptr };

public:
    /**
     * Creates a LobbySearchRequest with default options for online games, this can be modified after creation
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual ULobbySearchRequest* CreateOnlineLobbySearchRequest();

    /**
     * Queries online system for the list of joinable lobbys matching the search request 
     */
    virtual bool SearchLobby(
        APlayerController* SearchingPlayer
        , ULobbySearchRequest* SearchRequest
        , FLobbySearchCompleteDelegate Delegate = FLobbySearchCompleteDelegate());

protected:
    void SearchOnlineLobbyInternal(
        ULocalPlayer* LocalPlayer
        , ULobbySearchRequest* SearchRequest
        , FLobbySearchCompleteDelegate Delegate = FLobbySearchCompleteDelegate());

    virtual void HandleSearchOnlineLobbyComplete(
        const TOnlineResult<FFindLobbies>& SearchResult
        , FLobbySearchCompleteDelegate Delegate);


    //////////////////////////////////////////////////////////////////////
    // Join Lobby
protected:
    //
    // List of lobbies currently participating as hosts or guests
    // 
    // Key   : Lobby's Local Name
    // Value : Lobby info
    //
    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<ULobbyResult>> JoiningLobbies;

    UPROPERTY(Transient)
    TObjectPtr<ULobbyJoinRequest> OngoingJoinRequest{ nullptr };

public:
    /**
     * Get a lobby that you have already joined as a current host or guest
     */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lobby")
    virtual const ULobbyResult* GetJoinedLobby(FName LocalName) const;

protected:
    virtual void AddJoiningLobby(ULobbyResult* InLobbyResult);
    virtual void RemoveJoiningLobby(ULobbyResult* InLobbyResult);
    virtual void RemoveJoiningLobby(FName LobbyLocalName);


public:
    /**
     * Creates a LobbyJoinRequest with default options for online games, this can be modified after creation
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual ULobbyJoinRequest* CreateOnlineLobbyJoinRequest(ULobbyResult* LobbyResult);

    /**
     * Starts process to join an existing lobby, if successful this will connect to the specified server
     */
    virtual bool JoinLobby(
        APlayerController* JoiningPlayer
        , ULobbyJoinRequest* JoinRequest
        , FLobbyJoinCompleteDelegate Delegate = FLobbyJoinCompleteDelegate());

protected:
    void JoinOnlineLobbyInternal(
        ULocalPlayer* LocalPlayer
        , ULobbyJoinRequest* JoinRequest
        , FLobbyJoinCompleteDelegate Delegate = FLobbyJoinCompleteDelegate());

    virtual void HandleJoinOnlineLobbyComplete(
        const TOnlineResult<FJoinLobby>& JoinResult
        , FAccountId JoiningAccountId
        , FLobbyJoinCompleteDelegate Delegate);

    /**
     * Create a URL for lobby travel to a joined lobby
     */
    FString ConstructJoiningLobbyTravelURL(const FAccountId& AccountId, const FLobbyId& LobbyId);


    //////////////////////////////////////////////////////////////////////
    // Clean Up Lobby
public:
    /**
     * Clean up all active lobbies, called from cases like returning to the main menu
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual void CleanUpAllLobbies(const APlayerController* InPlayerController = nullptr);

    /**
     * Clean up specific active lobbys, called from cases like returning to the main menu
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual void CleanUpLobby(FName LocalName, const APlayerController* InPlayerController = nullptr);

protected:
    virtual void CleanUpOngoingRequest();


    //////////////////////////////////////////////////////////////////////
    // Join Lobby Request
public:
    UPROPERTY(BlueprintAssignable, Category = "Lobby", meta = (DisplayName = "On User Join Lobby Request"))
    FUserJoinLobbyRequestDynamicDelegate K2_OnUserJoinLobbyRequest;
    FUserJoinLobbyRequestDelegate OnUserJoinLobbyRequest;

protected:
    void NotifyUserJoinLobbyRequest(
        const FPlatformUserId& LocalPlatformUserId
        , ULobbyResult* RequestedLobby
        , FOnlineServiceResult Result);


    //////////////////////////////////////////////////////////////////////
    // Lobby Member Change
public:
    UPROPERTY(BlueprintAssignable, Category = "Lobby", meta = (DisplayName = "On Lobby Member Changed"))
    FLobbyMemberChangedDynamicDelegate K2_OnLobbyMemberChanged;
    FLobbyMemberChangedDelegate OnLobbyMemberChanged;

protected:
    void NotifyLobbyMemberChanged(FName LocalName, int32 CurrentMembers, int32 MaxMembers);


    //////////////////////////////////////////////////////////////////////
    // Travel Lobby
public:
	UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual bool TravelToLobby(APlayerController* InPlayerController, const ULobbyResult* LobbyResult);

};
