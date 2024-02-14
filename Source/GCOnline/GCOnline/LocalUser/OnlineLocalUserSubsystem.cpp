// Copyright (C) 2024 owoDra

#include "OnlineLocalUserSubsystem.h"

#include "OnlineServiceSubsystem.h"

#include "Online/OnlineServices.h"
#include "Online/Auth.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineLocalUserSubsystem)


// Initialization

bool UOnlineLocalUserSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}

void UOnlineLocalUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	ResetLocalUser();
}


void UOnlineLocalUserSubsystem::InitializeLocalUser(FInputDeviceId InPrimaryInputDevice, bool bCanUseGuestLogin)
{
	bLocalUserInitialized = true;

	PrimaryInputDeviceId = InPrimaryInputDevice;
	PlatformUserId = IPlatformInputDeviceMapper::Get().GetUserForInputDevice(PrimaryInputDeviceId);

	auto* LocalPlayer{ GetLocalPlayerChecked() };

	bCanBeGuest = LocalPlayer->IsPrimaryPlayer() ? false : bCanUseGuestLogin;

	LocalPlayer->SetPlatformUserId(PlatformUserId);
}

void UOnlineLocalUserSubsystem::ResetLocalUser()
{
	bLocalUserInitialized = false;

	CachedPrivilegeResults.Reset();
	CachedAccountInfos.Reset();
	PrimaryInputDeviceId = FInputDeviceId();
	PlatformUserId = FPlatformUserId();
	bCanBeGuest = false;
	bIsGuest = false;
	LoginState = ELocalUserLoginState::Invalid;

	auto* LocalPlayer{ GetLocalPlayer() };
	if (LocalPlayer && LocalPlayer->IsPrimaryPlayer())
	{
		InitializeLocalUser(IPlatformInputDeviceMapper::Get().GetDefaultInputDevice(), false);
	}
}


// Account Cache

void UOnlineLocalUserSubsystem::UpdateCachedPrivilegeResult(EOnlinePrivilege Privilege, EOnlinePrivilegeResult Result, EOnlineServiceContext Context)
{
	if (!ensure(Context < EOnlineServiceContext::Invalid))
	{
		return;
	}

	// Cache old availability

	const auto OldAvailability{ GetPrivilegeAvailability(Privilege) };

	// Find or add results cache for context

	auto& Cache{ CachedPrivilegeResults.FindOrAdd(Context) };

	// Update direct cache first

	Cache.CachedPrivileges.Add(Privilege, Result);

	// Notify privilege change

	HandleChangedAvailability(Privilege, OldAvailability);
}

void UOnlineLocalUserSubsystem::UpdateCachedAccountInfo(const TSharedPtr<FAccountInfo>& InAccountInfo, EOnlineServiceContext Context)
{
	check(InAccountInfo);

	if (!ensure(Context < EOnlineServiceContext::Invalid))
	{
		return;
	}

	// Find or add account cache for context

	auto& Cache{ CachedAccountInfos.FindOrAdd(Context) };

	// Update direct cache first

	Cache = InAccountInfo;

	// Set Unique id for localplayer and playerstate

	if (Context == EOnlineServiceContext::Default)
	{
		auto NetId{ FUniqueNetIdRepl(Cache->AccountId) };

		auto* LocalPlayer{ GetLocalPlayerChecked() };
		LocalPlayer->SetCachedUniqueNetId(NetId);

		auto* PlayerController{ LocalPlayer->GetPlayerController(nullptr) };
		auto PlayerState{ PlayerController ? PlayerController->PlayerState : nullptr };

		if (PlayerState)
		{
			PlayerState->SetUniqueId(NetId);
		}
	}
}

void UOnlineLocalUserSubsystem::HandleChangedAvailability(EOnlinePrivilege Privilege, ELocalUserOnlineAvailability OldAvailability)
{
	const auto NewAvailability{ GetPrivilegeAvailability(Privilege) };

	if (OldAvailability != NewAvailability)
	{
		OnLocalUserAvailabilityChanged.Broadcast(GetLocalPlayer(), Privilege, OldAvailability, NewAvailability);
	}
}


// Local User

bool UOnlineLocalUserSubsystem::IsLoggedIn() const
{
	return (LoginState == ELocalUserLoginState::LoggedInLocalOnly || LoginState == ELocalUserLoginState::LoggedInOnline);
}

bool UOnlineLocalUserSubsystem::IsDoingLogin() const
{
	return (LoginState == ELocalUserLoginState::DoingInitialLogin || LoginState == ELocalUserLoginState::DoingNetworkLogin);
}

TSharedPtr<FAccountInfo> UOnlineLocalUserSubsystem::GetCachedAccountInfo(EOnlineServiceContext Context) const
{
	// Look up directly, game has a separate cache than default

	if (const auto* FoundInfo{ CachedAccountInfos.Find(Context) })
	{
		return *FoundInfo;
	}

	// Now try system resolution

	else if (auto* Subsystem{ UGameInstance::GetSubsystem<UOnlineServiceSubsystem>(GetGameInstance()) })
	{
		auto ResolvedContext{ Subsystem->ResolveOnlineServiceContext(Context) };

		if (const auto* FoundInfo_Resolved{ CachedAccountInfos.Find(ResolvedContext) })
		{
			return *FoundInfo_Resolved;
		}
	}

	return nullptr;
}

