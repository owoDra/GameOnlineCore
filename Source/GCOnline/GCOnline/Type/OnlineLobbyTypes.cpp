// Copyright (C) 2024 owoDra

#include "OnlineLobbyTypes.h"

#include "Online/OnlineSessionNames.h"
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineLobbyTypes)


/////////////////////////////////////////////////////////////////
// FLobbyFilteringInfo

void FLobbyFilteringInfo::FillFilteringParamForSearch(TArray<FFindLobbySearchFilter>& Param)
{
	// Check and set "DEDICATEDONLY" options

	if (DedicatedServerOnly != ELobbySearchConditionalOption::NotSpecified)
	{
		Param.Emplace(
			FFindLobbySearchFilter{ SEARCH_DEDICATED_ONLY, ESchemaAttributeComparisonOp::Equals, (DedicatedServerOnly == ELobbySearchConditionalOption::Enabled) });
	}

	// Check and set "EMPTYONLY" options

	if (EmptyOnly != ELobbySearchConditionalOption::NotSpecified)
	{
		Param.Emplace(
			FFindLobbySearchFilter{ SEARCH_EMPTY_SERVERS_ONLY, ESchemaAttributeComparisonOp::Equals, (EmptyOnly == ELobbySearchConditionalOption::Enabled) });
	}

	// Check and set "NONEMPTYONLY" options

	if (NoneEmptyOnly != ELobbySearchConditionalOption::NotSpecified)
	{
		Param.Emplace(
			FFindLobbySearchFilter{ SEARCH_NONEMPTY_SERVERS_ONLY, ESchemaAttributeComparisonOp::Equals, (NoneEmptyOnly == ELobbySearchConditionalOption::Enabled) });
	}

	// Check and set "SECUREONLY" options

	if (SecureOnly != ELobbySearchConditionalOption::NotSpecified)
	{
		Param.Emplace(
			FFindLobbySearchFilter{ SEARCH_SECURE_SERVERS_ONLY, ESchemaAttributeComparisonOp::Equals, (SecureOnly == ELobbySearchConditionalOption::Enabled) });
	}

	// Check and set "PRESENCESEARCH" options

	if (PresenceSearch != ELobbySearchConditionalOption::NotSpecified)
	{
		/*Param.Emplace(
			FFindLobbySearchFilter{ SEARCH_PRESENCE, ESchemaAttributeComparisonOp::Equals, (PresenceSearch == ELobbySearchConditionalOption::Enabled) });*/
		Param.Emplace(
			FFindLobbySearchFilter{ FName(TEXT("LOBBYSERVICEATTRIBUTE5")), ESchemaAttributeComparisonOp::Equals, (PresenceSearch == ELobbySearchConditionalOption::Enabled)});
	}

	// Check and set "MINSLOTSAVAILABLE" options

	if (MinSlotsAvailable != INDEX_NONE)
	{
		Param.Emplace(
			FFindLobbySearchFilter{ SEARCH_MINSLOTSAVAILABLE, ESchemaAttributeComparisonOp::GreaterThanEquals, static_cast<int64>(MinSlotsAvailable) });
	}

	// Check and set "EXCLUDEUNIQUEIDS" options

	if (!ExcludeUniqueIds.IsEmpty())
	{
		FString Ids;
		for (const auto& Id : ExcludeUniqueIds)
		{
			Ids += Id.ToString();
			Ids += TEXT(";");
		}

		Param.Emplace(
			FFindLobbySearchFilter{ SEARCH_EXCLUDE_UNIQUEIDS, ESchemaAttributeComparisonOp::NotIn , Ids });
	}

	// Check and set "SEARCHUSER" options

	if (!SearchUser.IsEmpty())
	{
		Param.Emplace(
			FFindLobbySearchFilter{ SEARCH_USER, ESchemaAttributeComparisonOp::In , SearchUser });
	}

	// Check and set "SEARCHKEYWORDS" options

	if (!SearchKeywords.IsEmpty())
	{
		Param.Emplace(
			FFindLobbySearchFilter{ SEARCH_KEYWORDS, ESchemaAttributeComparisonOp::In , SearchKeywords });
	}

	// Check and set "MATCHMAKINGQUEUE" options

	if (!MatchmakingQueue.IsEmpty())
	{
		Param.Emplace(
			FFindLobbySearchFilter{ SEARCH_MATCHMAKING_QUEUE, ESchemaAttributeComparisonOp::In , MatchmakingQueue });
	}
}


/////////////////////////////////////////////////////////////////
// UHostLobbyRequest

