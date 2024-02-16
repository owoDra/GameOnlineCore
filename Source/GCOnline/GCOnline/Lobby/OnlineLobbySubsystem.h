// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

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

///////////////////////////////////////////////////

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
    // Events
#pragma region Events
public:
    //
    // Native Delegate when a local user has accepted an invite
    // 
    FUserJoinLobbyRequestDelegate OnUserJoinLobbyRequestEvent;

    //
    // Event broadcast when a local user has accepted an invite
    //
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On User Join Requested Lobby"))
    FUserJoinLobbyRequestDynamicDelegate K2_OnUserJoinLobbyRequestEvent;


    //
    // Native Delegate when a JoinLobby call has completed
    //
    FJoinLobbyCompleteDelegate OnJoinLobbyCompleteEvent;

    //
    // Event broadcast when a JoinLobby call has completed
    //
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On Join Lobby Complete"))
    FJoinLobbyCompleteDynamicDelegate K2_OnJoinLobbyCompleteEvent;

    //
    // Native Delegate for modifying the connect URL prior to a client travel
    //
    FLobbyPreClientTravelDelegate OnPreClientTravelEvent;
    
protected:
    void NotifyUserLobbyRequest(const FPlatformUserId& PlatformUserId, ULobbySearchResult* RequestedLobby, const FOnlineServiceResult& RequestedLobbyResult);
    void NotifyJoinLobbyComplete(const FOnlineServiceResult& Result);
   

#pragma endregion


    //////////////////////////////////////////////////////////////////////
    // Create Lobby
public:
    //
    // Native Delegate when a CreateLobby call has completed
    // 
    FLobbyCreateCompleteDelegate OnCreateLobbyCompleteEvent;

    //
    // Event broadcast when a CreateLobby call has completed
    //
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On Create Lobby Complete"))
    FLobbyCreateCompleteDynamicDelegate K2_OnCreateLobbyCompleteEvent;

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
        , ULobbyCreateRequest* HostRequest
        , FLobbyCreateCompleteSingleDelegate Delegate = FLobbyCreateCompleteSingleDelegate());

protected:
    void CreateOnlineLobbyInternal(
        ULocalPlayer* LocalPlayer
        , ULobbyCreateRequest* HostRequest
        , FLobbyCreateCompleteSingleDelegate Delegate = FLobbyCreateCompleteSingleDelegate());

    virtual void HandleCreateOnlineLobbyComplete(
        const TOnlineResult<FCreateLobby>& CreateResult
        , FLobbyCreateCompleteSingleDelegate Delegate);

    void NotifyCreateLobbyComplete(const FOnlineServiceResult& Result);


    //////////////////////////////////////////////////////////////////////
    // Lobby Travel
protected:
    //
    // The travel URL that will be used after lobby operations are complete
    //
    UPROPERTY(Transient)
    FString PendingLobbyTravelURL;

public:
    /**
     * Execute lobby travel if there is currently a pending lobby travel
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual bool StartPendingLobbyTravel();

protected:
    virtual void SetPendingLobbyTravelURL(const FString& URL);
    virtual void ClearPendingLobbyTravelURL();


    //////////////////////////////////////////////////////////////////////
    // Lobby
#pragma region Lobby
protected:
    //
    // Settings for the current search
    //
    TSharedPtr<FLobbySearchSettings> SearchSettings;

    //
    // The travel URL that will be used after lobby operations are complete
    //
	UPROPERTY(Transient)
    FString PendingTravelURL;

    //
    // Most recent result information for a lobby creation attempt, stored here to allow storing error codes for later
    //
    UPROPERTY(Transient)
    FOnlineResultInformation CreateLobbyResult;

    //
    // True if we want to cancel the lobby after it is created
    //
    UPROPERTY(Transient)
    bool bWantToDestroyPendingLobby{ false };

public:
    /** 
     * Creates a host lobby request with default options for online games, this can be modified after creation 
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual UHostLobbyRequest* CreateOnlineHostLobbyRequest();

    /** 
     * Creates a lobby search object with default options to look for default online games, this can be modified after creation 
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual USearchLobbyRequest* CreateOnlineSearchLobbyRequest();

    

    /** 
     * Starts a process to look for existing lobbys or create a new one if no viable lobbys are found 
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual void QuickPlayLobby(APlayerController* JoiningOrHostingPlayer, USearchLobbyRequest* SearchRequest, UHostLobbyRequest* HostRequest);

    /** 
     * Starts process to join an existing lobby, if successful this will connect to the specified server 
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual void JoinLobby(APlayerController* JoiningPlayer, USearchLobbyResult* SearchResult);

    /** 
     * Queries online system for the list of joinable lobbys matching the search request 
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual void FindLobbies(APlayerController* SearchingPlayer, USearchLobbyRequest* SearchRequest);

    /** 
     * Clean up any active lobbys, called from cases like returning to the main menu 
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual void CleanUpLobbies();

#pragma endregion




    //////////////////////////////////////////////////////////////////////
    // Quick Play Lobby
#pragma region Quick Play Lobby
protected:
    /** 
     * Called when a quick play search finishes, can be overridden for game-specific behavior 
     */
    virtual void HandleQuickPlaySearchFinished(bool bSucceeded, const FText& ErrorMessage, TWeakObjectPtr<APlayerController> JoiningOrHostingPlayer, TStrongObjectPtr<UHostLobbyRequest> HostRequest);

#pragma endregion


    //////////////////////////////////////////////////////////////////////
    // Join Lobby
#pragma region Join Lobby
protected:
    void JoinSessionInternal(ULocalPlayer* LocalPlayer, USearchLobbyResult* Request);

#pragma endregion


    //////////////////////////////////////////////////////////////////////
    // Find Lobby
#pragma region Find Lobby
protected:
    void FindLobbiesInternal(APlayerController* SearchingPlayer, const TSharedRef<FLobbySearchSettings>& InSearchSettings);

#pragma endregion


    //////////////////////////////////////////////////////////////////////
    // Travel Lobby
#pragma region Travel Lobby
protected:
    void InternalTravelToLobby(const FName LobbyName);

    /** 
     * Called when traveling to a session fails 
     */
    //virtual void TravelLocalSessionFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ReasonString);

#pragma endregion


    //////////////////////////////////////////////////////////////////////
    // Utilities
protected:
    /** 
     * Get the local user id for a given controller 
     */
    FAccountId GetAccountId(APlayerController* PlayerController) const;

    /** 
     * Get the lobby id for a given lobby name 
     */
    FLobbyId GetLobbyId(const FName LobbyName) const;
    FLobbyId GetLobbyId(const FName& LobbyName, const FAccountId& AccountId) const;

};
