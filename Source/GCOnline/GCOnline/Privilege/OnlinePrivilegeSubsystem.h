// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

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

class UOnlineServiceSubsystem;
struct FUniqueNetIdRepl;

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
	UPROPERTY(Transient)
    TObjectPtr<UOnlineServiceSubsystem> OnlineServiceSubsystem{ nullptr };

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
    /**
     * Returns privileges interface of specific type, will return null if there is no type
     */
    IPrivilegesPtr GetPrivilegesInterface(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

    
    ///////////////////////////////////////////////////////////////////////
    // Privilege
public:
    virtual EOnlinePrivilege ConvertOnlineServicesPrivilege(EUserPrivileges Privilege) const;
    virtual EUserPrivileges ConvertOnlineServicesPrivilege(EOnlinePrivilege Privilege) const;
    virtual EOnlinePrivilegeResult ConvertOnlineServicesPrivilegeResult(EUserPrivileges Privilege, EPrivilegeResults Results) const;

    /** 
     * Returns human readable string for privilege checks 
     */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Privilege")
    virtual FText GetPrivilegeDescription(EOnlineServiceContext Context, EOnlinePrivilege Privilege) const;

    /**
     * Returns human readable string for privilege results
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Privilege")
    virtual FText GetPrivilegeResultDescription(EOnlineServiceContext Context, EOnlinePrivilegeResult Result) const;

public:
    /**
     * Query the local user's account for privileges on available online services
     */
    bool QueryUserPrivilege(
        const ULocalPlayer* LocalPlayer
        , EOnlineServiceContext Context = EOnlineServiceContext::Default
        , EOnlinePrivilege DesiredPrivilege = EOnlinePrivilege::CanPlayOnline
        , FOnlinePrivilegeQueryDelegate Delegate = FOnlinePrivilegeQueryDelegate());

protected:
    /**
     * Called when a query for privileges is completed.
     */
    virtual void HandleQueryPrivilegeComplete(
        const TOnlineResult<FQueryUserPrivilege>& Result
        , const ULocalPlayer* LocalPlayer
        , EOnlineServiceContext Context
        , EUserPrivileges DesiredPrivilege
        , FOnlinePrivilegeQueryDelegate Delegate = FOnlinePrivilegeQueryDelegate());

};
