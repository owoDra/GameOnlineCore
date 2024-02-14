// Copyright (C) 2024 owoDra

#pragma once

#include "Type/OnlineServiceTypes.h"

#include "Online/Lobbies.h"

#include "OnlineLobbyTypes.generated.h"

using namespace UE::Online;

class USearchLobbyRequest;


////////////////////////////////////////////////////////////////////////
// Enums

/** 
 * Specifies the online features and connectivity that should be used for a game lobby 
 */
UENUM(BlueprintType)
enum class ELobbyOnlineMode : uint8
{
	LAN,
	Online
};

/**
 * Selecting options that can be specified as True/False for lowby searches, and
 */
UENUM(BlueprintType)
enum class ELobbySearchConditionalOption : uint8
{
	NotSpecified,
	Enabled,
	Disabled
};


////////////////////////////////////////////////////////////////////////
// Structs

/**
 * Information data for filtering used to search the lobby
 */
USTRUCT(BlueprintType)
struct FLobbyFilteringInfo
{
	GENERATED_BODY()
public:
	FLobbyFilteringInfo() = default;

public:
	//
	// Whether to search only dedicated server lobbies
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	ELobbySearchConditionalOption DedicatedServerOnly{ ELobbySearchConditionalOption::NotSpecified };

	//
	// Whether to search only empty lobbies
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	ELobbySearchConditionalOption EmptyOnly{ ELobbySearchConditionalOption::NotSpecified };

	//
	// Whether to search only not empty lobbies
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	ELobbySearchConditionalOption NoneEmptyOnly{ ELobbySearchConditionalOption::NotSpecified };

	//
	// Whether to search only secure lobbies
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	ELobbySearchConditionalOption SecureOnly{ ELobbySearchConditionalOption::NotSpecified };

	//
	// Whether to search only presence lobbies
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	ELobbySearchConditionalOption PresenceSearch{ ELobbySearchConditionalOption::Enabled };

	//
	// Minimum number of remaining available participants in the lobby 
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	int32 MinSlotsAvailable{ INDEX_NONE };

	//
	// Minimum number of remaining available participants in the lobby 
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	TArray<FUniqueNetIdRepl> ExcludeUniqueIds;

	//
	// User ID to search for lobby of
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	FString SearchUser;

	//
	// Keywords to match in lobby search
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	FString SearchKeywords;

	//
	// The matchmaking queue name to matchmake in, e.g. "TeamDeathmatch"
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	FString MatchmakingQueue;

public:
	void FillFilteringParamForSearch(TArray<FFindLobbySearchFilter>& Param);

};


////////////////////////////////////////////////////////////////////////
// Delegates

/** 
 * Delegates called when a lobby search completes 
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FFindLobbiesFinishedDelegate
										, bool bSucceeded
										, const FText& ErrorMessage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFindLobbiesFinishedDynamicDelegate
										, bool, bSucceeded
										, FText, ErrorMessage);


/**
 * Event triggered when the local user has requested to join a lobby from an external source, for example from a platform overlay.
 * Generally, the game should transition the player into the lobby.
 * 
 * @param LocalPlatformUserId the local user id that accepted the invitation. This is a platform user id because the user might not be signed in yet.
 * @param RequestedSession the requested lobby. Can be null if there was an error processing the request.
 * @param RequestedSessionResult result of the requested lobby processing
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FUserRequestLobbyDelegate
										, const FPlatformUserId&			/*LocalPlatformUserId*/
										, USearchLobbyResult*				/*RequestedSession*/
										, const FOnlineResultInformation&	/*RequestedSessionResult*/);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FUserRequestLobbyDynamicDelegate
										, const FPlatformUserId&			, LocalPlatformUserId
										, USearchLobbyResult*				, RequestedSession
										, const FOnlineResultInformation&	, RequestedSessionResult);


/**
 * Event triggered when a lobby join has completed, after joining the underlying lobby and before traveling to the server if it was successful.
 * The event parameters indicate if this was successful, or if there was an error that will stop it from traveling.
 * 
 * @param Result result of the lobby join
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FJoinLobbyCompleteDelegate, const FOnlineResultInformation& /*Result*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FJoinLobbyCompleteDynamicDelegate, const FOnlineResultInformation&, Result);


/**
 * Event triggered when a lobby creation for hosting has completed, right before it travels to the map.
 * The event parameters indicate if this was successful, or if there was an error that will stop it from traveling.
 * 
 * @param Result result of the lobby join
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FCreateLobbyCompleteDelegate, const FOnlineResultInformation& /*Result*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCreateLobbyCompleteDynamicDelegate, const FOnlineResultInformation&, Result);


/**
 * Event triggered when a lobby join has completed, after resolving the connect string and prior to the client traveling.
 * 
 * @param URL resolved connection string for the lobby with any additional arguments
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FLobbyPreClientTravelDelegate, FString& /*URL*/);


