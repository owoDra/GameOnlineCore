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


/**
 * Wrapper for using OSS EOnlineService with blueprints
 */
UENUM(BlueprintType)
enum class EOnlineServiceType : uint8
{
	// Null, Providing minimal functionality when no backend services are required
	Null,
	// Epic Online Services
	Epic,
	// Xbox services
	Xbox,
	// PlayStation Network
	PSN,
	// Nintendo
	Nintendo,
	// Unused,
	Reserved_5		UMETA(hidden),
	// Steam
	Steam,
	// Google
	Google,
	// GooglePlay
	GooglePlay,
	// Apple
	Apple,
	// GameKit
	AppleGameKit,
	// Samsung
	Samsung,
	// Oculus
	Oculus,
	// Tencent
	Tencent,
	// Reserved for future use/platform extensions
	Reserved_14		UMETA(hidden),
	Reserved_15		UMETA(hidden),
	Reserved_16		UMETA(hidden),
	Reserved_17		UMETA(hidden),
	Reserved_18		UMETA(hidden),
	Reserved_19		UMETA(hidden),
	Reserved_20		UMETA(hidden),
	Reserved_21		UMETA(hidden),
	Reserved_22		UMETA(hidden),
	Reserved_23		UMETA(hidden),
	Reserved_24		UMETA(hidden),
	Reserved_25		UMETA(hidden),
	Reserved_26		UMETA(hidden),
	Reserved_27		UMETA(hidden),
	// For game specific Online Services
	GameDefined_0 = 28,
	GameDefined_1,
	GameDefined_2,
	GameDefined_3,
	// None, used internally to resolve Platform or Default if they are not configured
	None = 253,
	// Platform native, may not exist for all platforms
	Platform = 254	UMETA(hidden),
	// Default, configured via ini, TODO: List specific ini section/key
	Default = 255	UMETA(hidden)
};

