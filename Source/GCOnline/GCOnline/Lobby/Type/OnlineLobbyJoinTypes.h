// Copyright (C) 2024 owoDra

#pragma once

#include "Type/OnlineServiceResultTypes.h"

//#include "OnlineLobbyJoinTypes.generated.h"

using namespace UE::Online;

class ULobbySearchResult;


////////////////////////////////////////////////////////////////////////
// Delegates


/**
 * Event triggered when the local user has requested to join a lobby from an external source, for example from a platform overlay.
 * Generally, the game should transition the player into the lobby.
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FUserJoinLobbyRequestDelegate
										, const FPlatformUserId&			/*LocalPlatformUserId*/
										, ULobbySearchResult*				/*RequestedSession*/
										, FOnlineServiceResult				/*RequestedSessionResult*/);

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUserJoinLobbyRequestDynamicDelegate
//										, const FPlatformUserId&			, LocalPlatformUserId
//										, ULobbySearchResult*				, RequestedSession
//										, FOnlineServiceResult				, RequestedSessionResult);


/**
 * Event triggered when a lobby join has completed, after joining the underlying lobby and before traveling to the server if it was successful.
 * The event parameters indicate if this was successful, or if there was an error that will stop it from traveling.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FJoinLobbyCompleteDelegate, const FOnlineServiceResult& /*Result*/);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJoinLobbyCompleteDynamicDelegate, const FOnlineServiceResult&, Result);


/**
 * Event triggered when a lobby join has completed, after resolving the connect string and prior to the client traveling.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FLobbyPreClientTravelDelegate, FString& /*URL*/);
