// Copyright (C) 2024 owoDra

#include "OnlineLobbySearchTypes.h"

#include "OnlineDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineLobbySearchTypes)


/////////////////////////////////////////////////////////////////
// ULobbySearchResult

ULobbySearchResult::ULobbySearchResult(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


// Lobby Attribute

bool ULobbySearchResult::GetLobbyAttributeAsString(FName Key, FString& OutValue) const
{
	if (const auto* VariantValue{ Lobby->Attributes.Find(ResolveAttributeKey(Key)) })
	{
		OutValue = VariantValue->GetString();
		return true;
	}
	
	return false;
}

bool ULobbySearchResult::GetLobbyAttributeAsInteger(FName Key, int32& OutValue) const
{
	if (const auto* VariantValue{ Lobby->Attributes.Find(ResolveAttributeKey(Key)) })
	{
		OutValue = VariantValue->GetInt64();
		return true;
	}

	return false;
}

bool ULobbySearchResult::GetLobbyAttributeAsDouble(FName Key, double& OutValue) const
{
	if (const auto* VariantValue{ Lobby->Attributes.Find(ResolveAttributeKey(Key)) })
	{
		OutValue = VariantValue->GetDouble();
		return true;
	}

	return false;
}

bool ULobbySearchResult::GetLobbyAttributeAsBoolean(FName Key, bool& OutValue) const
{
	if (const auto* VariantValue{ Lobby->Attributes.Find(ResolveAttributeKey(Key)) })
	{
		OutValue = VariantValue->GetBoolean();
		return true;
	}

	return false;
}

FName ULobbySearchResult::ResolveAttributeKey(const FName& Key) const
{
	const auto* DevSetting{ GetDefault<UOnlineDeveloperSettings>() };

	return DevSetting ? DevSetting->RedirectLobbyAttribute_ToOnlineService(Key) : Key;
}


// Lobby Status

int32 ULobbySearchResult::GetMaxMembers() const
{
	return Lobby->MaxMembers;
}

int32 ULobbySearchResult::GetNumMembers() const
{
	return Lobby->Members.Num();
}

int32 ULobbySearchResult::GetNumOpenSlot() const
{
	return GetMaxMembers() - GetNumMembers();
}


// Utilities

FString ULobbySearchResult::GetDebugString() const
{
	return ToLogString(Lobby->LobbyId);
}


/////////////////////////////////////////////////////////////////
// ULobbySearchRequest

ULobbySearchRequest::ULobbySearchRequest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


FFindLobbies::Params ULobbySearchRequest::GenerateFindParameters() const
{
	const auto* DevSettings{ GetDefault<UOnlineDeveloperSettings>() };

	FFindLobbies::Params Prams;
	Prams.MaxResults = MaxResult;

	for (const auto& Filter : Filters)
	{
		auto FilterParam{ Filter.ToSearchFilter() };

		FilterParam.AttributeName = DevSettings->RedirectLobbyAttribute_ToOnlineService(FilterParam.AttributeName);

		Prams.Filters.Emplace(FilterParam);
	}

	return Prams;
}

void ULobbySearchRequest::NotifySearchFinished(const FOnlineServiceResult& Result)
{
	OnSearchFinished.Broadcast(Result);
	K2_OnSearchFinished.Broadcast(Result);
}
