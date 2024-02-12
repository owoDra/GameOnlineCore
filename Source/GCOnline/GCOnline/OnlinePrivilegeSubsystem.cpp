// Copyright (C) 2024 owoDra

#include "OnlinePrivilegeSubsystem.h"

#include "Type/OnlineUserTypes.h"
#include "OnlineDeveloperSettings.h"
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
	Super::Initialize(Collection);

	ContextCaches.CreateOnlineServiceContexts(GetWorld());
}

void UOnlinePrivilegeSubsystem::Deinitialize()
{
	Super::Deinitialize();

	ContextCaches.DestroyOnlineServiceContexts();
}

bool UOnlinePrivilegeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}


// Context Cache

IPrivilegesPtr UOnlinePrivilegeSubsystem::GetOnlinePrivileges(EOnlineServiceContext Context) const
{
	const auto* System{ ContextCaches.GetContextCache(Context) };

	if (System && System->IsValid())
	{
		return System->OnlineServices->GetPrivilegesInterface();
	}

	return nullptr;
}


// Privilege

ELocalUserPrivilege UOnlinePrivilegeSubsystem::ConvertOnlineServicesPrivilege(EUserPrivileges Privilege) const
{
	switch (Privilege)
	{
	case EUserPrivileges::CanPlay:
		return ELocalUserPrivilege::CanPlay;
	case EUserPrivileges::CanPlayOnline:
		return ELocalUserPrivilege::CanPlayOnline;
	case EUserPrivileges::CanCommunicateViaTextOnline:
		return ELocalUserPrivilege::CanCommunicateViaTextOnline;
	case EUserPrivileges::CanCommunicateViaVoiceOnline:
		return ELocalUserPrivilege::CanCommunicateViaVoiceOnline;
	case EUserPrivileges::CanUseUserGeneratedContent:
		return ELocalUserPrivilege::CanUseUserGeneratedContent;
	case EUserPrivileges::CanCrossPlay:
		return ELocalUserPrivilege::CanUseCrossPlay;
	default:
		return ELocalUserPrivilege::Invalid;
	}
}

EUserPrivileges UOnlinePrivilegeSubsystem::ConvertOnlineServicesPrivilege(ELocalUserPrivilege Privilege) const
{
	switch (Privilege)
	{
	case ELocalUserPrivilege::CanPlay:
		return EUserPrivileges::CanPlay;
	case ELocalUserPrivilege::CanPlayOnline:
		return EUserPrivileges::CanPlayOnline;
	case ELocalUserPrivilege::CanCommunicateViaTextOnline:
		return EUserPrivileges::CanCommunicateViaTextOnline;
	case ELocalUserPrivilege::CanCommunicateViaVoiceOnline:
		return EUserPrivileges::CanCommunicateViaVoiceOnline;
	case ELocalUserPrivilege::CanUseUserGeneratedContent:
		return EUserPrivileges::CanUseUserGeneratedContent;
	case ELocalUserPrivilege::CanUseCrossPlay:
		return EUserPrivileges::CanCrossPlay;
	default:
		// No failure type, return CanPlay
		return EUserPrivileges::CanPlay;
	}
}

ELocalUserPrivilegeResult UOnlinePrivilegeSubsystem::ConvertOnlineServicesPrivilegeResult(EUserPrivileges Privilege, EPrivilegeResults Results) const
{
	if (Results == EPrivilegeResults::NoFailures)
	{
		return ELocalUserPrivilegeResult::Available;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::UserNotFound | EPrivilegeResults::UserNotLoggedIn))
	{
		return ELocalUserPrivilegeResult::UserNotLoggedIn;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::RequiredPatchAvailable | EPrivilegeResults::RequiredSystemUpdate))
	{
		return ELocalUserPrivilegeResult::VersionOutdated;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::AgeRestrictionFailure))
	{
		return ELocalUserPrivilegeResult::AgeRestricted;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::AccountTypeFailure))
	{
		return ELocalUserPrivilegeResult::AccountTypeRestricted;
	}
	if (EnumHasAnyFlags(Results, EPrivilegeResults::NetworkConnectionUnavailable))
	{
		return ELocalUserPrivilegeResult::NetworkConnectionUnavailable;
	}

	// Bucket other account failures together

	const auto AccountUseFailures{ EPrivilegeResults::OnlinePlayRestricted | EPrivilegeResults::UGCRestriction | EPrivilegeResults::ChatRestriction };

	if (EnumHasAnyFlags(Results, AccountUseFailures))
	{
		return ELocalUserPrivilegeResult::AccountUseRestricted;
	}

	// If you can't play at all, this is a license failure

	if (Privilege == EUserPrivileges::CanPlay)
	{
		return ELocalUserPrivilegeResult::LicenseInvalid;
	}

	// Unknown reason

	return ELocalUserPrivilegeResult::PlatformFailure;
}


