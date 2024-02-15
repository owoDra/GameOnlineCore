// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "OnlineLocalUserManagerSubsystem.generated.h"

///////////////////////////////////////////////////

class UOnlineLocalUserSubsystem;

///////////////////////////////////////////////////

/**
 * Subsystem that manages local players that have already been initialized as local users
 */
UCLASS(BlueprintType)
class GCONLINE_API UOnlineLocalUserManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UOnlineLocalUserManagerSubsystem() {}

    ///////////////////////////////////////////////////////////////////////
    // Initialization
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;


    ///////////////////////////////////////////////////////////////////////
    // Local User
protected:
    //
    // Maximum number of local players can initialize as local user
    //
    UPROPERTY(Transient)
    int32 MaxLocalUsers{ 0 };

public:
    /** 
     * Sets the maximum number of local players, will not destroy existing ones 
     */
    UFUNCTION(BlueprintCallable, Category = "Local User")
    virtual void SetMaxLocalUsers(int32 InMaxLocalUsers);

    /** 
     * Gets the maximum number of local players 
     */
    UFUNCTION(BlueprintPure, Category = "Local User")
    int32 GetMaxLocalUsers() const;

    /** 
     * Gets the current number of local players, will always be at least 1 
     */
    UFUNCTION(BlueprintPure, Category = "Local User")
    int32 GetNumLocalPlayers() const;

public:
    /** 
     * Returns the user info for a given local player index in game instance, 0 is always valid in a running game 
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "Local User")
    UOnlineLocalUserSubsystem* GetUserInfoForLocalPlayerIndex(int32 LocalPlayerIndex) const;

    /** 
     * Returns the primary user info for a given platform user index. Can return null 
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "Local User")
    UOnlineLocalUserSubsystem* GetUserInfoForPlatformUser(FPlatformUserId PlatformUser) const;

    /** 
     * Returns the user info for a unique net id. Can return null 
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "Local User")
    UOnlineLocalUserSubsystem* GetUserInfoForUniqueNetId(const FUniqueNetIdRepl& NetId) const;

    /** 
     * Returns the user info for a given input device. Can return null 
     */
    UFUNCTION(BlueprintCallable, BlueprintPure = False, Category = "Local User")
    UOnlineLocalUserSubsystem* GetUserInfoForInputDevice(FInputDeviceId InputDevice) const;

public:
    /** 
     * Returns true if this this could be a real platform user with a valid identity (even if not currently logged in) 
     */
    virtual bool IsRealPlatformUser(FPlatformUserId PlatformUser) const;

    /** 
     * Gets the user for an input device 
     */
    virtual FPlatformUserId GetPlatformUserIdForInputDevice(FInputDeviceId InputDevice) const;

    /** 
     * Gets a user's primary input device id 
     */
    virtual FInputDeviceId GetPrimaryInputDeviceForPlatformUser(FPlatformUserId PlatformUser) const;


    ///////////////////////////////////////////////////////////////////////
    // Local User State
public:
    /**
     * Resets the all state when returning to the main menu after an error
     */
    UFUNCTION(BlueprintCallable, Category = "Local User State")
    virtual void ResetAllLocalUserStates(bool bDestroyPlayer = true);

    /**
     * Initialize the local user associated with the specified local player index
     * 
     * Tips:
     *  Automatically initialized for Primary Local Player.
     *  If a local player for the specified index does not exist, create a new one.
     */
    UFUNCTION(BlueprintCallable, Category = "Local User State")
    virtual bool InitializeLocalUser(int32 LocaPlayerIndex, FInputDeviceId InPrimaryInputDevice, bool bCanUseGuestLogin);


};
