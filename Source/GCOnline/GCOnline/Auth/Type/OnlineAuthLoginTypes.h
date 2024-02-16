// Copyright (C) 2024 owoDra

#pragma once

#include "Type/OnlinePrivilegeTypes.h"
#include "Type/OnlineServiceContextTypes.h"
#include "Type/OnlineServiceResultTypes.h"

#include "Online/OnlineServicesEngineUtils.h"

#include "OnlineAuthLoginTypes.generated.h"

using ELoginStatusType = UE::Online::ELoginStatus;

class ULocalPlayer;
class APlayerController;
class UOnlineLocalUserSubsystem;


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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLocalUserLoginCompleteDynamicMulticastDelegate
	, const APlayerController*	, PlayerController
	, FOnlineServiceResult		, Result
	, EOnlineServiceContext		, OnlineContext);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FLocalUserLoginCompleteDynamicDelegate
	, const APlayerController*	, PlayerController
	, FOnlineServiceResult		, Result
	, EOnlineServiceContext		, OnlineContext);

DECLARE_DELEGATE_FiveParams(FLocalUserLoginCompleteDelegate
	, UOnlineLocalUserSubsystem*			/*LocalUser*/
	, ELoginStatusType						/*NewStatus*/
	, FUniqueNetIdRepl						/*NetId*/
	, FOnlineServiceResult					/*Result*/
	, EOnlineServiceContext					/*Type*/);


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
	// Generally either CanPlay or CanPlayOnline, specifies what level of privilege is required
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EOnlinePrivilege RequestedPrivilege{ EOnlinePrivilege::CanPlay };

	//
	// What specific online context to log in to, game means to login to all relevant ones 
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EOnlineServiceContext OnlineContext{ EOnlineServiceContext::Default };

	//
	// True if we should not show login errors, the game will be responsible for displaying them
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bSuppressLoginErrors{ false };

	//
	// If bound, call this dynamic delegate at completion of login
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLocalUserLoginCompleteDynamicDelegate OnLocalUserLoginComplete;

};