////////////////////////////////////////////////////////////////////////
// Objects

/** 
 * A request object that stores the parameters used when hosting a gameplay lobby 
 */
UCLASS(BlueprintType)
class GCONLINE_API UHostLobbyRequest : public UObject
{
	GENERATED_BODY()
public:
	UHostLobbyRequest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	//
	// Indicates if the lobby is a full online lobby or a different type
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	ELobbyOnlineMode OnlineMode{ ELobbyOnlineMode::Online };

	//
	// String used during matchmaking to specify what type of game mode this is
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	FString ModeNameForAdvertisement;

	//
	// The map that will be loaded at the start of gameplay, this needs to be a valid Primary Asset top-level map
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby", meta = (AllowedTypes = "World"))
	FPrimaryAssetId MapID;

	//
	// Extra arguments passed as URL options to the game
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	TMap<FString, FString> ExtraArgs;

	//
	// Maximum players allowed per gameplay lobby
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	int32 MaxPlayerCount{ 16 };


public:
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
	virtual bool ValidateAndLogErrors(FText& OutError) const;

};


/** 
 * A result object returned from the online system that describes a joinable game lobby 
 */
UCLASS(BlueprintType)
class GCONLINE_API USearchLobbyResult : public UObject
{
	GENERATED_BODY()
public:
	USearchLobbyResult(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	//
	// Pointer to the platform-specific implementation
	//
	TSharedPtr<const FLobby> Lobby;

public:
	/** 
	 * Returns an internal description of the lobby, not meant to be human readable 
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	FString GetDescription() const;

	/** 
	 * Gets an arbitrary string setting, bFoundValue will be false if the setting does not exist 
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	void GetStringSetting(FName Key, FString& Value, bool& bFoundValue) const;

	/** 
	 * Gets an arbitrary integer setting, bFoundValue will be false if the setting does not exist 
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	void GetIntSetting(FName Key, int32& Value, bool& bFoundValue) const;

	/** 
	 * The number of private connections that are available 
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetNumOpenPrivateConnections() const;

	/** 
	 * The number of publicly available connections that are available 
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetNumOpenPublicConnections() const;

	/** 
	 * The maximum number of publicly available connections that could be available, including already filled connections 
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetMaxPublicConnections() const;

	/** 
	 * Ping to the search result, MAX_QUERY_PING is unreachable 
	 */
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetPingInMs() const;

};


/** 
 * Request object describing a lobby search, this object will be updated once the search has completed 
 */
UCLASS(BlueprintType)
class GCONLINE_API USearchLobbyRequest : public UObject
{
	GENERATED_BODY()
public:
	USearchLobbyRequest(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	///////////////////////////////////////////////
	// Search Parameter
public:
	//
	// Indicates if the this is looking for full online games or a different type like LAN
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	ELobbyOnlineMode OnlineMode;

	//
	// Indicates if the this is looking for full online games or a different type like LAN
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	int32 MaxResult{ 10 };

	//
	// Information for filtering used to search the lobby
	//
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	FLobbyFilteringInfo FilteringInfo;


	///////////////////////////////////////////////
	// Search Result
public:
	//
	// Exclude all matches where any unique ids in a given array are present
	//
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	TArray<TObjectPtr<USearchLobbyResult>> Results;

	//
	// Native Delegate called when a lobby search completes
	//
	FFindLobbiesFinishedDelegate OnSearchFinished;

public:
	/** 
	 * Called by subsystem to execute finished delegates 
	 */
	void NotifySearchFinished(bool bSucceeded, const FText& ErrorMessage);

private:
	//
	// Delegate called when a lobby search completes
	//
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (DisplayName = "On Search Finished", AllowPrivateAccess = true))
	FFindLobbiesFinishedDynamicDelegate K2_OnSearchFinished;

};


/**
 * Wrappers for the parameters actually used for lobby searches based on the request
 */
class FLobbySearchSettings : public FGCObject
{
public:
	FLobbySearchSettings(USearchLobbyRequest* InSearchRequest);
	~FLobbySearchSettings() = default;

public:
	TObjectPtr<USearchLobbyRequest> SearchRequest{ nullptr };

	FFindLobbies::Params FindLobbyParams;

public:
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(SearchRequest);
	}

	virtual FString GetReferencerName() const override
	{
		static const auto NameString{ FString(TEXT("FLobbySearchSettings")) };
		return NameString;
	}

};

