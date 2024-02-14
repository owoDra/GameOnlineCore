// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "Type/OnlineServiceTypes.h"
#include "Type/OnlineLobbyTypes.h"

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

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;


    ///////////////////////////////////////////////////////////////////////
    // Context Cache
#pragma region Context Cache
protected:
    /**
     * Internal structure of cache information for pointers to use the Auth interface in this subsystem.
     */
    struct FOnlineServiceContextCache
    {
    public:
        FOnlineServiceContextCache() = default;

        FOnlineServiceContextCache(const IOnlineServicesPtr& InOnlineServices)
            : OnlineServices(InOnlineServices)
        {
            check(OnlineServices.IsValid());
        }

        ~FOnlineServiceContextCache() { Reset(); }

    public:
        //
        // Online services, accessor to specific services
        //
        IOnlineServicesPtr OnlineServices;

    public:
        /** 
         * Resets state, important to clear all shared ptrs 
         */
        void Reset()
        {
            OnlineServices.Reset();
        }

        /**
         * Returns cached pointers are valid
         */
        bool IsValid() const
        {
            return OnlineServices.IsValid();
        }
    };

    //
    // ContextCache per OnlineService
    //
    TOnlineServiceContextContainer<FOnlineServiceContextCache> ContextCaches;

    FOnlineEventDelegateHandle LobbyMemberJoinedHandle;
    FOnlineEventDelegateHandle LobbyMemberLeftHandle;
    FOnlineEventDelegateHandle LobbyLeaderChangedHandle;
    FOnlineEventDelegateHandle LobbyAttributesChangedHandle;
    FOnlineEventDelegateHandle LobbyMemberAttributesChangedHandle;
    FOnlineEventDelegateHandle LobbyJoinRequestedHandle;

protected:
    /**
     * Returns online service of specific type, will return null if there is no type
     */
    IOnlineServicesPtr GetOnlineService(EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

    /**
     * Returns lobbies interface of specific type, will return null if there is no type
     */
    ILobbiesPtr GetOnlineLobbies(EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

protected:
    void BindOnlineDelegates();

#pragma endregion

    
    //////////////////////////////////////////////////////////////////////
    // Events
#pragma region Events
public:
    //
    // Native Delegate when a local user has accepted an invite
    // 
    FUserRequestLobbyDelegate OnUserLobbyRequestEvent;

    //
    // Event broadcast when a local user has accepted an invite
    //
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On User Requested Lobby"))
    FUserRequestLobbyDynamicDelegate K2_OnUserLobbyRequestEvent;


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
    // Native Delegate when a CreateLobby call has completed
    // 
    FCreateLobbyCompleteDelegate OnCreateLobbyCompleteEvent;

    //
    // Event broadcast when a CreateLobby call has completed
    //
    UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On Create Lobby Complete"))
    FCreateLobbyCompleteDynamicDelegate K2_OnCreateLobbyCompleteEvent;

    //
    // Native Delegate for modifying the connect URL prior to a client travel
    //
    FLobbyPreClientTravelDelegate OnPreClientTravelEvent;
    
protected:
    void NotifyUserLobbyRequest(const FPlatformUserId& PlatformUserId, USearchLobbyResult* RequestedLobby, const FOnlineResultInformation& RequestedLobbyResult);
    void NotifyJoinLobbyComplete(const FOnlineResultInformation& Result);
    void NotifyCreateLobbyComplete(const FOnlineResultInformation& Result);

#pragma endregion


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
     * Creates a new online game using the lobby request information, if successful this will start a hard map transfer 
     */
    UFUNCTION(BlueprintCallable, Category = "Lobby")
    virtual void HostLobby(APlayerController* HostingPlayer, UHostLobbyRequest* HostRequest);

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
    // Create Lobby
#pragma region Create Lobby
protected:
    void CreateOnlineLobbyInternal(ULocalPlayer* LocalPlayer, UHostLobbyRequest* HostRequest);

    /** 
     * Called when a new lobby is either created or fails to be created 
     */
    virtual void OnCreateLobbyComplete(FName LobbyName, bool bWasSuccessful);

    virtual void FinishLobbyCreation(bool bWasSuccessful);

    void SetCreateLobbyError(const FText& ErrorText);

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
