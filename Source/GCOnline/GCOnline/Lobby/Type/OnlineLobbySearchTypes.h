// Copyright (C) 2024 owoDra

#pragma once

#include "Type/OnlineServiceResultTypes.h"
#include "Type/OnlineLobbyAttributeTypes.h"

#include "Online/Lobbies.h"

#include "OnlineLobbySearchTypes.generated.h"

using namespace UE::Online;


////////////////////////////////////////////////////////////////////////
// Delegates

/** 
 * Delegates called when a lobby search completes 
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FFindLobbiesFinishedDelegate, FOnlineServiceResult /* Result */);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFindLobbiesFinishedDynamicDelegate, FOnlineServiceResult, Result);


////////////////////////////////////////////////////////////////////////
// Objects

/** 
 * A result object returned from the online system that describes a joinable game lobby 
 */
UCLASS(BlueprintType)
class GCONLINE_API ULobbySearchResult : public UObject
{
	GENERATED_BODY()
public:
	ULobbySearchResult(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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


	///////////////////////////////////////////////////
	// Utilities
public:
	/**
	 * Returns an lobby id as debug string
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	FString GetDebugString() const;

};


/** 
 * Request object describing a lobby search, this object will be updated once the search has completed 
 */
UCLASS(BlueprintType)
class GCONLINE_API ULobbySearchRequest : public UObject
{
	GENERATED_BODY()
public:
	ULobbySearchRequest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	///////////////////////////////////////////////
	// Search Parameter
public:
	//
	// Maximum number of search results
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	int32 MaxResult{ 10 };

	//
	// Filter list to filter lobbies to search
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	TSet<FLobbyAttributeFilter> Filters;

public:
	/**
	 * Generate parameters for lobby search from current settings
	 */
	FFindLobbies::Params GenerateFindParameters() const;


	///////////////////////////////////////////////
	// Search Result
public:
	//
	// Exclude all matches where any unique ids in a given array are present
	//
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	TArray<TObjectPtr<ULobbySearchResult>> Results;

	//
	// Native Delegate called when a lobby search completes
	//
	FFindLobbiesFinishedDelegate OnSearchFinished;

public:
	/** 
	 * Called by subsystem to execute finished delegates 
	 */
	void NotifySearchFinished(const FOnlineServiceResult& Result);

private:
	//
	// Delegate called when a lobby search completes
	//
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On Search Finished", AllowPrivateAccess = true))
	FFindLobbiesFinishedDynamicDelegate K2_OnSearchFinished;

};
