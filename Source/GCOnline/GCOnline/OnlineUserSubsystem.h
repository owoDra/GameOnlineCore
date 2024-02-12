// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "Type/OnlineUserTypes.h"

// OSSv2
#include "Online/Connectivity.h"
#include "Online/OnlineAsyncOpHandle.h"

#include "GameplayTagContainer.h"

#include "OnlineUserSubsystem.generated.h"

///////////////////////////////////////////////////

namespace UE::Online
{
    enum class ELoginStatus : uint8;
    enum class EPrivilegeResults : uint32;
    enum class EUserPrivileges : uint8;

    using IAuthPtr              = TSharedPtr<class IAuth>;
    using IConnectivityPtr      = TSharedPtr<class IConnectivity>;
    using IPrivilegesPtr        = TSharedPtr<class IPrivileges>;
    using IOnlineServicesPtr    = TSharedPtr<class IOnlineServices>;

    template <typename OpType>

    class TOnlineResult;

    struct FAuthLogin;
    struct FConnectionStatusChanged;
    struct FExternalUIShowLoginUI;
    struct FAuthLoginStatusChanged;
    struct FQueryUserPrivilege;
    struct FAccountInfo;
}
using namespace UE::Online;

class ULocalUserAccountInfo;

///////////////////////////////////////////////////


/**
 * Subsystem with features to extend the functionality of Online Servicies (OSSv2) and make it easier to use in projects
 * This subsystem primarily handles local user registration, management, and login/logout processes
 * 
 * Tips:
 *  Through this subsystem, local users are initialized to online services and have access to other services
 *  Use "OnlineUserInfoSubsystem" to obtain detailed information about a user account (nick name, icon, etc.)
 * 
 * Include OSSv2 Interface Feature:
 *  - Auth Interface
 *  - Connectivity Interface
 */
UCLASS(BlueprintType)
class GCONLINE_API UOnlineUserSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UOnlineUserSubsystem() {}

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

        //
        // Login status changed event handle
        //
        FOnlineEventDelegateHandle LoginStatusChangedHandle;

        //
        // Connection status changed event handle
        //
        FOnlineEventDelegateHandle ConnectionStatusChangedHandle;

        //
        // Last connection status that was passed into the HandleNetworkConnectionStatusChanged hander
        //
        EOnlineServicesConnectionStatus CurrentConnectionStatus{ EOnlineServicesConnectionStatus::NotConnected };


    public:
        /** 
         * Resets state, important to clear all shared ptrs 
         */
        void Reset()
        {
            OnlineServices.Reset();
            LoginStatusChangedHandle.Unbind();
            ConnectionStatusChangedHandle.Unbind();
        }

        /**
         * Returns cached pointers are valid
         */
        bool IsValid() const
        {
            return OnlineServices.IsValid();
        }
    };

public:
    //
    // ContextCache per OnlineService
    //
    TOnlineServiceContextContainer<FOnlineServiceContextCache> ContextCaches;

protected:
    /**
     * Bind online delegates
     */
    virtual void BindOnlineDelegates();

