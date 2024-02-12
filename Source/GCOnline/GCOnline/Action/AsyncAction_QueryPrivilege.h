// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlinePrivilegeTypes.h"

#include "AsyncAction_QueryPrivilege.generated.h"

class UOnlinePrivilegeSubsystem;
class ULocalUserAccountInfo;


/**
 * Delegate to notifies query local user privilege
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FAsyncQueryPrivilegeDelegate
												, ULocalUserAccountInfo*	, User
												, EOnlineServiceContext		, Context
												, ELocalUserPrivilege		, DesiredPrivilege
												, ELocalUserPrivilegeResult	, Result);


/**
 * Async action to query local user privilege
 */
UCLASS()
class GCONLINE_API UAsyncAction_QueryPrivilege : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UOnlinePrivilegeSubsystem> Subsystem;

	TWeakObjectPtr<ULocalUserAccountInfo> TargetUser;
	EOnlineServiceContext Context;
	ELocalUserPrivilege DesiredPrivilege;

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
		, ULocalUserAccountInfo* InTargetUser
		, EOnlineServiceContext InContext
		, ELocalUserPrivilege InDesiredPrivilege);

protected:
	virtual void Activate() override;

	virtual void HandleQueryComplete(
		ULocalUserAccountInfo* UserInfo
		, EOnlineServiceContext	Context
		, ELocalUserPrivilege DesiredPrivilege
		, ELocalUserPrivilegeResult PrivilegeResult
		, const TOptional<FOnlineErrorType>& Error);
	
};
