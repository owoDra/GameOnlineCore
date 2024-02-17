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

	auto Lobby{ ensure(LobbyToJoin) ? LobbyToJoin->GetLobby() : nullptr };
	if (ensure(Lobby))
	{
		Params.LocalName = LocalName;
		Params.LobbyId = Lobby->LobbyId;
		Params.bPresenceEnabled = bPresenceEnabled;
	}

	return Params;
}