public:
    /** 
     * Returns auth interface of specific type, will return null if there is no type 
     */
    IAuthPtr GetOnlineAuth(EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

    /**
     * Returns connectivity interface of specific type, will return null if there is no type
     */
    IConnectivityPtr GetOnlineConnectivity(EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

#pragma endregion


    ///////////////////////////////////////////////////////////////////////
    // Local User
#pragma region Local User
protected:
    //
    // List of LocalPlayer user information currently registered and initialized in the subsystem
    // 
    // Tips:
    //  The index of this list is the same as the index of LocalPlayerArray in GameInstance.
    //  Also, the 0th index is always existed as Primary user.
    //
	UPROPERTY(Transient)
    TMap<int32, TObjectPtr<ULocalUserAccountInfo>> LocalUserInfos;

    //
    // Maximum number of local players
    //
    UPROPERTY(Transient)
    int32 MaxNumberOfLocalPlayers{ 0 };

public:
    /** 
     * Sets the maximum number of local players, will not destroy existing ones 
     */
    UFUNCTION(BlueprintCallable, Category = "Local User")
    virtual void SetMaxLocalPlayers(int32 InMaxLocalPlayers);

    /** 
     * Gets the maximum number of local players 
     */
    UFUNCTION(BlueprintPure, Category = "Local User")
    int32 GetMaxLocalPlayers() const;

    /** 
     * Gets the current number of local players, will always be at least 1 
     */
    UFUNCTION(BlueprintPure, Category = "Local User")
    int32 GetNumLocalPlayers() const;

public:
    /** 
     * Returns the user info for a given local player index in game instance, 0 is always valid in a running game 
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "Local User")
    const ULocalUserAccountInfo* GetUserInfoForLocalPlayerIndex(int32 LocalPlayerIndex) const;

    /** 
     * Returns the primary user info for a given platform user index. Can return null 
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "Local User")
    const ULocalUserAccountInfo* GetUserInfoForPlatformUser(FPlatformUserId PlatformUser) const;

    /** 
     * Returns the user info for a unique net id. Can return null 
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "Local User")
    const ULocalUserAccountInfo* GetUserInfoForUniqueNetId(const FUniqueNetIdRepl& NetId) const;

    /** 
     * Returns the user info for a given input device. Can return null 
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "Local User")
    const ULocalUserAccountInfo* GetUserInfoForInputDevice(FInputDeviceId InputDevice) const;

public:
    /** 
     * Returns true if this this could be a real platform user with a valid identity (even if not currently logged in) 
     */
    virtual bool IsRealPlatformUser(FPlatformUserId PlatformUser) const;

    /** 
     * Gets the user for an input device 
     */
    virtual FPlatformUserId GetPlatformUserIdForInputDevice(FInputDeviceId InputDevice) const;

    /** 
     * Gets a user's primary input device id 
     */
    virtual FInputDeviceId GetPrimaryInputDeviceForPlatformUser(FPlatformUserId PlatformUser) const;

    /** 
     * Returns the unique net id for a local platform user 
     */
    FUniqueNetIdRepl GetLocalUserNetId(FPlatformUserId PlatformUser, EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

    TSharedPtr<FAccountInfo> GetOnlineServiceAccountInfo(IAuthPtr AuthService, FPlatformUserId InUserId) const;

public:
    /**
     * Resets the all state when returning to the main menu after an error
     */
    UFUNCTION(BlueprintCallable, Category = "Local User")
    virtual void ResetUserState();

protected:
    /** 
     * Create a new user info object 
     */
    virtual ULocalUserAccountInfo* CreateLocalUserInfo(int32 LocalPlayerIndex);

    /** 
     * Deconst wrapper for const getters 
     */
    ULocalUserAccountInfo* ModifyInfo(const ULocalUserAccountInfo* Info) { return const_cast<ULocalUserAccountInfo*>(Info); }

    /** 
     * Refresh user info from OSS 
     */
    virtual void RefreshLocalUserInfo(ULocalUserAccountInfo* UserInfo);

    /** 
     * Assign a local player to a specific local user and call callbacks as needed 
     */
    virtual void SetLocalPlayerUserInfo(ULocalPlayer* LocalPlayer, const ULocalUserAccountInfo* UserInfo);

#pragma endregion


    ///////////////////////////////////////////////////////////////////////
    // Login
#pragma region Login
protected:
    /** 
     * Internal structure to represent an in-progress login request 
     */
    struct FUserLoginRequest : public TSharedFromThis<FUserLoginRequest>
    {
    public:
        FUserLoginRequest(ULocalUserAccountInfo* InUserInfo, ELocalUserPrivilege InPrivilege, EOnlineServiceContext InContext, FLocalUserLoginCompleteDelegate&& InDelegate)
            : UserInfo(TWeakObjectPtr<ULocalUserAccountInfo>(InUserInfo))
            , DesiredPrivilege(InPrivilege)
            , DesiredContext(InContext)
            , Delegate(MoveTemp(InDelegate))
        {}

    public:
        //
        // Which local user is trying to log on
        //
        TWeakObjectPtr<ULocalUserAccountInfo> UserInfo;

        //
        // Overall state of login request, could come from many sources
        //
        EOnlineServiceTaskState OverallLoginState{ EOnlineServiceTaskState::NotStarted };

        //
        // State of attempt to use platform auth.
        //
        EOnlineServiceTaskState TransferPlatformAuthState{ EOnlineServiceTaskState::NotStarted };

        //
        // State of attempt to use AutoLogin
        //
        EOnlineServiceTaskState AutoLoginState{ EOnlineServiceTaskState::NotStarted };

        //
        // State of attempt to use external login UI
        //
        EOnlineServiceTaskState LoginUIState{ EOnlineServiceTaskState::NotStarted };

        //
        // Final privilege to that is requested
        //
        ELocalUserPrivilege DesiredPrivilege{ ELocalUserPrivilege::Invalid };

        //
        // State of attempt to request the relevant privilege
        //
        EOnlineServiceTaskState PrivilegeCheckState{ EOnlineServiceTaskState::NotStarted };

        //
        // The final context to log into 
        //
        EOnlineServiceContext DesiredContext{ EOnlineServiceContext::Invalid };

        //
        // What online system we are currently logging into
        //
        EOnlineServiceContext CurrentContext{ EOnlineServiceContext::Invalid };

        //
        // User callback for completion
        //
        FLocalUserLoginCompleteDelegate Delegate;

        //
        // Most recent/relevant error to display to user
        //
        TOptional<FOnlineErrorType> Error;
    };

    //
    // List of current in progress login requests
    //
    TArray<TSharedRef<FUserLoginRequest>> ActiveLoginRequests;

public:
    //
    // Delegate called when any requested login request completes 
    //
    UPROPERTY(BlueprintAssignable, Category = "Login")
    FLocalUserLoginCompleteDynamicMulticastDelegate OnUserLoginComplete;

public:
    /**
     * Returns the current login status for a player on the specified online system, only works for real platform users
     */
    ELoginStatus GetLocalUserLoginStatus(FPlatformUserId PlatformUser, EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

    /**
     * Returns the state of login the specified local player
     */
    UFUNCTION(BlueprintPure, Category = "Local User")
    ELocalUserLoginState GetLocalPlayerLoginState(int32 LocalPlayerIndex) const;

public:
    /**
     * Tries to process login with local player as local play user
     * Update an existing LocalPlayer or create a new LocalPlayer as needed
     * 
     * Tips:
     *  When the process has succeeded or failed, it will broadcast the OnUserLoginComplete delegate.
     *
     * @param LocalPlayerIndex	Desired index of LocalPlayer in Game Instance, 0 will be primary player and 1+ for local multiplayer
     * @param PrimaryInputDevice The physical controller that should be mapped to this user, will use the default device if invalid
     * @param bCanUseGuestLogin	If true, this player can be a guest without a real Unique Net Id
     *
     * @returns true if the process was started, false if it failed before properly starting
     */
    UFUNCTION(BlueprintCallable, Category = "Login")
    virtual bool TryLoginForLocalPlay(int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin);

    /**
     * Tries to log in to online services as an online play user for a local player who is already logged in as a local play user.
     * Update an existing LocalPlayer or create a new LocalPlayer as needed
     * 
     * Note:
     *  A local player must be created to log in to online play.
	 *	The primary local player will be created automatically, but from the secondary player onward, 
	 *	please log in to Local Play first and create a local player
     * 
     * Tips:
     *  When the process has succeeded or failed, it will broadcast the OnUserLoginComplete delegate.
     *
     * @param LocalPlayerIndex	Index of existing LocalPlayer in Game Instance
     *
     * @returns true if the process was started, false if it failed before properly starting
     */
    UFUNCTION(BlueprintCallable, Category = "Login")
    virtual bool TryLoginForOnlinePlay(int32 LocalPlayerIndex);

    /**
     * Tries to process logins to local or online using specific login parameters
     * 
     * Tips:
     *  When the process has succeeded or failed, it will broadcast the OnUserLoginComplete delegate.
     *
     * @returns true if the process was started, false if it failed before properly starting
     */
    UFUNCTION(BlueprintCallable, Category = "Login")
    virtual bool TryGenericLogin(FLocalUserLoginParams Params);

    /** 
     * Cancels the running login process and disable the callback
     */
    UFUNCTION(BlueprintCallable, Category = "Login")
    virtual bool CancelLogin(int32 LocalPlayerIndex);

    /** 
     * Tries to process the logout of a local player who is already logged in to some online service or local play.
     */
    UFUNCTION(BlueprintCallable, Category = "Login")
    virtual bool TryLogout(int32 LocalPlayerIndex, bool bDestroyPlayer = false);

protected:
    /**
     * Starts the process of login for an existing local user, will return false if callback was not scheduled
     * This activates the low level state machine and does not modify the login state on user info
     */
    virtual bool LoginLocalUser(
        const ULocalUserAccountInfo* UserInfo
        , EOnlineServiceContext Context
        , ELocalUserPrivilege RequestedPrivilege
        , FLocalUserLoginCompleteDelegate OnComplete);

    /** 
     * Forcibly logs out and deinitializes a single user 
     */
    virtual void LogoutLocalUser(FPlatformUserId PlatformUser);

    /** 
     * Performs the next step of a login request, which could include completing it. Returns true if it's done 
     */
    virtual void ProcessLoginRequest(TSharedRef<FUserLoginRequest> Request);

    /** 
     * Call login on OSS, with platform auth from the platform OSS. Return true if AutoLogin started 
     */
    virtual bool TransferPlatformAuth(FOnlineServiceContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);

    /** 
     * Call AutoLogin on OSS. Return true if AutoLogin started. 
     */
    virtual bool AutoLogin(FOnlineServiceContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);

    /** 
     * Call ShowLoginUI on OSS. Return true if ShowLoginUI started. 
     */
    virtual bool ShowLoginUI(FOnlineServiceContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);

    /** 
     * Call QueryUserPrivilege on OSS. Return true if QueryUserPrivilege started. 
     */
    virtual bool QueryLoginRequestedPrivilege(FOnlineServiceContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser);

protected:
    virtual void HandleAuthLoginStatusChanged(const FAuthLoginStatusChanged& EventParameters, EOnlineServiceContext Context);
    virtual void HandleUserLoginCompleted(const TOnlineResult<FAuthLogin>& Result, FPlatformUserId PlatformUser, EOnlineServiceContext Context);
    virtual void HandleOnLoginUIClosed(const TOnlineResult<FExternalUIShowLoginUI>& Result, FPlatformUserId PlatformUser, EOnlineServiceContext Context);
    virtual void HandleCheckPrivilegesComplete(ULocalUserAccountInfo* UserInfo, EOnlineServiceContext Context, ELocalUserPrivilege DesiredPrivilege, ELocalUserPrivilegeResult PrivilegeResult, const TOptional<FOnlineErrorType>& Error);

    virtual void HandleLoginForUserInitialize(const ULocalUserAccountInfo* UserInfo, ELoginStatusType NewStatus, FUniqueNetIdRepl NetId, const TOptional<FOnlineErrorType>& InError, EOnlineServiceContext Context, FLocalUserLoginParams Params);
    virtual void HandleUserLoginFailed(FLocalUserLoginParams Params, FText Error);
    virtual void HandleUserLoginSucceeded(FLocalUserLoginParams Params);

#pragma endregion


    ///////////////////////////////////////////////////////////////////////
    // Connection
#pragma region Connection
protected:
    virtual void HandleNetworkConnectionStatusChanged(const FConnectionStatusChanged& EventParameters, EOnlineServiceContext Context);

    void CacheConnectionStatus(EOnlineServiceContext Context);

public:
    /** 
     * Returns the current online connection status 
     */
    EOnlineServicesConnectionStatus GetConnectionStatus(EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

    /** 
     * Returns true if we are currently connected to backend servers 
     */
    bool HasOnlineConnection(EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

#pragma endregion


    ///////////////////////////////////////////////////////////////////////
    // Input
#pragma region Input
protected:
    /**
     * Callback for when an input device (i.e. a gamepad) has been connected or disconnected.
     */
    virtual void HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);

#pragma endregion
    

    ///////////////////////////////////////////////////////////////////////
    // Utilities
protected:
    void SendSystemMessage(FGameplayTag MessageType, FText TitleText, FText BodyText);

};
