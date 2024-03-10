// Copyright (C) 2024 owoDra

#pragma once

#include "Type/OnlineServiceResultTypes.h"

#include "OnlineLobbyJoinTypes.generated.h"

using namespace UE::Online;

class ULobbyJoinRequest;

////////////////////////////////////////////////////////////////////////
// Delegates

/**
 * Event triggered when the local user has requested to join a lobby from an external source, for example from a platform overlay.
 * Generally, the game should transition the player into the lobby.
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FUserJoinLobbyRequestDelegate
										, const FPlatformUserId&			/*LocalPlatformUserId*/
										, ULobbyResult*						/*RequestedLobby*/
										, FOnlineServiceResult				/*Result*/);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUserJoinLobbyRequestDynamicDelegate
										, const FPlatformUserId&			, LocalPlatformUserId
										, ULobbyResult*						, RequestedLobby
										, FOnlineServiceResult				, Result);


/**
 * Delegate to notifies lobby join has completed
 */
DECLARE_DELEGATE_TwoParams(FLobbyJoinCompleteDelegate, ULobbyJoinRequest* /*Lobby*/, FOnlineServiceResult /*Result*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FLobbyJoinCompleteMulticastDelegate, ULobbyJoinRequest*/*Lobby*/, FOnlineServiceResult/*Result*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLobbyJoinCompleteDynamicDelegate, ULobbyJoinRequest*, Lobby, FOnlineServiceResult, Result);


/**
 * Event triggered when a lobby join has completed, after resolving the connect string and prior to the client traveling.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FLobbyPreClientTravelDelegate, FString& /*URL*/);


////////////////////////////////////////////////////////////////////////
// Objects

/**
 * Request object describing a lobby join, this object will be updated once the search has completed
 */
UCLASS(BlueprintType)
class GCONLINE_API ULobbyJoinRequest : public UObject
{
	GENERATED_BODY()
public:
	ULobbyJoinRequest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	///////////////////////////////////////////////
	// Search Parameters
public:
	//
	// Local name to identify the joining lobby
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	FName LocalName{ NAME_GameSession };

	//
	// Whether this lobby should be set as the user's new presence lobby
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	bool bPresenceEnabled{ true };

	//
	// Lobby results data for joining lobbies
	// 
	// Tips:
	//	Basically, use the LobbyResult obtained by SearchLobby
	// 
	//	When the join is completed, the data of the joined Lobby is overwritten in this LobbyResult.
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	TObjectPtr<ULobbyResult> LobbyToJoin;

public:
	/**
	 * Generate parameters for lobby join from current settings
	 */
	FJoinLobby::Params GenerateJoinParameters() const;

};
