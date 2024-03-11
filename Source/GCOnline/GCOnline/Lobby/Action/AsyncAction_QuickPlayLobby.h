// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlineServiceResultTypes.h"
#include "Type/OnlineLobbySearchTypes.h"
#include "Type/OnlineLobbyJoinTypes.h"
#include "Type/OnlineLobbyCreateTypes.h"

#include "AsyncAction_QuickPlayLobby.generated.h"

class UOnlineLobbySubsystem;
class ULobbyResult;
class APlayerController;


/**
 * Delegate to notifies quick play lobby complete
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAsyncQuickPlayLobbyDelegate
												, const APlayerController*	, PlayerController
												, ULobbyResult*				, LobbyResult
												, FOnlineServiceResult		, ServiceResult);


/**
 * Async action to quick play lobby
 */
UCLASS()
class GCONLINE_API UAsyncAction_QuickPlayLobby : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<UOnlineLobbySubsystem> Subsystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> PC;

	UPROPERTY(Transient)
	TObjectPtr<ULobbySearchRequest> SearchReq;

	UPROPERTY(Transient)
	TObjectPtr<ULobbyCreateRequest> CreateReq;

	UPROPERTY(Transient)
	bool bCanCreateLobby{ false };

	UPROPERTY(Transient)
	bool bPendingCancel{ false };

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncQuickPlayLobbyDelegate OnComplete;

	UPROPERTY(BlueprintAssignable)
	FAsyncQuickPlayLobbyDelegate OnFailed;

	UPROPERTY(BlueprintAssignable)
	FAsyncQuickPlayLobbyDelegate OnCancelled;

public:
	/**
	 * Searchs a new online game using the lobby request information
	 */
	UFUNCTION(BlueprintCallable, Category = "Lobby", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_QuickPlayLobby* QuickPlayLobby(
		UOnlineLobbySubsystem* Target
		, APlayerController* PlayerController
		, ULobbySearchRequest* SearchRequest
		, ULobbyCreateRequest* CreateRequest
		, bool bCanBeHost = false);

protected:
	virtual void Activate() override;
	virtual void Cancel() override;

	//////////////////////////////////////////////////////////////////////////////
	// [Step A] Search and choose lobby
protected:
	virtual void StepA1_SearchLobby();
	virtual void StepA2_SelectLobby(ULobbySearchRequest* SearchRequest, FOnlineServiceResult Result);
	virtual ULobbyResult* ChoosePreferredLobby(const TArray<ULobbyResult*>& Results);


	//////////////////////////////////////////////////////////////////////////////
	// [Step B] Join preffered lobby
protected:
	virtual void StepB1_JoinLobby(ULobbyResult* PrefferedLobbyResult);
	virtual void StepB2_CompleteJoin(ULobbyJoinRequest* JoinRequest, FOnlineServiceResult Result);
	virtual ULobbyJoinRequest* CreatePreferredJoinRequest(ULobbyResult* PrefferedLobbyResult);


	//////////////////////////////////////////////////////////////////////////////
	// [Step C] Create new lobby
protected:
	virtual void StepC1_CreateLobby();
	virtual void StepC2_CompleteCreate(ULobbyCreateRequest* CreateRequest, FOnlineServiceResult Result);


	//////////////////////////////////////////////////////////////////////////////
	// Success
protected:
	virtual void HandleSuccess(ULobbyResult* LobbyResult);


	//////////////////////////////////////////////////////////////////////////////
	// Failure
protected:
	virtual void HandleFailure();
	virtual void HandleFailureWithResult(const FOnlineServiceResult& Result);


	//////////////////////////////////////////////////////////////////////////////
	// Cancel
protected:
	virtual void HandleLeaveLobby();

};
