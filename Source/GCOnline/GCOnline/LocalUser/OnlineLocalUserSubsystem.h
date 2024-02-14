// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"

#include "Type/OnlineServiceContextTypes.h"
#include "Type/OnlinePrivilegeTypes.h"
#include "Type/OnlineLocalUserTypes.h"

#include "OnlineLocalUserSubsystem.generated.h"

///////////////////////////////////////////////////

namespace UE::Online
{
	struct FAccountInfo;
}
using namespace UE::Online;

///////////////////////////////////////////////////

/**
 * Subsystem that manages local user data linked to local players
 */
UCLASS(BlueprintType)
class GCONLINE_API UOnlineLocalUserSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:
    UOnlineLocalUserSubsystem() {}

    ///////////////////////////////////////////////////////////////////////
    // Initialization
protected:
	UPROPERTY(Transient)
	bool bLocalUserInitialized{ false };

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void InitializeLocalUser(FInputDeviceId InPrimaryInputDevice, bool bCanUseGuestLogin);

public:
	UFUNCTION(BlueprintCallable, Category = "Local User")
	void ResetLocalUser();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Local User")
	bool HasLocalUserInitialized() const { return bLocalUserInitialized; }


	/////////////////////////////////////////////////////
	// Local User
public:
	/**
	 * Cached data of privilege result for each online service
	 */
	struct FPrivilegeCache
	{
	public:
		FPrivilegeCache() = default;
		~FPrivilegeCache() = default;

	public:
		//
		// Cached values of various user privileges
		//
		TMap<EOnlinePrivilege, EOnlinePrivilegeResult> CachedPrivileges;
	};

	//
	// Cached datas of account-specific information for each online service
	//
	TMap<EOnlineServiceContext, FPrivilegeCache> CachedPrivilegeResults;

	//
	// Cached datas of account info for each online service
	//
	TMap<EOnlineServiceContext, TSharedPtr<FAccountInfo>> CachedAccountInfos;

	//
	// Primary controller input device for this user, they could also have additional secondary devices
	//
	UPROPERTY(BlueprintReadOnly, Category = "Local User")
	FInputDeviceId PrimaryInputDeviceId;

	//
	// Specifies the logical user on the local platform, guest users will point to the primary user
	//
	UPROPERTY(BlueprintReadOnly, Category = "Local User")
	FPlatformUserId PlatformUserId;

	//
	// Whether this user is allowed to be a guest
	//
	UPROPERTY(BlueprintReadOnly, Category = "Local User")
	bool bCanBeGuest{ false };

	//
	// Whether this is a guest user attached to primary user 0
	//
	UPROPERTY(BlueprintReadOnly, Category = "Local User")
	bool bIsGuest{ false };

	//
	// Overall state of the user's logged in process
	//
	UPROPERTY(BlueprintReadOnly, Category = "Local User")
	ELocalUserLoginState LoginState{ ELocalUserLoginState::Invalid };

public:
	//
	// Delegate called when privilege availability changes for a user
	//
	UPROPERTY(BlueprintAssignable, Category = "Local User")
	FLocalUserAvailabilityChangedDelegate OnLocalUserAvailabilityChanged;

public:
	/**
	 * Updates cached privilege results, will propagate to game if needed
	 */
	void UpdateCachedPrivilegeResult(EOnlinePrivilege Privilege, EOnlinePrivilegeResult Result, EOnlineServiceContext Context);

	/**
	 * Updates cached account info
	 */
	void UpdateCachedAccountInfo(const TSharedPtr<FAccountInfo>& InAccountInfo, EOnlineServiceContext Context);

	/**
	 * Possibly send privilege availability notification, compares current value to cached old value
	 */
	void HandleChangedAvailability(EOnlinePrivilege Privilege, ELocalUserOnlineAvailability OldAvailability);

public:
	/**
	 * Returns whether this user has successfully logged in
	 */
	UFUNCTION(BlueprintCallable, Category = "Local User")
	bool IsLoggedIn() const;

	/**
	 * Returns whether this user is in the middle of logging in
	 */
	UFUNCTION(BlueprintCallable, Category = "Local User")
	bool IsDoingLogin() const;

	/**
	 * Gets cached privilege data for a type of online system, can return null for service
	 */
	TSharedPtr<FAccountInfo> GetCachedAccountInfo(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

	/**
	 * Returns the most recently queries result for a specific privilege, will return unknown if never queried
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Local User")
	EOnlinePrivilegeResult GetCachedPrivilegeResult(EOnlinePrivilege Privilege, EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

	/**
	 * Ask about the general availability of a feature, this combines cached results with state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Local User")
	ELocalUserOnlineAvailability GetPrivilegeAvailability(EOnlinePrivilege Privilege, EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

	/**
	 * Returns the net id for the given context
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Local User")
	FUniqueNetIdRepl GetNetId(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

	/**
	 * Returns the user's human readable nickname
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Local User")
	FString GetNickname(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;


	////////////////////////////////////////////////////////////////
	// Utilities
public:
	/**
	 * Return the game instance
	 */
	UGameInstance* GetGameInstance() const;

	/**
	 * Returns an internal debug string for this player
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	FString GetDebugString() const;

};