FText UOnlinePrivilegeSubsystem::GetPrivilegeDescription(EOnlineServiceContext Context, ELocalUserPrivilege Privilege) const
{
	const auto* DevSetting{ GetDefault<UOnlineDeveloperSettings>() };

	if (ensure(DevSetting))
	{
		return DevSetting->GetPrivilegesDescription(ContextCaches.ResolveOnlineServiceContext(Context), Privilege);
	}

	return FText::GetEmpty();
}

FText UOnlinePrivilegeSubsystem::GetPrivilegeResultDescription(EOnlineServiceContext Context, ELocalUserPrivilegeResult Result) const
{
	const auto* DevSetting{ GetDefault<UOnlineDeveloperSettings>() };

	if (ensure(DevSetting))
	{
		return DevSetting->GetPrivilegesResultDescription(ContextCaches.ResolveOnlineServiceContext(Context), Result);
	}

	return FText::GetEmpty();
}


bool UOnlinePrivilegeSubsystem::BP_QueryUserPrivilege(ULocalUserAccountInfo* TargetUser, EOnlineServiceContext Context, ELocalUserPrivilege DesiredPrivilege)
{
	return QueryUserPrivilege(TargetUser, Context, DesiredPrivilege);
}

bool UOnlinePrivilegeSubsystem::QueryUserPrivilege(TWeakObjectPtr<ULocalUserAccountInfo> TargetUser, EOnlineServiceContext Context, ELocalUserPrivilege DesiredPrivilege, FLocalUserPrivilegeQueryDelegate Delegate)
{
	auto* UserAccountInfo{ TargetUser.Get() };
	auto PrivilegesInterface{ GetOnlinePrivileges(Context) };

	if (ensure(UserAccountInfo))
	{
		if (PrivilegesInterface.IsValid())
		{
			const auto DesiredPrivilege_OSS{ ConvertOnlineServicesPrivilege(DesiredPrivilege) };

			FQueryUserPrivilege::Params Params;
			Params.LocalAccountId = UserAccountInfo->GetNetId(Context).GetV2();
			Params.Privilege = DesiredPrivilege_OSS;

			auto QueryHandle{ PrivilegesInterface->QueryUserPrivilege(MoveTemp(Params)) };
			QueryHandle.OnComplete(this, &ThisClass::HandleQueryPrivilegeComplete, TargetUser, Context, DesiredPrivilege_OSS, Delegate);

			return true;
		}

		UserAccountInfo->UpdateCachedPrivilegeResult(DesiredPrivilege, ELocalUserPrivilegeResult::Available, Context);
	}

	Delegate.ExecuteIfBound(UserAccountInfo, Context, DesiredPrivilege, ELocalUserPrivilegeResult::Available, TOptional<FOnlineErrorType>());

	return false;
}

void UOnlinePrivilegeSubsystem::HandleQueryPrivilegeComplete(const TOnlineResult<FQueryUserPrivilege>& Result, TWeakObjectPtr<ULocalUserAccountInfo> UserInfo, EOnlineServiceContext Context, EUserPrivileges DesiredPrivilege, FLocalUserPrivilegeQueryDelegate Delegate)
{
	auto LocalUserPrivilege{ ConvertOnlineServicesPrivilege(DesiredPrivilege) };
	auto LocalUserPrivilegeResult{ ELocalUserPrivilegeResult::PlatformFailure };

	if (const auto* OkResult{ Result.TryGetOkValue() })
	{
		LocalUserPrivilegeResult = ConvertOnlineServicesPrivilegeResult(DesiredPrivilege, OkResult->PrivilegeResult);
	}
	else
	{
		UE_LOG(LogGameCore_OnlinePrivileges, Warning, TEXT("QueryUserPrivilege failed: %s"), *Result.GetErrorValue().GetLogString());
	}

	// Update the user cached value

	auto* LocalUserAccountInfo{ UserInfo.Get()};
	if (LocalUserAccountInfo)
	{
		LocalUserAccountInfo->UpdateCachedPrivilegeResult(LocalUserPrivilege, LocalUserPrivilegeResult, Context);
	}

	// Execute Delegate

	Delegate.ExecuteIfBound(LocalUserAccountInfo, Context, LocalUserPrivilege, LocalUserPrivilegeResult, Result.IsError() ? Result.GetErrorValue() : Errors::Unknown());
}
