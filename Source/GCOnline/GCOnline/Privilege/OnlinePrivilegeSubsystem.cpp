// Copyright (C) 2024 owoDra

#include "OnlinePrivilegeSubsystem.h"

#include "OnlineDeveloperSettings.h"
#include "OnlineServiceSubsystem.h"
#include "OnlineLocalUserSubsystem.h"
#include "GCOnlineLogs.h"

// OSS v2
#include "Online/Privileges.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlinePrivilegeSubsystem)


// Initialization

void UOnlinePrivilegeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	OnlineServiceSubsystem = Collection.InitializeDependency<UOnlineServiceSubsystem>();

	check(OnlineServiceSubsystem);
}

void UOnlinePrivilegeSubsystem::Deinitialize()
{
	OnlineServiceSubsystem = nullptr;
}

bool UOnlinePrivilegeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}


// Context Cache

IPrivilegesPtr UOnlinePrivilegeSubsystem::GetPrivilegesInterface(EOnlineServiceContext Context) const
{
	auto OnlineService{ OnlineServiceSubsystem->GetContextCache() };

	if (ensure(OnlineService))
	{
		return OnlineService->GetPrivilegesInterface();
	}

	return nullptr;
}


// Privilege

EOnlinePrivilege UOnlinePrivilegeSubsystem::ConvertOnlineServicesPrivilege(EUserPrivileges Privilege) const
{
	switch (Privilege)
	{
	case EUserPrivileges::CanPlay:
		return EOnlinePrivilege::CanPlay;
	case EUserPrivileges::CanPlayOnline:
		return EOnlinePrivilege::CanPlayOnline;
	case EUserPrivileges::CanCommunicateViaTextOnline:
		return EOnlinePrivilege::CanCommunicateViaTextOnline;
	case EUserPrivileges::CanCommunicateViaVoiceOnline:
		return EOnlinePrivilege::CanCommunicateViaVoiceOnline;
	case EUserPrivileges::CanUseUserGeneratedContent:
		return EOnlinePrivilege::CanUseUserGeneratedContent;
	case EUserPrivileges::CanCrossPlay:
		return EOnlinePrivilege::CanUseCrossPlay;
	default:
		return EOnlinePrivilege::Invalid;
	}
}

EUserPrivileges UOnlinePrivilegeSubsystem::ConvertOnlineServicesPrivilege(EOnlinePrivilege Privilege) const
{
	switch (Privilege)
	{
	case EOnlinePrivilege::CanPlay:
		return EUserPrivileges::CanPlay;
	case EOnlinePrivilege::CanPlayOnline:
		return EUserPrivileges::CanPlayOnline;
	case EOnlinePrivilege::CanCommunicateViaTextOnline:
		return EUserPrivileges::CanCommunicateViaTextOnline;
	case EOnlinePrivilege::CanCommunicateViaVoiceOnline:
		return EUserPrivileges::CanCommunicateViaVoiceOnline;
	case EOnlinePrivilege::CanUseUserGeneratedContent:
		return EUserPrivileges::CanUseUserGeneratedContent;
	case EOnlinePrivilege::CanUseCrossPlay:
		return EUserPrivileges::CanCrossPlay;
	default:
		// No failure type, return CanPlay
		return EUserPrivileges::CanPlay;
	}
}

EOnlinePrivilegeResult UOnlinePrivilegeSubsystem::ConvertOnlineServicesPrivilegeResult(EUserPrivileges Privilege, EPrivilegeResults Results) const
{
	if (Results == EPrivilegeResults::NoFailures)
	{
		return EOnlinePrivilegeResult::Available;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::UserNotFound | EPrivilegeResults::UserNotLoggedIn))
	{
		return EOnlinePrivilegeResult::UserNotLoggedIn;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::RequiredPatchAvailable | EPrivilegeResults::RequiredSystemUpdate))
	{
		return EOnlinePrivilegeResult::VersionOutdated;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::AgeRestrictionFailure))
	{
		return EOnlinePrivilegeResult::AgeRestricted;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::AccountTypeFailure))
	{
		return EOnlinePrivilegeResult::AccountTypeRestricted;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::NetworkConnectionUnavailable))
	{
		return EOnlinePrivilegeResult::NetworkConnectionUnavailable;
	}

	// Bucket other account failures together

	const auto AccountUseFailures{ EPrivilegeResults::OnlinePlayRestricted | EPrivilegeResults::UGCRestriction | EPrivilegeResults::ChatRestriction };

	if (EnumHasAnyFlags(Results, AccountUseFailures))
	{
		return EOnlinePrivilegeResult::AccountUseRestricted;
	}

	// If you can't play at all, this is a license failure

	if (Privilege == EUserPrivileges::CanPlay)
	{
		return EOnlinePrivilegeResult::LicenseInvalid;
	}

	// Unknown reason

	return EOnlinePrivilegeResult::PlatformFailure;
}


