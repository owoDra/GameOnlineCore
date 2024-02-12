// Copyright (C) 2024 owoDra

#pragma once

#include "Type/OnlineServiceTypes.h"
#include "Type/OnlinePrivilegeTypes.h"

#include "OnlineUserTypes.generated.h"

using ELoginStatusType = UE::Online::ELoginStatus;

class UOnlineUserSubsystem;


////////////////////////////////////////////////////////////////////////
// Enums

/** 
 * Enum describing the state of initialization for a specific local user 
 */
UENUM(BlueprintType)
enum class ELocalUserLoginState : uint8
{
	// User has not started login process
	Unknown,

	// Player is in the process of acquiring a user id with local login
	DoingInitialLogin,

	// Player is performing the network login, they have already logged in locally
	DoingNetworkLogin,

	// Player failed to log in at all
	FailedToLogin,

	// Player is logged in and has access to online functionality
	LoggedInOnline,

	// Player is logged in locally (either guest or real user), but cannot perform online actions
	LoggedInLocalOnly,

	// Invalid state or user 
	Invalid,
};


////////////////////////////////////////////////////////////////////////
// Delegates

/** 
 * Delegates when initialization processes succeed or fail 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FLocalUserLoginCompleteDynamicMulticastDelegate
												, const ULocalUserAccountInfo*	, UserInfo
												, bool							, bSuccess
												, FText							, Error
												, ELocalUserPrivilege			, RequestedPrivilege
												, EOnlineServiceContext			, OnlineContext);

DECLARE_DYNAMIC_DELEGATE_FiveParams(FLocalUserLoginCompleteDynamicDelegate
												, const ULocalUserAccountInfo*	, UserInfo
												, bool							, bSuccess
												, FText							, Error
												, ELocalUserPrivilege			, RequestedPrivilege
												, EOnlineServiceContext			, OnlineContext);

DECLARE_DELEGATE_FiveParams(FLocalUserLoginCompleteDelegate
												, const ULocalUserAccountInfo*          /*UserInfo*/
												, ELoginStatusType                      /*NewStatus*/
												, FUniqueNetIdRepl                      /*NetId*/
												, const TOptional<FOnlineErrorType>&    /*Error*/
												, EOnlineServiceContext                 /*Type*/);


////////////////////////////////////////////////////////////////////////
// Structs

/** 
 * Parameter data used in the login process for LocalUser local or online play
 * 
 * Tips:
 *	This would normally be filled in by wrapper functions like async nodes 
 */
USTRUCT(BlueprintType)
struct GCONLINE_API FLocalUserLoginParams
{
	GENERATED_BODY()
public:
	FLocalUserLoginParams() = default;

public:
	//
	// What local player index to use, can specify one above current if can create player is enabled
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 LocalPlayerIndex{ 0 };

	//
	// Primary controller input device for this user, they could also have additional secondary devices
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FInputDeviceId PrimaryInputDevice;

	//
	// Specifies the logical user on the local platform
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FPlatformUserId PlatformUser;

	//
	// Generally either CanPlay or CanPlayOnline, specifies what level of privilege is required
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ELocalUserPrivilege RequestedPrivilege{ ELocalUserPrivilege::CanPlay };

	//
	// What specific online context to log in to, game means to login to all relevant ones 
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EOnlineServiceContext OnlineContext{ EOnlineServiceContext::Game };

	//
	// True if this is allowed to create a new local player for initial login
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCanCreateNewLocalPlayer{ false };

	// 
	// True if this player can be a guest user without an actual online presence
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCanUseGuestLogin{ false };

	//
	// True if we should not show login errors, the game will be responsible for displaying them
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bSuppressLoginErrors = false;

	//
	// If bound, call this dynamic delegate at completion of login
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLocalUserLoginCompleteDynamicDelegate OnLocalUserLoginComplete;

};


////////////////////////////////////////////////////////////////////////
// Objects

/**
 * User information assigned to local players already initialized to online services
 */
UCLASS(BlueprintType)
class GCONLINE_API ULocalUserAccountInfo : public UObject
{
	GENERATED_BODY()
public:
	ULocalUserAccountInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/////////////////////////////////////////////////////
	// Account Cache
public:
	/**
	 * Cached data of account-specific information for each online service
	 */
	struct FAccountInfoCache
	{
	public:
		FAccountInfoCache() = default;
		~FAccountInfoCache() = default;

