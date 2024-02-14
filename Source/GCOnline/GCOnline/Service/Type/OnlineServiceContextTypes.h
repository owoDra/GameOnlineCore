// Copyright (C) 2024 owoDra

#pragma once

#include "OnlineServiceContextTypes.generated.h"


/** 
 * Enum specifying where and how to run online queries 
 */
UENUM(BlueprintType)
enum class EOnlineServiceContext : uint8
{
	// The default engine online system, this will always exist and will be the same as either Service or Platform
	Default,

	// Explicitly ask for the platform system, which may not exist
	Platform,

	// Invalid system
	Invalid		UMETA(hidden),

	// Looks for platform system first, then falls back to default
	PlatformOrDefault
};
