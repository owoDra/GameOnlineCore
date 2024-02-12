// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "Type/OnlineServiceTypes.h"
#include "Type/OnlinePrivilegeTypes.h"

// OSSv2
#include "Online/OnlineAsyncOpHandle.h"

#include "OnlinePrivilegeSubsystem.generated.h"

///////////////////////////////////////////////////

namespace UE::Online
{
    enum class EPrivilegeResults : uint32;
    enum class EUserPrivileges : uint8;

    using IOnlineServicesPtr = TSharedPtr<class IOnlineServices>;
    using IPrivilegesPtr        = TSharedPtr<class IPrivileges>;

    template <typename OpType>

    class TOnlineResult;

    struct FQueryUserPrivilege;
}
using namespace UE::Online;

class ULocalUserAccountInfo;

///////////////////////////////////////////////////

/**
 * Subsystem with features to extend the functionality of Online Servicies (OSSv2) and make it easier to use in projects
 * This subsystem handles the functionality for querying the privileges to use online services for local users who are already logged in to the online services
 * 
 * Tips:
 *  Logged-in local users use information managed by the "OnlineUserSubsystem"
 * 
 * Include OSSv2 Interface Feature:
 *  - Privileges Interface
 */
UCLASS(BlueprintType)
class GCONLINE_API UOnlinePrivilegeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UOnlinePrivilegeSubsystem() {}

    ///////////////////////////////////////////////////////////////////////
    // Initialization
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;


    ///////////////////////////////////////////////////////////////////////
    // Context Cache
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

protected:
    /**
     * Returns privileges interface of specific type, will return null if there is no type
     */
    IPrivilegesPtr GetOnlinePrivileges(EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

    
    ///////////////////////////////////////////////////////////////////////
    // Privilege
public:
    virtual ELocalUserPrivilege ConvertOnlineServicesPrivilege(EUserPrivileges Privilege) const;
    virtual EUserPrivileges ConvertOnlineServicesPrivilege(ELocalUserPrivilege Privilege) const;
    virtual ELocalUserPrivilegeResult ConvertOnlineServicesPrivilegeResult(EUserPrivileges Privilege, EPrivilegeResults Results) const;

    /** 
     * Returns human readable string for privilege checks 
     */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Privilege")
    virtual FText GetPrivilegeDescription(EOnlineServiceContext Context, ELocalUserPrivilege Privilege) const;

    /**
     * Returns human readable string for privilege results
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Privilege")
    virtual FText GetPrivilegeResultDescription(EOnlineServiceContext Context, ELocalUserPrivilegeResult Result) const;

public:
    /**
     * Query the local user's account for privileges on available online services
     */
	UFUNCTION(BlueprintCallable, Category = "Privilege", meta = (DisplayName = "QueryUserPrivilege"))
    bool BP_QueryUserPrivilege(
        ULocalUserAccountInfo* TargetUser
        , EOnlineServiceContext Context
        , ELocalUserPrivilege DesiredPrivilege);

    bool QueryUserPrivilege(
        TWeakObjectPtr<ULocalUserAccountInfo> TargetUser
        , EOnlineServiceContext Context
        , ELocalUserPrivilege DesiredPrivilege
        , FLocalUserPrivilegeQueryDelegate Delegate = FLocalUserPrivilegeQueryDelegate());

protected:
    /**
     * Called when a query for privileges is completed.
     */
    virtual void HandleQueryPrivilegeComplete(
        const TOnlineResult<FQueryUserPrivilege>& Result
        , TWeakObjectPtr<ULocalUserAccountInfo> UserInfo
        , EOnlineServiceContext Context
        , EUserPrivileges DesiredPrivilege
        , FLocalUserPrivilegeQueryDelegate Delegate = FLocalUserPrivilegeQueryDelegate());

};
