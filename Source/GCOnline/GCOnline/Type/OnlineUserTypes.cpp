// Copyright (C) 2024 owoDra

#include "OnlineUserTypes.h"

#include "OnlineUserSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineUserTypes)


/////////////////////////////////////////////////////////////////
// ULocalUserInfo

ULocalUserAccountInfo::ULocalUserAccountInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


// Account Cache

const ULocalUserAccountInfo::FAccountInfoCache* ULocalUserAccountInfo::GetCachedAccountInfo(EOnlineServiceContext Context) const
{
	return const_cast<ULocalUserAccountInfo*>(this)->GetCachedAccountInfo(Context);
}

ULocalUserAccountInfo::FAccountInfoCache* ULocalUserAccountInfo::GetCachedAccountInfo(EOnlineServiceContext Context)
{
	// Look up directly, game has a separate cache than default

	if (auto* FoundData{ CachedAccountInfos.Find(Context) })
	{
		return FoundData;
	}

	// Now try system resolution

	auto ResolvedContext{ GetSubsystem()->ContextCaches.ResolveOnlineServiceContext(Context) };
	return CachedAccountInfos.Find(ResolvedContext);
}


void ULocalUserAccountInfo::UpdateCachedPrivilegeResult(ELocalUserPrivilege Privilege, ELocalUserPrivilegeResult Result, EOnlineServiceContext Context)
{
	// Cache old availability

	const auto OldAvailability{ GetPrivilegeAvailability(Privilege) };

	// This should only be called with a resolved and valid type

	auto* GameCache{ GetCachedAccountInfo(EOnlineServiceContext::Game) };
	auto* ContextCache{ GetCachedAccountInfo(Context) };

	// Ensure always be valid

	if (!ensure(GameCache && ContextCache))
	{
		return;
	}

	// Update direct cache first

	ContextCache->CachedPrivileges.Add(Privilege, Result);

	// Look for another context to merge into game

	if (GameCache != ContextCache)
	{
		auto GameContextResult{ Result };
		auto OtherContextResult{ ELocalUserPrivilegeResult::Available };

		for (auto& KVP : CachedAccountInfos)
		{
			if (&KVP.Value != ContextCache && &KVP.Value != GameCache)
			{
				if (auto* FoundResult{ KVP.Value.CachedPrivileges.Find(Privilege) })
				{
					OtherContextResult = *FoundResult;
				}
				else
				{
					OtherContextResult = ELocalUserPrivilegeResult::Unknown;
				}

				break;
			}
		}

		// Other context is worse, use that

		if (GameContextResult == ELocalUserPrivilegeResult::Available && OtherContextResult != ELocalUserPrivilegeResult::Available)
		{
			GameContextResult = OtherContextResult;
		}

		GameCache->CachedPrivileges.Add(Privilege, GameContextResult);
	}

	// Notify privilege change

	HandleChangedAvailability(Privilege, OldAvailability);
}

void ULocalUserAccountInfo::UpdateCachedNetId(const FUniqueNetIdRepl& NewId, EOnlineServiceContext Context)
{
	auto* Cache{ GetCachedAccountInfo(Context) };
	if (ensure(Cache))
	{
		Cache->CachedNetId = NewId;
	}
}

void ULocalUserAccountInfo::HandleChangedAvailability(ELocalUserPrivilege Privilege, ELocalUserAvailability OldAvailability)
{
	const auto NewAvailability{ GetPrivilegeAvailability(Privilege) };

	if (OldAvailability != NewAvailability)
	{
		OnUserPrivilegeChanged.Broadcast(this, Privilege, OldAvailability, NewAvailability);
	}
}


// Local Player Info

bool ULocalUserAccountInfo::IsLoggedIn() const
{
	return (LoginState == ELocalUserLoginState::LoggedInLocalOnly || LoginState == ELocalUserLoginState::LoggedInOnline);
}

bool ULocalUserAccountInfo::IsDoingLogin() const
{
	return (LoginState == ELocalUserLoginState::DoingInitialLogin || LoginState == ELocalUserLoginState::DoingNetworkLogin);
}


ELocalUserPrivilegeResult ULocalUserAccountInfo::GetCachedPrivilegeResult(ELocalUserPrivilege Privilege, EOnlineServiceContext Context) const
{
	if (auto* const FoundCached{ GetCachedAccountInfo(Context) })
	{
		if (auto* const FoundResult{ FoundCached->CachedPrivileges.Find(Privilege) })
		{
			return *FoundResult;
		}
	}

	return ELocalUserPrivilegeResult::Unknown;
}

