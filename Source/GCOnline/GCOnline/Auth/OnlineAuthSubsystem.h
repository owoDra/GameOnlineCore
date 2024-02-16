// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "Type/OnlineAuthLoginTypes.h"
#include "Type/OnlineServiceTaskTypes.h"

// OSSv2
#include "Online/OnlineAsyncOpHandle.h"

#include "OnlineAuthSubsystem.generated.h"

///////////////////////////////////////////////////

namespace UE::Online
{
    enum class ELoginStatus : uint8;
    enum class EPrivilegeResults : uint32;
    enum class EUserPrivileges : uint8;

    using IOnlineServicesPtr = TSharedPtr<class IOnlineServices>;
    using IAuthPtr  = TSharedPtr<class IAuth>;

    template <typename OpType>
    class TOnlineResult;

    struct FAuthLogin;
    struct FExternalUIShowLoginUI;
    struct FAuthLoginStatusChanged;
    struct FAccountInfo;
}
using namespace UE::Online;

class UOnlineServiceSubsystem;
class UOnlineLocalUserSubsystem;
class UOnlineLocalUserManagerSubsystem;
class ULocalPlayer;
class APlayerController;

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
 */
UCLASS(BlueprintType)
class GCONLINE_API UOnlineAuthSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UOnlineAuthSubsystem() {}

    ///////////////////////////////////////////////////////////////////////
    // Initialization
