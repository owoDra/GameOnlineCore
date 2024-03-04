// Copyright (C) 2024 owoDra

#include "OnlineLobbyJoinTypes.h"

#include "Type/OnlineLobbyResultTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineLobbyJoinTypes)


ULobbyJoinRequest::ULobbyJoinRequest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


FJoinLobby::Params ULobbyJoinRequest::GenerateJoinParameters() const
{
	FJoinLobby::Params Params;

	if (ensure(LobbyToJoin))
	{
		Params.LocalName = LocalName;
		Params.LobbyId = LobbyToJoin->GetLobbyId();
		Params.bPresenceEnabled = bPresenceEnabled;
	}

	return Params;
}
