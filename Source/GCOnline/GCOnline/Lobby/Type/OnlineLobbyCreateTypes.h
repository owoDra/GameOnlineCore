// Copyright (C) 2024 owoDra

#pragma once

#include "Type/OnlineServiceResultTypes.h"
#include "Type/OnlineLobbyAttributeTypes.h"

#include "Online/Lobbies.h"

#include "OnlineLobbyCreateTypes.generated.h"

using namespace UE::Online;

class ULobbyResult;


////////////////////////////////////////////////////////////////////////
// Enums

/** 
 * Online connection mode for newly created lobbies
 */
UENUM(BlueprintType)
enum class ELobbyOnlineMode : uint8
{
	LAN,
	Online
};


/**
 * Lobby joinable policy to create a new lobby
 * 
 * Tips:
 *	Same as ELobbyJoinPolicy but has blueprint accesibility
 */
UENUM(BlueprintType)
enum class ELobbyJoinablePolicy : uint8
{
	//
	// Lobby can be found through searches based on attribute matching,
	// by knowing the lobby id, or by invitation.
	//
	PublicAdvertised,

	//
	// Lobby may be joined by knowing the lobby id or by invitation.
	//
	PublicNotAdvertised,

	//
	// Lobby may only be joined by invitation.
	//
	InvitationOnly,
};


////////////////////////////////////////////////////////////////////////
// Delegates

/**
 * Delegates to notifies lobby creation for hosting has completed,
 */
DECLARE_DELEGATE_TwoParams(FLobbyCreateCompleteDelegate, ULobbyCreateRequest* /*Request*/, FOnlineServiceResult /*Result*/);


////////////////////////////////////////////////////////////////////////
// Objects

/** 
 * A request object that stores the parameters used when hosting a gameplay lobby 
 */
UCLASS(BlueprintType)
class GCONLINE_API ULobbyCreateRequest : public UObject
{
	GENERATED_BODY()
public:
	ULobbyCreateRequest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//////////////////////////////////////////////////////
	// Create Parameters
public:
	//
	// Online connection mode for newly created lobbies
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	ELobbyOnlineMode OnlineMode{ ELobbyOnlineMode::Online };

	//
	// Lobby joinable policy to create a new lobby
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	ELobbyJoinablePolicy JoinablePolicy{ ELobbyJoinablePolicy::PublicAdvertised };

	//
	// Local name to manage lobbies used when creating lobbies
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	FName LocalName;

	//
	// Schema ID for the lobby to be created for this project
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	FString SchemaId;

	//
	// Whether this lobby should be set as the user's new presence lobby.
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	bool bPresenceEnabled{ true };

	//
	// String used during matchmaking to specify what type of game mode this is
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	FString ModeNameForAdvertisement;

	//
	// The map that will be loaded at the start of gameplay, this needs to be a valid Primary Asset top-level map
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby", meta = (AllowedTypes = "World"))
	FPrimaryAssetId MapID;

	//
	// Initial values of attributes for newly created lobbies
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	TSet<FLobbyAttribute> InitialAttributes;

	//
	// Initial values of user attributes for newly created lobbies
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	TSet<FLobbyAttribute> InitialUserAttributes;

	//
	// Extra arguments passed as URL options to the game
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby", meta = (ForceInlineRow))
	TMap<FString, FString> ExtraArgs;

	//
	// Maximum players allowed per gameplay lobby
	//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	int32 MaxPlayerCount{ 2 };

public:
	/**
	 * Returns the join policy for newly create lobby, could be overridden in child classes
	 */
	virtual ELobbyJoinPolicy GetJoinPolicy() const;

	/** 
	 * Returns the maximum players that should actually be used, could be overridden in child classes 
	 */
	virtual int32 GetMaxPlayers() const;

	/** 
	 * Returns the full map name that will be used during gameplay 
	 */
	virtual FString GetMapName() const;

	/** 
	 * Constructs the full URL that will be passed to ServerTravel 
	 */
	virtual FString ConstructTravelURL() const;

	/** 
	 * Returns true if this request is valid, returns false and logs errors if it is not 
	 */
	virtual bool ValidateAndLogErrors(FString& OutError) const;

	/**
	 * Generate parameters for lobby creation from current settings
	 */
	FCreateLobby::Params GenerateCreationParameters() const;


	//////////////////////////////////////////////////////
	// Create Result
public:
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lobby")
	TObjectPtr<ULobbyResult> Result{ nullptr };

};
