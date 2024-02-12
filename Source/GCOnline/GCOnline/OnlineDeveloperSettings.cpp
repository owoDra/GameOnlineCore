// Copyright (C) 2024 owoDra

#include "OnlineDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineDeveloperSettings)


/////////////////////////////////////////////////////////////////////
// FPrivilegesDescriptionSetting

FPrivilegesDescriptionSetting::FPrivilegesDescriptionSetting()
{
	PrivilegeDescriptions =
	{
		{ ELocalUserPrivilege::CanPlay						, NSLOCTEXT("GameOnlineCore", "PrivilegeCanPlay"						, "play the game") },
		{ ELocalUserPrivilege::CanPlayOnline				, NSLOCTEXT("GameOnlineCore", "PrivilegeCanPlayOnline"					, "play online") },
		{ ELocalUserPrivilege::CanCommunicateViaTextOnline	, NSLOCTEXT("GameOnlineCore", "PrivilegeCanCommunicateViaTextOnline"	, "communicate with text") },
		{ ELocalUserPrivilege::CanCommunicateViaVoiceOnline	, NSLOCTEXT("GameOnlineCore", "PrivilegeCanCommunicateViaVoiceOnline"	, "communicate with voice") },
		{ ELocalUserPrivilege::CanUseUserGeneratedContent	, NSLOCTEXT("GameOnlineCore", "PrivilegeCanUseUserGeneratedContent"		, "access user content") },
		{ ELocalUserPrivilege::CanUseCrossPlay				, NSLOCTEXT("GameOnlineCore", "PrivilegeCanUseCrossPlay"				, "play with other platforms") }
	};

	PrivilegeResultDescriptions =
	{
		{ ELocalUserPrivilegeResult::Unknown						, NSLOCTEXT("GameOnlineCore", "ResultUnknown"						, "Unknown if the user is allowed") },
		{ ELocalUserPrivilegeResult::Available						, NSLOCTEXT("GameOnlineCore", "ResultAvailable"						, "The user is allowed") },
		{ ELocalUserPrivilegeResult::UserNotLoggedIn				, NSLOCTEXT("GameOnlineCore", "ResultUserNotLoggedIn"				, "The user must login") },
		{ ELocalUserPrivilegeResult::LicenseInvalid					, NSLOCTEXT("GameOnlineCore", "ResultLicenseInvalid"				, "A valid game license is required") },
		{ ELocalUserPrivilegeResult::VersionOutdated				, NSLOCTEXT("GameOnlineCore", "VersionOutdated"						, "The game or hardware needs to be updated") },
		{ ELocalUserPrivilegeResult::NetworkConnectionUnavailable	, NSLOCTEXT("GameOnlineCore", "ResultNetworkConnectionUnavailable"	, "A network connection is required") },
		{ ELocalUserPrivilegeResult::AgeRestricted					, NSLOCTEXT("GameOnlineCore", "ResultAgeRestricted"					, "This age restricted account is not allowed") },
		{ ELocalUserPrivilegeResult::AccountTypeRestricted			, NSLOCTEXT("GameOnlineCore", "ResultAccountTypeRestricted"			, "This account type does not have access") },
		{ ELocalUserPrivilegeResult::AccountUseRestricted			, NSLOCTEXT("GameOnlineCore", "ResultAccountUseRestricted"			, "This account is not allowed") },
		{ ELocalUserPrivilegeResult::PlatformFailure				, NSLOCTEXT("GameOnlineCore", "ResultPlatformFailure"				, "Not allowed") }
	};
}


/////////////////////////////////////////////////////////////////////
// UOnlineDeveloperSettings

UOnlineDeveloperSettings::UOnlineDeveloperSettings()
{
	CategoryName = TEXT("Game XXX Core");
	SectionName = TEXT("Game Online Core");

	PrivilegesDescriptions =
	{
		{ EOnlineServiceContext::Default, FPrivilegesDescriptionSetting() },
		{ EOnlineServiceContext::Platform, FPrivilegesDescriptionSetting() },
	};
}


// Privileges

FText UOnlineDeveloperSettings::GetPrivilegesDescription(EOnlineServiceContext Context, ELocalUserPrivilege Privilege) const
{
	if (auto* FoundRow{ PrivilegesDescriptions.Find(Context) })
	{
		if (auto* FoundText{ FoundRow->PrivilegeDescriptions.Find(Privilege) })
		{
			return *FoundText;
		}
	}

	return FText::GetEmpty();
}

FText UOnlineDeveloperSettings::GetPrivilegesResultDescription(EOnlineServiceContext Context, ELocalUserPrivilegeResult Result) const
{
	if (auto* FoundRow{ PrivilegesDescriptions.Find(Context) })
	{
		if (auto* FoundText{ FoundRow->PrivilegeResultDescriptions.Find(Result) })
		{
			return *FoundText;
		}
	}

	return FText::GetEmpty();
}
