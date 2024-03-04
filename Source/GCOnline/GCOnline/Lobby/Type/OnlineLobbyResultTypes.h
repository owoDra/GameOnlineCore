// Copyright (C) 2024 owoDra

#pragma once

#include "Type/OnlineServiceResultTypes.h"
#include "Type/OnlineLobbyAttributeTypes.h"

#include "Online/Lobbies.h"

#include "OnlineLobbyResultTypes.generated.h"

using namespace UE::Online;

class UOnlineLobbySubsystem;


/**
 * LobbyId wrappper for blueprint usage
 */
USTRUCT(BlueprintType)
struct FLobbyIdWrapper
{
	GENERATED_BODY()
public:
	FLobbyIdWrapper() = default;
	FLobbyIdWrapper(const FLobbyId& InLobbyId)
	{
		Type = static_cast<uint8>(InLobbyId.GetOnlineServicesType());
		Handle = static_cast<int32>(InLobbyId.GetHandle());
	}

private:
	UPROPERTY()
	uint8 Type{ static_cast<uint8>(EOnlineServices::None) };

	UPROPERTY()
	int32 Handle;

public:
	FLobbyId GetLobbyId() const
	{
		return FLobbyId(static_cast<EOnlineServices>(Type), static_cast<uint32>(Handle));
	}

	bool IsValid() const
	{
		return (Type != static_cast<uint8>(EOnlineServices::None));
	}
};


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
	// Temporal Lobby Result
protected:
	//
	// LobbyId for temporary lobby results created using LobbyId
	// 
	// Tips:
	//	This is used to recover from a disconnected lobby due to communication problems, for example, when using a saved LobbyId.
	//
	UPROPERTY(Transient)
	FLobbyIdWrapper TemporalLobbyId;

public:
	UFUNCTION(BlueprintCallable, Category = "Lobby", meta = (WorldContext = "InWorldContext"))
	static ULobbyResult* CreateTemporalLobbyResult(UOnlineLobbySubsystem* InSubsystem, FLobbyIdWrapper InLobbyIdWrapper);

	///////////////////////////////////////////////////
	// Utilities
public:
	/**
	 * Returns an lobby id as debug string
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	FString GetDebugString() const;

	/**
	 * Returns lobby id wrapper
	 */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	FLobbyIdWrapper GetLobbyIdWrapper() const;

};
