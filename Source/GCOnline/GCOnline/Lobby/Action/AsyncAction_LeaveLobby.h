// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlineLobbyJoinTypes.h"

#include "AsyncAction_LeaveLobby.generated.h"

class UOnlineLobbySubsystem;
class APlayerController;


/**
 * Delegate to notifies leave lobby complete
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAsyncLeaveLobbyDelegate
												, const APlayerController*	, PlayerController
												, FOnlineServiceResult		, ServiceResult);


/**
 * Async action to leave lobby
 */
UCLASS()
class GCONLINE_API UAsyncAction_LeaveLobby : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UOnlineLobbySubsystem> Subsystem;

	TWeakObjectPtr<APlayerController> PC;
	FName LocalName;

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncLeaveLobbyDelegate OnComplete;

public:
	/**
	 * Leave from sepecific joined lobby
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_LeaveLobby* LeaveLobby(
		UOnlineLobbySubsystem* Target
		, APlayerController* PlayerController
		, FName InLocalName);

protected:
	virtual void Activate() override;

	virtual void HandleFailure();
	virtual void HandleLeaveLobbyComplete(FOnlineServiceResult Result);
	
};
