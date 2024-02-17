// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlineLobbySearchTypes.h"

#include "AsyncAction_SearchLobby.generated.h"

class UOnlineLobbySubsystem;
class APlayerController;


/**
 * Delegate to notifies search lobby complete
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAsyncSearchLobbyDelegate
												, const APlayerController*	, PlayerController
												, ULobbySearchRequest*		, SearchRequest
												, FOnlineServiceResult		, ServiceResult);


/**
 * Async action to search lobbies
 */
UCLASS()
class GCONLINE_API UAsyncAction_SearchLobby : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UOnlineLobbySubsystem> Subsystem;

	TWeakObjectPtr<APlayerController> PC;
	TWeakObjectPtr<ULobbySearchRequest> Request;

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncSearchLobbyDelegate OnComplete;

public:
	/**
	 * Searchs a new online game using the lobby request information
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_SearchLobby* SearchLobby(
		UOnlineLobbySubsystem* Target
		, APlayerController* PlayerController
		, ULobbySearchRequest* SearchRequest);

protected:
	virtual void Activate() override;

	virtual void HandleFailure();
	virtual void HandleSearchLobbyComplete(ULobbySearchRequest* SearchRequest, FOnlineServiceResult Result);
	
};
