// Copyright (C) 2024 owoDra

#include "OnlineDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineDeveloperSettings)


/////////////////////////////////////////////////////////////////////
// FPrivilegesDescriptionSetting

FPrivilegesDescriptionSetting::FPrivilegesDescriptionSetting()
{
	PrivilegeDescriptions =
	{
		{ EOnlinePrivilege::CanPlay							, NSLOCTEXT("GameOnlineCore", "PrivilegeCanPlay"						, "play the game") },
		{ EOnlinePrivilege::CanPlayOnline					, NSLOCTEXT("GameOnlineCore", "PrivilegeCanPlayOnline"					, "play online") },
		{ EOnlinePrivilege::CanCommunicateViaTextOnline		, NSLOCTEXT("GameOnlineCore", "PrivilegeCanCommunicateViaTextOnline"	, "communicate with text") },
		{ EOnlinePrivilege::CanCommunicateViaVoiceOnline	, NSLOCTEXT("GameOnlineCore", "PrivilegeCanCommunicateViaVoiceOnline"	, "communicate with voice") },
		{ EOnlinePrivilege::CanUseUserGeneratedContent		, NSLOCTEXT("GameOnlineCore", "PrivilegeCanUseUserGeneratedContent"		, "access user content") },
		{ EOnlinePrivilege::CanUseCrossPlay					, NSLOCTEXT("GameOnlineCore", "PrivilegeCanUseCrossPlay"				, "play with other platforms") }
	};

	PrivilegeResultDescriptions =
	{
		{ EOnlinePrivilegeResult::Unknown						, NSLOCTEXT("GameOnlineCore", "ResultUnknown"						, "Unknown if the user is allowed") },
		{ EOnlinePrivilegeResult::Available						, NSLOCTEXT("GameOnlineCore", "ResultAvailable"						, "The user is allowed") },
		{ EOnlinePrivilegeResult::UserNotLoggedIn				, NSLOCTEXT("GameOnlineCore", "ResultUserNotLoggedIn"				, "The user must login") },
		{ EOnlinePrivilegeResult::LicenseInvalid				, NSLOCTEXT("GameOnlineCore", "ResultLicenseInvalid"				, "A valid game license is required") },
		{ EOnlinePrivilegeResult::VersionOutdated				, NSLOCTEXT("GameOnlineCore", "VersionOutdated"						, "The game or hardware needs to be updated") },
		{ EOnlinePrivilegeResult::NetworkConnectionUnavailable	, NSLOCTEXT("GameOnlineCore", "ResultNetworkConnectionUnavailable"	, "A network connection is required") },
		{ EOnlinePrivilegeResult::AgeRestricted					, NSLOCTEXT("GameOnlineCore", "ResultAgeRestricted"					, "This age restricted account is not allowed") },
		{ EOnlinePrivilegeResult::AccountTypeRestricted			, NSLOCTEXT("GameOnlineCore", "ResultAccountTypeRestricted"			, "This account type does not have access") },
		{ EOnlinePrivilegeResult::AccountUseRestricted			, NSLOCTEXT("GameOnlineCore", "ResultAccountUseRestricted"			, "This account is not allowed") },
		{ EOnlinePrivilegeResult::PlatformFailure				, NSLOCTEXT("GameOnlineCore", "ResultPlatformFailure"				, "Not allowed") }
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

FText UOnlineDeveloperSettings::GetPrivilegesDescription(EOnlineServiceContext Context, EOnlinePrivilege Privilege) const
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

FText UOnlineDeveloperSettings::GetPrivilegesResultDescription(EOnlineServiceContext Context, EOnlinePrivilegeResult Result) const
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
