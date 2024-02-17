// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlineLobbyCreateTypes.h"

#include "AsyncAction_CreateLobby.generated.h"

class UOnlineLobbySubsystem;
class APlayerController;


/**
 * Delegate to notifies create lobby complete
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAsyncCreateLobbyDelegate
												, const APlayerController*	, PlayerController
												, ULobbyCreateRequest*		, CreateRequest
												, FOnlineServiceResult		, ServiceResult);


/**
 * Async action to create lobby
 */
UCLASS()
class GCONLINE_API UAsyncAction_CreateLobby : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UOnlineLobbySubsystem> Subsystem;

	TWeakObjectPtr<APlayerController> PC;
	TWeakObjectPtr<ULobbyCreateRequest> Request;

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncCreateLobbyDelegate OnComplete;

public:
	/**
	 * Creates a new online game using the lobby request information
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_CreateLobby* CreateLobby(
		UOnlineLobbySubsystem* Target
		, APlayerController* PlayerController
		, ULobbyCreateRequest* CreateRequest);

protected:
	virtual void Activate() override;

	virtual void HandleFailure();
	virtual void HandleCreateLobbyComplete(ULobbyCreateRequest* CreateRequest, FOnlineServiceResult Result);
	
};
