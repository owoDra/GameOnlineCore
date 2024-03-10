// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlineServiceResultTypes.h"
#include "Type/OnlineLobbyAttributeTypes.h"

#include "AsyncAction_ModifyLobbyAttributes.generated.h"

class UOnlineLobbySubsystem;
class ULobbyResult;
class APlayerController;


/**
 * Delegate to notifies modify lobby attributes complete
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAsyncModifyLobbyAttributesDelegate
												, const APlayerController*	, PlayerController
												, const ULobbyResult*		, LobbyResult
												, FOnlineServiceResult		, ServiceResult);


/**
 * Async action to modify lobby attributes
 */
UCLASS()
class GCONLINE_API UAsyncAction_ModifyLobbyAttributes : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UOnlineLobbySubsystem> Subsystem;

	TWeakObjectPtr<APlayerController> PC;
	TWeakObjectPtr<const ULobbyResult> Lobby;
	TSet<FLobbyAttribute> ToChange;
	TSet<FLobbyAttribute> ToRemove;

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncModifyLobbyAttributesDelegate OnComplete;

public:
	/**
	 * Modify hosting lobby's attributes
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby", meta = (AutoCreateRefTerm = "AttrToChange, AttrToRemove", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_ModifyLobbyAttributes* ModifyLobbyAttributes(
		UOnlineLobbySubsystem* Target
		, APlayerController* PlayerController
		, const ULobbyResult* LobbyResult
		, const TSet<FLobbyAttribute>& AttrToChange
		, const TSet<FLobbyAttribute>& AttrToRemove);

protected:
	virtual void Activate() override;

	virtual void HandleFailure();
	virtual void HandleModifyLobbyAttributeComplete(const ULobbyResult* LobbyResult, FOnlineServiceResult Result);
	
};