FText UOnlinePrivilegeSubsystem::GetPrivilegeDescription(EOnlineServiceContext Context, EOnlinePrivilege Privilege) const
{
	const auto* DevSetting{ GetDefault<UOnlineDeveloperSettings>() };

	if (ensure(DevSetting))
	{
		return DevSetting->GetPrivilegesDescription(OnlineServiceSubsystem->ResolveOnlineServiceContext(Context), Privilege);
	}

	return FText::GetEmpty();
}

FText UOnlinePrivilegeSubsystem::GetPrivilegeResultDescription(EOnlineServiceContext Context, EOnlinePrivilegeResult Result) const
{
	const auto* DevSetting{ GetDefault<UOnlineDeveloperSettings>() };

	if (ensure(DevSetting))
	{
		return DevSetting->GetPrivilegesResultDescription(OnlineServiceSubsystem->ResolveOnlineServiceContext(Context), Result);
	}

	return FText::GetEmpty();
}


bool UOnlinePrivilegeSubsystem::QueryUserPrivilege(const ULocalPlayer* LocalPlayer, EOnlineServiceContext Context, EOnlinePrivilege DesiredPrivilege, FOnlinePrivilegeQueryDelegate Delegate)
{
	if (ensure(LocalPlayer))
	{
		auto* Subsystem{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer) };
		check(Subsystem);

		if (ensure(Subsystem->HasLocalUserInitialized()))
		{
			auto PrivilegesInterface{ GetPrivilegesInterface(Context) };

			if (PrivilegesInterface.IsValid())
			{
				const auto DesiredPrivilege_OSS{ ConvertOnlineServicesPrivilege(DesiredPrivilege) };

				FQueryUserPrivilege::Params Params;
				Params.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();
				Params.Privilege = DesiredPrivilege_OSS;

				auto QueryHandle{ PrivilegesInterface->QueryUserPrivilege(MoveTemp(Params)) };
				QueryHandle.OnComplete(this, &ThisClass::HandleQueryPrivilegeComplete, LocalPlayer, Context, DesiredPrivilege_OSS, Delegate);
			}
			else
			{
				Subsystem->UpdateCachedPrivilegeResult(DesiredPrivilege, EOnlinePrivilegeResult::Available, Context);
				Delegate.ExecuteIfBound(LocalPlayer, Context, DesiredPrivilege, EOnlinePrivilegeResult::Available, FOnlineServiceResult());
			}
			
			return true;
		}
	}

	return false;
}

void UOnlinePrivilegeSubsystem::HandleQueryPrivilegeComplete(const TOnlineResult<FQueryUserPrivilege>& Result, const ULocalPlayer* LocalPlayer, EOnlineServiceContext Context, EUserPrivileges DesiredPrivilege, FOnlinePrivilegeQueryDelegate Delegate)
{
	check(LocalPlayer);
	auto* Subsystem{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer) };
	check(Subsystem);

	auto LocalUserPrivilege{ ConvertOnlineServicesPrivilege(DesiredPrivilege) };
	auto LocalUserPrivilegeResult{ EOnlinePrivilegeResult::PlatformFailure };

	if (const auto* OkResult{ Result.TryGetOkValue() })
	{
		LocalUserPrivilegeResult = ConvertOnlineServicesPrivilegeResult(DesiredPrivilege, OkResult->PrivilegeResult);
	}
	else
	{
		UE_LOG(LogGameCore_OnlinePrivileges, Warning, TEXT("QueryUserPrivilege failed: %s"), *Result.GetErrorValue().GetLogString());
	}

	Subsystem->UpdateCachedPrivilegeResult(LocalUserPrivilege, LocalUserPrivilegeResult, Context);
	Delegate.ExecuteIfBound(LocalPlayer, Context, LocalUserPrivilege, LocalUserPrivilegeResult, { Result.IsError() ? Result.GetErrorValue() : Errors::Unknown() });
}
