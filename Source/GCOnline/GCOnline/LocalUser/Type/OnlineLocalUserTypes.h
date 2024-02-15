// Copyright (C) 2024 owoDra

#pragma once

#include "OnlineLocalUserTypes.generated.h"


////////////////////////////////////////////////////////////////////////
// Enums

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