protected:
    //
    // Login status changed event handle
    //
    TMap<EOnlineServiceContext, FOnlineEventDelegateHandle> LoginHandles;

    UPROPERTY(Transient)
    TObjectPtr<UOnlineServiceSubsystem> OnlineServiceSubsystem{ nullptr };

    UPROPERTY(Transient)
    TObjectPtr<UOnlineLocalUserManagerSubsystem> OnlineLocalUserManagerSubsystem{ nullptr };

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
    /**
     * Bind/Unbind online delegates
     */
    virtual void BindLoginDelegates();
    virtual void UnbindLoginDelegates();

    /** 
     * Returns auth interface of specific type, will return null if there is no type 
     */
    IAuthPtr GetAuthInterface(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

    TSharedPtr<FAccountInfo> GetOnlineServiceAccountInfo(IAuthPtr AuthService, FPlatformUserId InUserId) const;

    FAccountId GetLocalUserNetId(FPlatformUserId PlatformUser, EOnlineServiceContext Context = EOnlineServiceContext::Default) const;


    ///////////////////////////////////////////////////////////////////////
    // Login / Logout
protected:
    /** 
     * Internal structure to represent an in-progress login request 
     */
    struct FUserLoginRequest : public TSharedFromThis<FUserLoginRequest>
    {
    public:
        FUserLoginRequest(
            UOnlineLocalUserSubsystem* InLocalUser
            , EOnlinePrivilege InPrivilege
            , EOnlineServiceContext InContext
            , FLocalUserLoginCompleteDelegate&& InDelegate)
                : LocalUser(TWeakObjectPtr<UOnlineLocalUserSubsystem>(InLocalUser))
                , DesiredPrivilege(InPrivilege)
                , DesiredContext(InContext)
                , Delegate(MoveTemp(InDelegate))
        {}

    public:
        //
        // Which local user is trying to log on
        //
        TWeakObjectPtr<UOnlineLocalUserSubsystem> LocalUser;

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
        EOnlinePrivilege DesiredPrivilege{ EOnlinePrivilege::Invalid };

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
        FOnlineServiceResult Result;
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
     * Tries to process logins to local or online using specific login parameters
     * 
     * Tips:
     *  When the process has succeeded or failed, it will broadcast the OnUserLoginComplete delegate.
     */
    UFUNCTION(BlueprintCallable, Category = "Login")
    virtual bool TryLogin(const APlayerController* PlayerController, FLocalUserLoginParams Params);

    /** 
     * Cancels the running login process and disable the callback
     */
    UFUNCTION(BlueprintCallable, Category = "Login")
    virtual bool CancelLogin(const APlayerController* PlayerController);

    /** 
     * Tries to process the logout of a local player who is already logged in to some online service or local play.
     */
    UFUNCTION(BlueprintCallable, Category = "Login")
    virtual bool TryLogout(const APlayerController* PlayerController, bool bDestroyPlayer = false);

protected:
    /**
     * Starts the process of login for an existing local user, will return false if callback was not scheduled
     * This activates the low level state machine and does not modify the login state on user info
     */
    virtual bool LoginLocalUser(
        UOnlineLocalUserSubsystem* LocalUser
        , EOnlineServiceContext Context
        , EOnlinePrivilege RequestedPrivilege
        , FLocalUserLoginCompleteDelegate OnComplete);

    /** 
     * Performs the next step of a login request, which could include completing it. Returns true if it's done 
     */
    virtual void ProcessLoginRequest(TSharedRef<FUserLoginRequest> Request);

    virtual void HandleAuthLoginStatusChanged(const FAuthLoginStatusChanged& EventParameters, EOnlineServiceContext Context);

    virtual void HandleLoginForUserInitialize(UOnlineLocalUserSubsystem* LocalUser, ELoginStatusType NewStatus, FUniqueNetIdRepl NetId, FOnlineServiceResult Result, EOnlineServiceContext Context, FLocalUserLoginParams Params);
    virtual void HandleUserLoginFailed(UOnlineLocalUserSubsystem* LocalUser, FLocalUserLoginParams Params, FOnlineServiceResult Result);
    virtual void HandleUserLoginSucceeded(UOnlineLocalUserSubsystem* LocalUser, FLocalUserLoginParams Params, FOnlineServiceResult Result);


    ///////////////////////////////////////////////////////////////////////
    // Transfer Platform Auth
protected:
    /** 
     * Call login on OSS, with platform auth from the platform OSS. Return true if AutoLogin started 
     */
    virtual bool TransferPlatformAuth(
        IOnlineServicesPtr OnlineService
        , TSharedRef<FUserLoginRequest> Request
        , FPlatformUserId PlatformUser);

    virtual void HandleTransferPlatformAuth(
        const TOnlineResult<FAuthQueryExternalAuthToken>& Result
        , TWeakPtr<FUserLoginRequest> Request
        , FPlatformUserId PlatformUser);

    virtual void HandlePlatformLoginComplete(
        const TOnlineResult<FAuthLogin>& Result
        , TWeakPtr<FUserLoginRequest> Request
        , FPlatformUserId PlatformUser);


    ///////////////////////////////////////////////////////////////////////
    // Auto Login
protected:
    /** 
     * Call AutoLogin on OSS. Return true if AutoLogin started. 
     */
    virtual bool AutoLogin(
        IOnlineServicesPtr OnlineService
        , TSharedRef<FUserLoginRequest> Request
        , FPlatformUserId PlatformUser);

    virtual void HandleAutoLoginComplete(
        const TOnlineResult<FAuthLogin>& Result
        , TWeakPtr<FUserLoginRequest> Request
        , FPlatformUserId PlatformUser);


    ///////////////////////////////////////////////////////////////////////
    // Show Login UI
protected:
    /** 
     * Call ShowLoginUI on OSS. Return true if ShowLoginUI started. 
     */
    virtual bool ShowLoginUI(
        IOnlineServicesPtr OnlineService
        , TSharedRef<FUserLoginRequest> Request
        , FPlatformUserId PlatformUser);

    virtual void HandleLoginUIClosed(
        const TOnlineResult<FExternalUIShowLoginUI>& Result
        , TWeakPtr<FUserLoginRequest> Request
        , FPlatformUserId PlatformUser);


    ///////////////////////////////////////////////////////////////////////
    // Privilege Check
protected:
    /** 
     * Call QueryUserPrivilege on OSS. Return true if QueryUserPrivilege started. 
     */
    virtual bool QueryLoginRequestedPrivilege(
        IOnlineServicesPtr OnlineService
        , TSharedRef<FUserLoginRequest> Request
        , FPlatformUserId PlatformUser);

    virtual void HandleCheckPrivilegesComplete(
        const ULocalPlayer* LocalPlayer
        , EOnlineServiceContext Context
        , EOnlinePrivilege DesiredPrivilege
        , EOnlinePrivilegeResult PrivilegeResult
        , FOnlineServiceResult ServiceResult);

};
