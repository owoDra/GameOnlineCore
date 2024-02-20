// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlineAuthLoginTypes.h"

#include "AsyncAction_Login.generated.h"

class UOnlineAuthSubsystem;
class UOnlineLocalUserSubsystem;
class APlayerController;


/**
 * Async action to handle different functions for login users
 */
UCLASS()
class GCONLINE_API UAsyncAction_Login : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UOnlineAuthSubsystem> Subsystem;

	TWeakObjectPtr<APlayerController> PlayerController;
	FLocalUserLoginParams Params;

public:
	//
	// Call when login succeeds or fails
	//
	UPROPERTY(BlueprintAssignable)
	FLocalUserLoginCompleteDynamicMulticastDelegate OnLoginComplete;

public:
	/**
	 * Tries to process login with local player as local play user
     * Update an existing LocalPlayer or create a new LocalPlayer as needed
     * 
     * Tips:
     *  When the process has succeeded or failed, it will broadcast the OnUserLoginComplete delegate.
	 */
	UFUNCTION(BlueprintCallable, Category = "Login", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_Login* LoginForLocalPlay(UOnlineAuthSubsystem* Target, APlayerController* InPlayerController);

	/**
	 * Tries to process login with local player as online play user
     * Update an existing LocalPlayer or create a new LocalPlayer as needed
     * 
     * Note:
     *  A local player must be created to log in to online play.
	 *	The primary local player will be created automatically, but from the secondary player onward, 
	 *	please log in to Local Play first and create a local player
     * 
     * Tips:
     *  When the process has succeeded or failed, it will broadcast the OnUserLoginComplete delegate.
	 */
	UFUNCTION(BlueprintCallable, Category = "Login", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_Login* LoginForOnlinePlay(UOnlineAuthSubsystem* Target, APlayerController* InPlayerController);

protected:
	/** 
	 * Actually start the initialization 
	 */
	virtual void Activate() override;

	/** 
	 * Fail and send callbacks if needed 
	 */
	void HandleFailure();

	/** 
	 * Wrapper delegate, will pass on to OnInitializationComplete if appropriate 
	 */
	UFUNCTION()
	virtual void HandleInitializationComplete(UOnlineLocalUserSubsystem* LocalUser, FOnlineServiceResult Result, EOnlineServiceContext OnlineContext);
	
};
