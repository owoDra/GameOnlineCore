// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "Type/OnlineUserTypes.h"

#include "AsyncAction_Login.generated.h"

class UOnlineUserSubsystem;


/**
 * Async action to handle different functions for login users
 */
UCLASS()
class GCONLINE_API UAsyncAction_Login : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	TWeakObjectPtr<UOnlineUserSubsystem> Subsystem;

	FLocalUserLoginParams Params;

public:
	//
	// Call when initialization succeeds or fails
	//
	UPROPERTY(BlueprintAssignable)
	FLocalUserLoginCompleteDynamicMulticastDelegate OnInitializationComplete;

public:
	/**
	 * Tries to process login with local player as local play user
     * Update an existing LocalPlayer or create a new LocalPlayer as needed
     * 
     * Tips:
     *  When the process has succeeded or failed, it will broadcast the OnUserLoginComplete delegate.
     *
     * @param LocalPlayerIndex	Desired index of LocalPlayer in Game Instance, 0 will be primary player and 1+ for local multiplayer
     * @param PrimaryInputDevice The physical controller that should be mapped to this user, will use the default device if invalid
     * @param bCanUseGuestLogin	If true, this player can be a guest without a real Unique Net Id
	 */
	UFUNCTION(BlueprintCallable, Category = "Login", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_Login* LoginForLocalPlay(
		UOnlineUserSubsystem* Target
		, int32 LocalPlayerIndex
		, FInputDeviceId PrimaryInputDevice
		, bool bCanUseGuestLogin);

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
     *
     * @param LocalPlayerIndex	Index of existing LocalPlayer in Game Instance
	 */
	UFUNCTION(BlueprintCallable, Category = "Login", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_Login* LoginForOnlinePlay(UOnlineUserSubsystem* Target, int32 LocalPlayerIndex);

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
	virtual void HandleInitializationComplete(const ULocalUserAccountInfo* UserInfo, bool bSuccess, FText Error, ELocalUserPrivilege RequestedPrivilege, EOnlineServiceContext OnlineContext);
	
};
