// Copyright (C) 2024 owoDra

#pragma once

#include "Type/OnlineServiceResultTypes.h"
#include "Type/OnlineLobbyAttributeTypes.h"

#include "Online/Lobbies.h"

#include "OnlineLobbySearchTypes.generated.h"

using namespace UE::Online;

class ULobbyResult;


////////////////////////////////////////////////////////////////////////
// Delegates

/** 
 * Delegates called when a lobby search completes 
 */
DECLARE_DELEGATE_TwoParams(FLobbySearchCompleteDelegate, ULobbySearchRequest*/*Request*/, FOnlineServiceResult/*Result*/);


////////////////////////////////////////////////////////////////////////
// Objects

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
	// Search Parameters
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
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lobby")
	TArray<TObjectPtr<ULobbyResult>> Results;

};
