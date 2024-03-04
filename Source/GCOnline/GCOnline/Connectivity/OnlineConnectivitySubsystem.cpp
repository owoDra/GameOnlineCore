// Copyright (C) 2024 owoDra

#include "OnlineConnectivitySubsystem.h"

#include "OnlineDeveloperSettings.h"
#include "OnlineServiceSubsystem.h"
#include "OnlineLocalUserSubsystem.h"
#include "GCOnlineLogs.h"

// OSS v2
#include "Online/OnlineResult.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineConnectivitySubsystem)


// Initialization

void UOnlineConnectivitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	OnlineServiceSubsystem = Collection.InitializeDependency<UOnlineServiceSubsystem>();

	check(OnlineServiceSubsystem);

	BindConnectivityDelegates();
}

void UOnlineConnectivitySubsystem::Deinitialize()
{
	OnlineServiceSubsystem = nullptr;

	UnbindConnectivityDelegates();
}

bool UOnlineConnectivitySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (Cast<UGameInstance>(Outer)->IsDedicatedServerInstance())
	{
		return false;
	}

	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}


void UOnlineConnectivitySubsystem::BindConnectivityDelegates()
{
	auto BindServiceDelegates
	{
		[this](EOnlineServiceContext Context)
		{
			if (auto ConnectivityInterface{ GetConecctivityInterface(Context) })
			{
				auto& Handle{ ConnectionHandles.FindOrAdd(Context) };

				Handle = ConnectivityInterface->OnConnectionStatusChanged().Add(this, &ThisClass::HandleNetworkConnectionStatusChanged, Context);

				// Cache connection status

				FConnectionStatusChanged EventParams;
				EventParams.PreviousStatus = EOnlineServicesConnectionStatus::NotConnected;
				EventParams.CurrentStatus = EOnlineServicesConnectionStatus::NotConnected;

				const auto Result{ ConnectivityInterface->GetConnectionStatus(FGetConnectionStatus::Params()) };

				if (Result.IsOk())
				{
					EventParams.CurrentStatus = Result.GetOkValue().Status;
				}

				HandleNetworkConnectionStatusChanged(EventParams, Context);
			}

			// Set connected if Connectivity Interface not imple

			else
			{
				FConnectionStatusChanged EventParams;
				EventParams.PreviousStatus = EOnlineServicesConnectionStatus::NotConnected;
				EventParams.CurrentStatus = EOnlineServicesConnectionStatus::Connected;

				HandleNetworkConnectionStatusChanged(EventParams, Context);
			}
		}
	};

	// Default Service

	BindServiceDelegates(EOnlineServiceContext::Default);

	// Platform Service

	BindServiceDelegates(EOnlineServiceContext::Platform);
}

void UOnlineConnectivitySubsystem::UnbindConnectivityDelegates()
{
	for (auto It{ ConnectionHandles.CreateIterator() }; It; ++It)
	{
		It->Value.Unbind();
		It.RemoveCurrent();
	}

	ConnectionStatusCaches.Reset();
}


IConnectivityPtr UOnlineConnectivitySubsystem::GetConecctivityInterface(EOnlineServiceContext Context) const
{
	if (!OnlineServiceSubsystem->IsOnlineServiceReady())
	{
		return nullptr;
	}

	auto OnlineService{ OnlineServiceSubsystem->GetContextCache() };

	if (ensure(OnlineService))
	{
		return OnlineService->GetConnectivityInterface();
	}

	return nullptr;
}


// Connectivity

void UOnlineConnectivitySubsystem::HandleNetworkConnectionStatusChanged(const FConnectionStatusChanged& EventParameters, EOnlineServiceContext Context)
{
	UE_LOG(LogGameCore_OnlineConnectivity, Log, TEXT("HandleNetworkConnectionStatusChanged(Context:%d, ServiceName:%s, OldStatus:%s, NewStatus:%s)"),
		static_cast<int32>(Context),
		*EventParameters.ServiceName,
		LexToString(EventParameters.PreviousStatus),
		LexToString(EventParameters.CurrentStatus));

	// Cache old availablity for current users

	TMap<UOnlineLocalUserSubsystem*, ELocalUserOnlineAvailability> AvailabilityMap;

	for (auto It{ GetGameInstance()->GetLocalPlayerIterator() }; It; ++It)
	{
		if (auto* LocalUserSubsystem{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(*It) })
		{
			AvailabilityMap.Add(LocalUserSubsystem, LocalUserSubsystem->GetPrivilegeAvailability(EOnlinePrivilege::CanPlayOnline, Context));
		}
	}

	// Update connection status

	auto& Cache{ ConnectionStatusCaches.FindOrAdd(Context) };
	Cache = EventParameters.CurrentStatus;

	// Notify other systems when someone goes online/offline

	for (const auto& KVP : AvailabilityMap)
	{
		KVP.Key->HandleChangedAvailability(EOnlinePrivilege::CanPlayOnline, KVP.Value);
	}
}


EOnlineServicesConnectionStatus UOnlineConnectivitySubsystem::GetConnectionStatus(EOnlineServiceContext Context) const
{
	if (const auto* Cache{ ConnectionStatusCaches.Find(Context) })
	{
		return *Cache;
	}

	const auto ResolvedContext{ OnlineServiceSubsystem->ResolveOnlineServiceContext(Context) };

	if (const auto* ResolvedCache{ ConnectionStatusCaches.Find(ResolvedContext) })
	{
		return *ResolvedCache;
	}

	return EOnlineServicesConnectionStatus::NotConnected;
}

bool UOnlineConnectivitySubsystem::HasOnlineConnection(EOnlineServiceContext Context) const
{
	return GetConnectionStatus(Context) == EOnlineServicesConnectionStatus::Connected;
}
