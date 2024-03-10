// Copyright (C) 2024 owoDra

#pragma once

#include "Type/OnlineServiceResultTypes.h"
#include "Type/OnlineLobbyAttributeTypes.h"

#include "Online/Lobbies.h"

#include "OnlineLobbyResultTypes.generated.h"

using namespace UE::Online;

class UOnlineLobbySubsystem;


/** 
 * A result object returned from the online system that describes a joinable/joined game lobby 
 */
UCLASS(BlueprintType)
class GCONLINE_API ULobbyResult : public UObject
{
	GENERATED_BODY()
public:
	ULobbyResult(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	///////////////////////////////////////////////////
	// Initialization
protected:
	//
	// Pointer to the platform-specific implementation
	//
	TSharedPtr<const FLobby> Lobby;

public:
	virtual void InitializeResult(const TSharedPtr<const FLobby>& InLobby) { Lobby = InLobby; }

	const TSharedPtr<const FLobby>& GetLobby() const { return Lobby; }


	///////////////////////////////////////////////////
	// Lobby Info
public:
	virtual FName GetLocalName() const;
	virtual FAccountId GetOwnerAccountId() const;
	virtual FLobbyId GetLobbyId() const;


	///////////////////////////////////////////////////
	// Lobby Attribute
public:
	/** 
	 * Gets an lobby attribute value as string
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool GetLobbyAttributeAsString(FName Key, FString& OutValue) const;

	/**
	 * Gets an lobby attribute value as int
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool GetLobbyAttributeAsInteger(FName Key, int32& OutValue) const;

	/**
	 * Gets an lobby attribute value as double
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool GetLobbyAttributeAsDouble(FName Key, double& OutValue) const;

	/**
	 * Gets an lobby attribute value as bool
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool GetLobbyAttributeAsBoolean(FName Key, bool& OutValue) const;

protected:
	/**
	 * Convert to a key with redirection set in DevSetting (Project => OnlineService)
	 */
	virtual FName ResolveAttributeKey(const FName& Key) const;


	///////////////////////////////////////////////////
	// Lobby Status
public:
	/** 
	 * Returns number of people allowed in the lobby
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetMaxMembers() const;

	/**
	 * Returns number of people currently in the lobby
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetNumMembers() const;

	/**
	 * Returns number of remaining openings in the lobby
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetNumOpenSlot() const;


	//////////////////////////////////////////////////////////////////////
	// Lobby Travel
protected:
	//
	// The travel URL that will be used after lobby operations are complete
	//
	UPROPERTY(Transient)
	FString LobbyTravelURL;

public:
	/**
	 * Execute lobby travel if there is currently a pending lobby travel
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	virtual const FString& GetLobbyTravelURL() const { return LobbyTravelURL; }

	virtual void SetLobbyTravelURL(const FString& URL) { LobbyTravelURL = URL; }
	virtual void ClearLobbyTravelURL() { LobbyTravelURL.Empty(); }


	///////////////////////////////////////////////////
	// Utilities
public:
	/**
	 * Returns an lobby id as debug string
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	FString GetDebugString() const;

};
