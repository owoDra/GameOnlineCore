// Copyright (C) 2024 owoDra

#include "OnlineServiceSubsystem.h"

#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineServiceSubsystem)


// Initialization

bool UOnlineServiceSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}

void UOnlineServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	CreateOnlineServiceContexts();
}

void UOnlineServiceSubsystem::Deinitialize()
{
	DestroyOnlineServiceContexts();
}


// Context Cache

void UOnlineServiceSubsystem::CreateOnlineServiceContexts()
{
	const auto* World{ GetWorld() };
	check(World);

	// Cache default service

	DefaultService = GetServices(World, EOnlineServices::Default);

	// Cache platform service

	auto PlatformServices{ GetServices(World, EOnlineServices::Platform) };
	if (PlatformServices && DefaultService != PlatformServices)
	{
		PlatformService = PlatformServices;
	}
}

void UOnlineServiceSubsystem::DestroyOnlineServiceContexts()
{
	// All cached shared ptrs must be cleared here

	DefaultService.Reset();
	PlatformService.Reset();
}


IOnlineServicesPtr UOnlineServiceSubsystem::GetContextCache(EOnlineServiceContext Context) const
{
	switch (Context)
	{
	case EOnlineServiceContext::Default:
		return DefaultService;

	case EOnlineServiceContext::Platform:
		return PlatformService;

	case EOnlineServiceContext::PlatformOrDefault:
		return PlatformService ? PlatformService : DefaultService;
	}

	return nullptr;
}

EOnlineServiceContext UOnlineServiceSubsystem::ResolveOnlineServiceContext(EOnlineServiceContext Context) const
{
	switch (Context)
	{
	case EOnlineServiceContext::Default:
		return EOnlineServiceContext::Default;

	case EOnlineServiceContext::Platform:
		return PlatformService ? EOnlineServiceContext::Platform : EOnlineServiceContext::Invalid;

	case EOnlineServiceContext::PlatformOrDefault:
		return PlatformService ? EOnlineServiceContext::Platform : EOnlineServiceContext::Default;
	}

	return EOnlineServiceContext::Invalid;
}

bool UOnlineServiceSubsystem::HasSeparatePlatformContext() const
{
	return PlatformService.IsValid();
}


// Error Message

void UOnlineServiceSubsystem::SendErrorMessage(const FOnlineServiceResult& InResult)
{
	OnOnlineServiceErrorMessage.Broadcast(InResult);
}
