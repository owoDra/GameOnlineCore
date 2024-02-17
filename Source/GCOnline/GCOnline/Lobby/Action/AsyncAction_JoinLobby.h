// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlineLobbyJoinTypes.h"

#include "AsyncAction_JoinLobby.generated.h"

class UOnlineLobbySubsystem;
class APlayerController;


/**
 * Delegate to notifies join lobby complete
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAsyncJoinLobbyDelegate
												, const APlayerController*	, PlayerController
												, ULobbyJoinRequest*		, JoinRequest
												, FOnlineServiceResult		, ServiceResult);


/**
 * Async action to join lobby
 */
UCLASS()
class GCONLINE_API UAsyncAction_JoinLobby : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UOnlineLobbySubsystem> Subsystem;

	TWeakObjectPtr<APlayerController> PC;
	TWeakObjectPtr<ULobbyJoinRequest> Request;

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncJoinLobbyDelegate OnComplete;

public:
	/**
	 * Joins a new online game using the lobby request information
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_JoinLobby* JoinLobby(
		UOnlineLobbySubsystem* Target
		, APlayerController* PlayerController
		, ULobbyJoinRequest* JoinRequest);

protected:
	virtual void Activate() override;

	virtual void HandleFailure();
	virtual void HandleJoinLobbyComplete(ULobbyJoinRequest* JoinRequest, FOnlineServiceResult Result);
	
};