UHostLobbyRequest::UHostLobbyRequest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


int32 UHostLobbyRequest::GetMaxPlayers() const
{
	return MaxPlayerCount;
}

FString UHostLobbyRequest::GetMapName() const
{
	FAssetData MapAssetData;
	if (UAssetManager::Get().GetPrimaryAssetData(MapID, /*out*/ MapAssetData))
	{
		return MapAssetData.PackageName.ToString();
	}
	else
	{
		return FString();
	}
}

FString UHostLobbyRequest::ConstructTravelURL() const
{
	FString CombinedExtraArgs;

	if (OnlineMode == ELobbyOnlineMode::LAN)
	{
		CombinedExtraArgs += TEXT("?bIsLanMatch");
	}

	CombinedExtraArgs += TEXT("?listen");

	for (const auto& KVP : ExtraArgs)
	{
		if (!KVP.Key.IsEmpty())
		{
			if (KVP.Value.IsEmpty())
			{
				CombinedExtraArgs += FString::Printf(TEXT("?%s"), *KVP.Key);
			}
			else
			{
				CombinedExtraArgs += FString::Printf(TEXT("?%s=%s"), *KVP.Key, *KVP.Value);
			}
		}
	}

	//bIsRecordingDemo ? TEXT("?DemoRec") : TEXT(""));

	return FString::Printf(TEXT("%s%s"), *GetMapName(), *CombinedExtraArgs);
}

bool UHostLobbyRequest::ValidateAndLogErrors(FText& OutError) const
{
#if WITH_SERVER_CODE
	if (GetMapName().IsEmpty())
	{
		OutError = FText::Format(NSLOCTEXT("GameOnlineCore", "InvalidMapFormat", "Can't find asset data for MapID {0}, hosting request failed."), FText::FromString(MapID.ToString()));
		return false;
	}

	return true;
#else
	// Client builds are only meant to connect to dedicated servers, they are missing the code to host a session by default
	// You can change this behavior in subclasses to handle something like a tutorial

	OutError = NSLOCTEXT("GameOnlineCore", "ClientBuildCannotHost", "Client builds cannot host game sessions.");
	return false;
#endif
}


/////////////////////////////////////////////////////////////////
// USearchLobbyResult

USearchLobbyResult::USearchLobbyResult(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


FString USearchLobbyResult::GetDescription() const
{
	return ToLogString(Lobby->LobbyId);
}

void USearchLobbyResult::GetStringSetting(FName Key, FString& Value, bool& bFoundValue) const
{
	if (const auto* VariantValue{ Lobby->Attributes.Find(Key) })
	{
		bFoundValue = true;
		Value = VariantValue->GetString();
	}
	else
	{
		bFoundValue = false;
	}
}

void USearchLobbyResult::GetIntSetting(FName Key, int32& Value, bool& bFoundValue) const
{
	if (const auto* VariantValue{ Lobby->Attributes.Find(Key) })
	{
		bFoundValue = true;
		Value = static_cast<int32>(VariantValue->GetInt64());
	}
	else
	{
		bFoundValue = false;
	}
}

int32 USearchLobbyResult::GetNumOpenPrivateConnections() const
{
	/// @TODO:  Private connections
	return 0;
}

int32 USearchLobbyResult::GetNumOpenPublicConnections() const
{
	return Lobby->MaxMembers - Lobby->Members.Num();
}

int32 USearchLobbyResult::GetMaxPublicConnections() const
{
	return Lobby->MaxMembers;
}

int32 USearchLobbyResult::GetPingInMs() const
{
	/// @TODO:  Not a property of lobbies.  Need to implement with sessions.
	return 0;
}


/////////////////////////////////////////////////////////////////
// USearchLobbyRequest

USearchLobbyRequest::USearchLobbyRequest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void USearchLobbyRequest::NotifySearchFinished(bool bSucceeded, const FText& ErrorMessage)
{
	OnSearchFinished.Broadcast(bSucceeded, ErrorMessage);
	K2_OnSearchFinished.Broadcast(bSucceeded, ErrorMessage);
}


/////////////////////////////////////////////////////////////////
// FLobbySearchSettings

FLobbySearchSettings::FLobbySearchSettings(USearchLobbyRequest* InSearchRequest)
{
	check(InSearchRequest);

	SearchRequest = InSearchRequest;

	FindLobbyParams.MaxResults = SearchRequest->MaxResult;

	/// @TODO attribute support

	//InSearchRequest->FilteringInfo.FillFilteringParamForSearch(FindLobbyParams.Filters);
}
