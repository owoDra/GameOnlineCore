// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlineServiceResultTypes.h"
#include "Type/OnlineLobbyCreateTypes.h"

#include "AsyncAction_ModifyLobbyJoinPolicy.generated.h"

class UOnlineLobbySubsystem;
class ULobbyResult;
class APlayerController;


/**
 * Delegate to notifies modify lobby join policy complete
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAsyncModifyLobbyJoinPolicyDelegate
												, const APlayerController*	, PlayerController
												, const ULobbyResult*		, LobbyResult
												, FOnlineServiceResult		, ServiceResult);


/**
 * Async action to join lobby
 */
UCLASS()
class GCONLINE_API UAsyncAction_ModifyLobbyJoinPolicy : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UOnlineLobbySubsystem> Subsystem;

	TWeakObjectPtr<APlayerController> PC;
	TWeakObjectPtr<const ULobbyResult> Lobby;
	ELobbyJoinablePolicy Policy;

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncModifyLobbyJoinPolicyDelegate OnComplete;

public:
	/**
	 * Modify hosting lobby's join policy
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_ModifyLobbyJoinPolicy* ModifyLobbyJoinPolicy(
		UOnlineLobbySubsystem* Target
		, APlayerController* PlayerController
		, const ULobbyResult* LobbyResult
		, ELobbyJoinablePolicy NewPolicy);

protected:
	virtual void Activate() override;

	virtual void HandleFailure();
	virtual void HandleModifyLobbyJoinPolicyComplete(const ULobbyResult* LobbyResult, FOnlineServiceResult Result);
	
};