EOnlinePrivilegeResult UOnlineLocalUserSubsystem::GetCachedPrivilegeResult(EOnlinePrivilege Privilege, EOnlineServiceContext Context) const
{
	const FPrivilegeCache* Cache{ nullptr };

	// Look up directly, game has a separate cache than default

	if (const auto* FoundData{ CachedPrivilegeResults.Find(Context) })
	{
		Cache = FoundData;
	}

	// Now try system resolution

	else if (auto* Subsystem{ UGameInstance::GetSubsystem<UOnlineServiceSubsystem>(GetGameInstance()) })
	{
		auto ResolvedContext{ Subsystem->ResolveOnlineServiceContext(Context) };

		Cache = CachedPrivilegeResults.Find(ResolvedContext);
	}

	// Looking for result from cache
	
	if (Cache)
	{
		if (const auto* FoundResult{ Cache->CachedPrivileges.Find(Privilege) })
		{
			return *FoundResult;
		}
	}

	return EOnlinePrivilegeResult::Unknown;
}

ELocalUserOnlineAvailability UOnlineLocalUserSubsystem::GetPrivilegeAvailability(EOnlinePrivilege Privilege, EOnlineServiceContext Context) const
{
	// Bad feature or user

	if ((static_cast<int32>(Privilege) < 0) ||
		(static_cast<int32>(Privilege) >= static_cast<int32>(EOnlinePrivilege::Count)) ||
		(LoginState == ELocalUserLoginState::Invalid))
	{
		return ELocalUserOnlineAvailability::Invalid;
	}

	auto CachedResult{ GetCachedPrivilegeResult(Privilege, Context) };

	// First handle explicit failures

	switch (CachedResult)
	{
	case EOnlinePrivilegeResult::LicenseInvalid:
	case EOnlinePrivilegeResult::VersionOutdated:
	case EOnlinePrivilegeResult::AgeRestricted:
		return ELocalUserOnlineAvailability::AlwaysUnavailable;

	case EOnlinePrivilegeResult::NetworkConnectionUnavailable:
	case EOnlinePrivilegeResult::AccountTypeRestricted:
	case EOnlinePrivilegeResult::AccountUseRestricted:
	case EOnlinePrivilegeResult::PlatformFailure:
		return ELocalUserOnlineAvailability::CurrentlyUnavailable;

	default:
		break;
	}

	// Guests can only play, cannot use online features

	if (bIsGuest)
	{
		if (Privilege == EOnlinePrivilege::CanPlay)
		{
			return ELocalUserOnlineAvailability::NowAvailable;
		}
		else
		{
			return ELocalUserOnlineAvailability::AlwaysUnavailable;
		}
	}

	// Check network status

	if ((Privilege == EOnlinePrivilege::CanPlayOnline) ||
		(Privilege == EOnlinePrivilege::CanUseCrossPlay) ||
		(Privilege == EOnlinePrivilege::CanCommunicateViaTextOnline) ||
		(Privilege == EOnlinePrivilege::CanCommunicateViaVoiceOnline))
	{
		/// @TODO Check by connectivity subsystem

		/*auto* Subsystem{ GetSubsystem() };
		if (ensure(Subsystem) && !Subsystem->HasOnlineConnection(EOnlineServiceContext::Game))
		{
			return ELocalUserOnlineAvailability::CurrentlyUnavailable;
		}*/
	}

	// Failed a prior login attempt

	if (LoginState == ELocalUserLoginState::FailedToLogin)
	{
		return ELocalUserOnlineAvailability::CurrentlyUnavailable;
	}

	// Haven't logged in yet

	else if (LoginState == ELocalUserLoginState::Unknown || LoginState == ELocalUserLoginState::DoingInitialLogin)
	{
		return ELocalUserOnlineAvailability::PossiblyAvailable;
	}

	// Local login succeeded so play checks are valid

	else if (LoginState == ELocalUserLoginState::LoggedInLocalOnly || LoginState == ELocalUserLoginState::DoingNetworkLogin)
	{
		if (Privilege == EOnlinePrivilege::CanPlay && CachedResult == EOnlinePrivilegeResult::Available)
		{
			return ELocalUserOnlineAvailability::NowAvailable;
		}

		// Haven't logged in online yet

		return ELocalUserOnlineAvailability::PossiblyAvailable;
	}

	// Fully logged in

	else if (LoginState == ELocalUserLoginState::LoggedInOnline)
	{
		if (CachedResult == EOnlinePrivilegeResult::Available)
		{
			return ELocalUserOnlineAvailability::NowAvailable;
		}

		// Failed for other reason

		return ELocalUserOnlineAvailability::CurrentlyUnavailable;
	}

	return ELocalUserOnlineAvailability::Unknown;
}

FUniqueNetIdRepl UOnlineLocalUserSubsystem::GetNetId(EOnlineServiceContext Context) const
{
	if (!bIsGuest)
	{
		if (auto AccountInfo{ GetCachedAccountInfo(Context) })
		{
			return FUniqueNetIdRepl({ AccountInfo->AccountId });
		}
	}

	return FUniqueNetIdRepl();
}

FString UOnlineLocalUserSubsystem::GetNickname(EOnlineServiceContext Context) const
{
	if (bIsGuest)
	{
		return NSLOCTEXT("GameOnlineCore", "GuestNickname", "Guest").ToString();
	}

	if (auto AccountInfo{ GetCachedAccountInfo(Context) })
	{
		if (auto* DisplayName{ AccountInfo->Attributes.Find(AccountAttributeData::DisplayName) })
		{
			return DisplayName->GetString();
		}
	}

	return FString();
}


// Utilities

UGameInstance* UOnlineLocalUserSubsystem::GetGameInstance() const
{
	return GetLocalPlayerChecked()->GetGameInstance();
}

FString UOnlineLocalUserSubsystem::GetDebugString() const
{
	return GetNetId().ToDebugString();
}
