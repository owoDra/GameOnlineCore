// Copyright (C) 2024 owoDra

#pragma once

#include "Online/OnlineError.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEngineUtils.h"

#include "OnlineServiceTypes.generated.h"

namespace UE::Online
{
	using IOnlineServicesPtr = TSharedPtr<class IOnlineServices>;
}
using namespace UE::Online;

using FOnlineErrorType = UE::Online::FOnlineError;


////////////////////////////////////////////////////////////////////////
// Enums

/** 
 * Enum specifying where and how to run online queries 
 */
UENUM(BlueprintType)
enum class EOnlineServiceContext : uint8
{
	// Called from game code, this uses the default system but with special handling that could merge results from multiple contexts
	Game,

	// The default engine online system, this will always exist and will be the same as either Service or Platform
	Default,

	// Explicitly ask for the platform system, which may not exist
	Platform,

	// Looks for platform system first, then falls back to default
	PlatformOrDefault,

	// Invalid system
	Invalid
};


/** 
 * Used to track the progress of different asynchronous operations 
 */
enum class EOnlineServiceTaskState : uint8
{
	// The task has not been started
	NotStarted,

	// The task is currently being processed
	InProgress,

	// The task has completed successfully
	Done,

	// The task failed to complete
	Failed
};


////////////////////////////////////////////////////////////////////////
// Structs

/** 
 * Detailed information about the online error. Effectively a wrapper for FOnlineError. 
 */
USTRUCT(BlueprintType)
struct FOnlineServiceTaskResult
{
	GENERATED_BODY()
public:
	FOnlineServiceTaskResult() = default;

public:
	//
	// Whether the operation was successful or not
	// 
	// Tips:
	//	If it was successful, the error fields of this struct will not contain extra information
	//
	UPROPERTY(BlueprintReadOnly)
	bool bWasSuccessful{ true };

	//
	// The unique error id. Can be used to compare against specific handled errors
	//
	UPROPERTY(BlueprintReadOnly)
	FString ErrorId;

	//
	// Error text to display to the user
	//
	UPROPERTY(BlueprintReadOnly)
	FText ErrorText;

	/**
	 * Initialize this from an FOnlineError
	 */
	void GCONLINE_API FromOnlineError(const FOnlineErrorType& InOnlineError);

};


/**
 * Container of data to store pointers for accessing the interface for each online service
 */
template<typename TContextCacheType>
struct TOnlineServiceContextContainer
{
public:
	TOnlineServiceContextContainer() = default;
	
	TOnlineServiceContextContainer(const UWorld* InWorld)
	{
		CreateOnlineServiceContexts(InWorld);
	}

	~TOnlineServiceContextContainer()
	{
		DestroyOnlineServiceContexts();
	}

public:
	//
	// ContextCache per OnlineService
	// 
	// Note:
	//  Do not access this outside of initialization
	//
	TContextCacheType* DefaultServiceContextCache{ nullptr };
	TContextCacheType* PlatformServiceContextCache{ nullptr };

public:
	FORCEINLINE void CreateOnlineServiceContexts(const UWorld* InWorld)
	{
		check(InWorld);

		// Cache default service

		DefaultServiceContextCache = new TContextCacheType(GetServices(InWorld, EOnlineServices::Default));

		// Cache platform service

		auto PlatformServices{ GetServices(InWorld, EOnlineServices::Platform) };
		if (PlatformServices && DefaultServiceContextCache->OnlineServices != PlatformServices)
		{
			PlatformServiceContextCache = new TContextCacheType(PlatformServices);
		}
	}
	FORCEINLINE void DestroyOnlineServiceContexts()
	{
		// All cached shared ptrs must be cleared here

		if (PlatformServiceContextCache)
		{
			delete PlatformServiceContextCache;
		}

		if (DefaultServiceContextCache)
		{
			delete DefaultServiceContextCache;
		}

		PlatformServiceContextCache = nullptr;
		DefaultServiceContextCache = nullptr;
	}

public:
	/**
	* Gets internal data for a type of online service, can return null for service
	*/
	FORCEINLINE TContextCacheType* GetContextCache(EOnlineServiceContext Context = EOnlineServiceContext::Game)
	{
		switch (Context)
		{
		case EOnlineServiceContext::Game:
		case EOnlineServiceContext::Default:
			return DefaultServiceContextCache;

		case EOnlineServiceContext::Platform:
			return PlatformServiceContextCache;
		case EOnlineServiceContext::PlatformOrDefault:
			return PlatformServiceContextCache ? PlatformServiceContextCache : DefaultServiceContextCache;
		}

		return nullptr;
	}
	FORCEINLINE const TContextCacheType* GetContextCache(EOnlineServiceContext Context = EOnlineServiceContext::Game) const
	{
		switch (Context)
		{
		case EOnlineServiceContext::Game:
		case EOnlineServiceContext::Default:
			return DefaultServiceContextCache;

		case EOnlineServiceContext::Platform:
			return PlatformServiceContextCache;
		case EOnlineServiceContext::PlatformOrDefault:
			return PlatformServiceContextCache ? PlatformServiceContextCache : DefaultServiceContextCache;
		}

		return nullptr;
	}

	/**
	 * Resolves a context that has default behavior into a specific context
	 */
	FORCEINLINE EOnlineServiceContext ResolveOnlineServiceContext(EOnlineServiceContext Context) const
	{
		switch (Context)
		{
		case EOnlineServiceContext::Game:
		case EOnlineServiceContext::Default:
			return EOnlineServiceContext::Default;

		case EOnlineServiceContext::Platform:
			return PlatformServiceContextCache ? EOnlineServiceContext::Platform : EOnlineServiceContext::Invalid;
		case EOnlineServiceContext::PlatformOrDefault:
			return PlatformServiceContextCache ? EOnlineServiceContext::Platform : EOnlineServiceContext::Default;
		}

		return EOnlineServiceContext::Invalid;
	}

	/**
	 * True if there is a separate platform and service interface
	 */
	FORCEINLINE bool HasSeparatePlatformContext() const
	{
		auto ServiceType{ ResolveOnlineServiceContext(EOnlineServiceContext::Default) };
		auto PlatformType{ ResolveOnlineServiceContext(EOnlineServiceContext::PlatformOrDefault) };

		return (ServiceType != PlatformType);
	}

};