ELocalUserAvailability ULocalUserAccountInfo::GetPrivilegeAvailability(ELocalUserPrivilege Privilege) const
{
	// Bad feature or user

	if ((static_cast<int32>(Privilege) < 0) ||
		(static_cast<int32>(Privilege) >= static_cast<int32>(ELocalUserPrivilege::Count)) ||
		(LoginState == ELocalUserLoginState::Invalid))
	{
		return ELocalUserAvailability::Invalid;
	}

	auto CachedResult{ GetCachedPrivilegeResult(Privilege) };

	// First handle explicit failures

	switch (CachedResult)
	{
	case ELocalUserPrivilegeResult::LicenseInvalid:
	case ELocalUserPrivilegeResult::VersionOutdated:
	case ELocalUserPrivilegeResult::AgeRestricted:
		return ELocalUserAvailability::AlwaysUnavailable;

	case ELocalUserPrivilegeResult::NetworkConnectionUnavailable:
	case ELocalUserPrivilegeResult::AccountTypeRestricted:
	case ELocalUserPrivilegeResult::AccountUseRestricted:
	case ELocalUserPrivilegeResult::PlatformFailure:
		return ELocalUserAvailability::CurrentlyUnavailable;

	default:
		break;
	}

	// Guests can only play, cannot use online features

	if (bIsGuest)
	{
		if (Privilege == ELocalUserPrivilege::CanPlay)
		{
			return ELocalUserAvailability::NowAvailable;
		}
		else
		{
			return ELocalUserAvailability::AlwaysUnavailable;
		}
	}

	// Check network status

	if ((Privilege == ELocalUserPrivilege::CanPlayOnline) ||
		(Privilege == ELocalUserPrivilege::CanUseCrossPlay) ||
		(Privilege == ELocalUserPrivilege::CanCommunicateViaTextOnline) ||
		(Privilege == ELocalUserPrivilege::CanCommunicateViaVoiceOnline))
	{
		auto* Subsystem{ GetSubsystem() };
		if (ensure(Subsystem) && !Subsystem->HasOnlineConnection(EOnlineServiceContext::Game))
		{
			return ELocalUserAvailability::CurrentlyUnavailable;
		}
	}

	// Failed a prior login attempt

	if (LoginState == ELocalUserLoginState::FailedToLogin)
	{
		return ELocalUserAvailability::CurrentlyUnavailable;
	}

	// Haven't logged in yet

	else if (LoginState == ELocalUserLoginState::Unknown || LoginState == ELocalUserLoginState::DoingInitialLogin)
	{
		return ELocalUserAvailability::PossiblyAvailable;
	}

	// Local login succeeded so play checks are valid

	else if (LoginState == ELocalUserLoginState::LoggedInLocalOnly || LoginState == ELocalUserLoginState::DoingNetworkLogin)
	{
		if (Privilege == ELocalUserPrivilege::CanPlay && CachedResult == ELocalUserPrivilegeResult::Available)
		{
			return ELocalUserAvailability::NowAvailable;
		}

		// Haven't logged in online yet

		return ELocalUserAvailability::PossiblyAvailable;
	}

	// Fully logged in

	else if (LoginState == ELocalUserLoginState::LoggedInOnline)
	{
		if (CachedResult == ELocalUserPrivilegeResult::Available)
		{
			return ELocalUserAvailability::NowAvailable;
		}

		// Failed for other reason

		return ELocalUserAvailability::CurrentlyUnavailable;
	}

	return ELocalUserAvailability::Unknown;
}


FUniqueNetIdRepl ULocalUserAccountInfo::GetNetId(EOnlineServiceContext Context) const
{
	if (const auto* FoundCached{ GetCachedAccountInfo(Context) })
	{
		return FoundCached->CachedNetId;
	}

	return FUniqueNetIdRepl();
}

FString ULocalUserAccountInfo::GetNickname() const
{
	if (bIsGuest)
	{
		return NSLOCTEXT("GameOnlineCore", "GuestNickname", "Guest").ToString();
	}

	if (const auto* Subsystem{ GetSubsystem() })
	{
		if (auto AuthService{ Subsystem->GetOnlineAuth(EOnlineServiceContext::Game) })
		{
			if (auto AccountInfo{ Subsystem->GetOnlineServiceAccountInfo(AuthService, GetPlatformUserId()) })
			{
				if (const auto* DisplayName{ AccountInfo->Attributes.Find(AccountAttributeData::DisplayName) })
				{
					return DisplayName->GetString();
				}
			}
		}
	}

	return FString();
}

FPlatformUserId ULocalUserAccountInfo::GetPlatformUserId() const
{
	return PlatformUser;
}


// Utilities

UOnlineUserSubsystem* ULocalUserAccountInfo::GetSubsystem() const
{
	return Cast<UOnlineUserSubsystem>(GetOuter());
}

FString ULocalUserAccountInfo::GetDebugString() const
{
	return GetNetId().ToDebugString();
}
