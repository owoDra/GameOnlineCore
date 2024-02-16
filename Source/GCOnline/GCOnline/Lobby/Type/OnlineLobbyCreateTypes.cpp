// Copyright (C) 2024 owoDra

#include "OnlineLobbyCreateTypes.h"

#include "OnlineDeveloperSettings.h"

#include "Online/OnlineSessionNames.h"
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineLobbyCreateTypes)


/////////////////////////////////////////////////////////////////
// ULobbyCreateRequest

ULobbyCreateRequest::ULobbyCreateRequest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LocalName = NAME_GameSession;
	SchemaId = TEXT("GameLobby");
}


ELobbyJoinPolicy ULobbyCreateRequest::GetJoinPolicy() const
{
	return static_cast<ELobbyJoinPolicy>(JoinablePolicy);
}

int32 ULobbyCreateRequest::GetMaxPlayers() const
{
	return MaxPlayerCount;
}

FString ULobbyCreateRequest::GetMapName() const
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

FString ULobbyCreateRequest::ConstructTravelURL() const
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

	return FString::Printf(TEXT("%s%s"), *GetMapName(), *CombinedExtraArgs);
}

bool ULobbyCreateRequest::ValidateAndLogErrors(FString& OutError) const
{
#if WITH_SERVER_CODE
	if (GetMapName().IsEmpty())
	{
		OutError = FString::Printf(TEXT("Can't find asset data for MapID(%s), hosting request failed."), *MapID.ToString());
		return false;
	}

	return true;
#else
	// Client builds are only meant to connect to dedicated servers, they are missing the code to host a session by default
	// You can change this behavior in subclasses to handle something like a tutorial

	OutError = TEXT("Client builds cannot host game sessions.");
	return false;
#endif
}

FCreateLobby::Params ULobbyCreateRequest::GenerateCreationParameters() const
{
	const auto* DevSettings{ GetDefault<UOnlineDeveloperSettings>() };

	FCreateLobby::Params Prams;
	Prams.LocalName = LocalName;
	Prams.SchemaId = FSchemaId(SchemaId);
	Prams.JoinPolicy = GetJoinPolicy();
	Prams.MaxMembers = GetMaxPlayers();
	Prams.bPresenceEnabled = bPresenceEnabled;

	// Add "ModeNameForAdvertisement" as attribute "SETTING_GAMEMODE"

	Prams.Attributes.Emplace(DevSettings->RedirectLobbyAttribute_ToOnlineService(SETTING_GAMEMODE), ModeNameForAdvertisement);

	// Add "MapID" as attribute "SETTING_MAPNAME"

	Prams.Attributes.Emplace(DevSettings->RedirectLobbyAttribute_ToOnlineService(SETTING_MAPNAME), GetMapName());

	// Add extra lobby attributes

	for (const auto& Attr : InitialAttributes)
	{
		Prams.Attributes.Emplace(DevSettings->RedirectLobbyAttribute_ToOnlineService(Attr.GetAttributeName()), Attr.ToSchemaVariant());
	}

	// Add lobby user attributes

	for (const auto& UserAttr : InitialUserAttributes)
	{
		Prams.Attributes.Emplace(DevSettings->RedirectUserLobbyAttribute_ToOnlineService(UserAttr.GetAttributeName()), UserAttr.ToSchemaVariant());
	}

	return Prams;
}
