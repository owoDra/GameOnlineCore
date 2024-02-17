// Copyright (C) 2024 owoDra

#include "OnlineLobbyResultTypes.h"

#include "OnlineDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineLobbyResultTypes)


/////////////////////////////////////////////////////////////////
// ULobbySearchResult

ULobbyResult::ULobbyResult(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


// Lobby Info

FName ULobbyResult::GetLocalName() const
{
	return ensure(Lobby) ? Lobby->LocalName : NAME_None;
}

FAccountId ULobbyResult::GetOwnerAccountId() const
{
	return ensure(Lobby) ? Lobby->OwnerAccountId : FAccountId();
}


// Lobby Attribute

bool ULobbyResult::GetLobbyAttributeAsString(FName Key, FString& OutValue) const
{
	if (!ensure(Lobby))
	{
		return false;
	}

	if (const auto* VariantValue{ Lobby->Attributes.Find(ResolveAttributeKey(Key)) })
	{
		OutValue = VariantValue->GetString();
		return true;
	}
	
	return false;
}

bool ULobbyResult::GetLobbyAttributeAsInteger(FName Key, int32& OutValue) const
{
	if (!ensure(Lobby))
	{
		return false;
	}

	if (const auto* VariantValue{ Lobby->Attributes.Find(ResolveAttributeKey(Key)) })
	{
		OutValue = VariantValue->GetInt64();
		return true;
	}

	return false;
}

bool ULobbyResult::GetLobbyAttributeAsDouble(FName Key, double& OutValue) const
{
	if (!ensure(Lobby))
	{
		return false;
	}

	if (const auto* VariantValue{ Lobby->Attributes.Find(ResolveAttributeKey(Key)) })
	{
		OutValue = VariantValue->GetDouble();
		return true;
	}

	return false;
}

bool ULobbyResult::GetLobbyAttributeAsBoolean(FName Key, bool& OutValue) const
{
	if (!ensure(Lobby))
	{
		return false;
	}

	if (const auto* VariantValue{ Lobby->Attributes.Find(ResolveAttributeKey(Key)) })
	{
		OutValue = VariantValue->GetBoolean();
		return true;
	}

	return false;
}

FName ULobbyResult::ResolveAttributeKey(const FName& Key) const
{
	const auto* DevSetting{ GetDefault<UOnlineDeveloperSettings>() };

	return DevSetting ? DevSetting->RedirectLobbyAttribute_ToOnlineService(Key) : Key;
}


// Lobby Status

int32 ULobbyResult::GetMaxMembers() const
{
	return ensure(Lobby) ? Lobby->MaxMembers : 0;
}

int32 ULobbyResult::GetNumMembers() const
{
	return ensure(Lobby) ? Lobby->Members.Num() : 0;
}

int32 ULobbyResult::GetNumOpenSlot() const
{
	return GetMaxMembers() - GetNumMembers();
}


// Utilities

FString ULobbyResult::GetDebugString() const
{
	return ensure(Lobby) ? ToLogString(Lobby->LobbyId) : TEXT("INVALID LOBBY");
}