	public:
		//
		// Cached net id per system
		//
		FUniqueNetIdRepl CachedNetId;

		//
		// Cached values of various user privileges
		//
		TMap<ELocalUserPrivilege, ELocalUserPrivilegeResult> CachedPrivileges;
	};

	//
	// Cached datas of account-specific information for each online service
	//
	TMap<EOnlineServiceContext, FAccountInfoCache> CachedAccountInfos;

public:
	/**
	 * Gets internal data for a type of online system, can return null for service
	 */
	const FAccountInfoCache* GetCachedAccountInfo(EOnlineServiceContext Context) const;
	FAccountInfoCache* GetCachedAccountInfo(EOnlineServiceContext Context);

	/**
	 * Updates cached privilege results, will propagate to game if needed
	 */
	void UpdateCachedPrivilegeResult(ELocalUserPrivilege Privilege, ELocalUserPrivilegeResult Result, EOnlineServiceContext Context);

	/**
	 * Updates cached privilege results, will propagate to game if needed
	 */
	void UpdateCachedNetId(const FUniqueNetIdRepl& NewId, EOnlineServiceContext Context);

	/**
	 * Possibly send privilege availability notification, compares current value to cached old value
	 */
	void HandleChangedAvailability(ELocalUserPrivilege Privilege, ELocalUserAvailability OldAvailability);


	/////////////////////////////////////////////////////
	// Local Player Info
public:
	//
	// Primary controller input device for this user, they could also have additional secondary devices
	//
	UPROPERTY(BlueprintReadOnly, Category = "UserInfo")
	FInputDeviceId PrimaryInputDevice;

	//
	// Specifies the logical user on the local platform, guest users will point to the primary user
	//
	UPROPERTY(BlueprintReadOnly, Category = "UserInfo")
	FPlatformUserId PlatformUser;

	//
	// Index of LocalPlayer where this data is registered
	//
	// Tips:	
	//	this will match the index in the GameInstance localplayers array once it is fully created
	//
	UPROPERTY(BlueprintReadOnly, Category = "UserInfo")
	int32 LocalPlayerIndex{ INDEX_NONE };

	//
	// Whether this user is allowed to be a guest
	//
	UPROPERTY(BlueprintReadOnly, Category = "UserInfo")
	bool bCanBeGuest{ false };

	//
	// Whether this is a guest user attached to primary user 0
	//
	UPROPERTY(BlueprintReadOnly, Category = "UserInfo")
	bool bIsGuest{ false };

	//
	// Overall state of the user's initialization process
	//
	UPROPERTY(BlueprintReadOnly, Category = "UserInfo")
	ELocalUserLoginState LoginState{ ELocalUserLoginState::Invalid };

public:
	//
	// Delegate called when privilege availability changes for a user
	//
	UPROPERTY(BlueprintAssignable, Category = "Privilege")
	FLocalUserAvailabilityChangedDelegate OnUserPrivilegeChanged;

public:
	/**
	 * Returns whether this user has successfully logged in
	 */
	UFUNCTION(BlueprintCallable, Category = "UserInfo")
	bool IsLoggedIn() const;

	/**
	 * Returns whether this user is in the middle of logging in
	 */
	UFUNCTION(BlueprintCallable, Category = "UserInfo")
	bool IsDoingLogin() const;

	/**
	 * Returns the most recently queries result for a specific privilege, will return unknown if never queried
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UserInfo")
	ELocalUserPrivilegeResult GetCachedPrivilegeResult(ELocalUserPrivilege Privilege, EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

	/**
	 * Ask about the general availability of a feature, this combines cached results with state
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UserInfo")
	ELocalUserAvailability GetPrivilegeAvailability(ELocalUserPrivilege Privilege) const;

	/**
	 * Returns the net id for the given context
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UserInfo")
	FUniqueNetIdRepl GetNetId(EOnlineServiceContext Context = EOnlineServiceContext::Game) const;

	/**
	 * Returns the user's human readable nickname
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UserInfo")
	FString GetNickname() const;

	/**
	 * Accessor for platform user id
	 */
	FPlatformUserId GetPlatformUserId() const;


	////////////////////////////////////////////////////////////////
	// Utilities
public:
	/**
	 * Return the subsystem this is owned by
	 */
	UOnlineUserSubsystem* GetSubsystem() const;

	/**
	 * Returns an internal debug string for this player
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	FString GetDebugString() const;

};
