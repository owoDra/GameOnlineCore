// Copyright (C) 2024 owoDra

#pragma once

#include "OnlineLocalUserTypes.generated.h"


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


/**
 * Enum specifying the general availability of a feature or privilege, this combines information from multiple sources
 */
UENUM(BlueprintType)
enum class ELocalUserOnlineAvailability : uint8
{
	// State is completely unknown and needs to be queried
	Unknown,

	// This feature is fully available for use right now
	NowAvailable,

	// This might be available after the completion of normal login procedures
	PossiblyAvailable,

	// This feature is not available now because of something like network connectivity but may be available in the future
	CurrentlyUnavailable,

	// This feature will never be available for the rest of this session due to hard account or platform restrictions
	AlwaysUnavailable,

	// Invalid feature
	Invalid,
};


////////////////////////////////////////////////////////////////////////
// Delegates

/**
 * Delegate when a privilege changes, this can be bound to see if online status/etc changes during gameplay
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FLocalUserAvailabilityChangedDelegate
												, const ULocalPlayer*			, LocalPlayer
												, EOnlinePrivilege				, Privilege
												, ELocalUserOnlineAvailability	, OldAvailability
												, ELocalUserOnlineAvailability	, NewAvailability);
