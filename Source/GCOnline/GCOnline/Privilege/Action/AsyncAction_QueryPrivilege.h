// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlinePrivilegeTypes.h"

#include "AsyncAction_QueryPrivilege.generated.h"

class UOnlinePrivilegeSubsystem;
class ULocalPlayer;
class APlayerController;


/**
 * Delegate to notifies query local user privilege
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FAsyncQueryPrivilegeDelegate
												, const ULocalPlayer*		, LocalPlayer
												, EOnlineServiceContext		, Context
												, EOnlinePrivilege			, DesiredPrivilege
												, EOnlinePrivilegeResult	, PrivilegeResult
												, FOnlineServiceResult		, ServiceResult);


/**
 * Async action to query local user privilege
 */
UCLASS()
class GCONLINE_API UAsyncAction_QueryPrivilege : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UOnlinePrivilegeSubsystem> Subsystem;

	TWeakObjectPtr<const ULocalPlayer> LocalPlayer;
	EOnlineServiceContext Context;
	EOnlinePrivilege DesiredPrivilege;

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncQueryPrivilegeDelegate OnQuery;

public:
	/**
	 * Query the local user's account for privileges on available online services
	 */
	UFUNCTION(BlueprintCallable, Category = "Privileges", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_QueryPrivilege* QueryLocalUserPrivilege(
		UOnlinePrivilegeSubsystem* Target
		, const APlayerController* InPlayerController
		, EOnlineServiceContext InContext
		, EOnlinePrivilege InDesiredPrivilege);

protected:
	virtual void Activate() override;

	virtual void HandleQueryComplete(
		const ULocalPlayer* InLocalPlayer
		, EOnlineServiceContext	InContext
		, EOnlinePrivilege InDesiredPrivilege
		, EOnlinePrivilegeResult InPrivilegeResult
		, FOnlineServiceResult InResult);
	
};
