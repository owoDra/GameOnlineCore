// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "GameplayTagContainer.h"

#include "OnlineErrorSubsystem.generated.h"


/**
 * Delegate to notifies an error/warning message
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnlineServiceSystemMessageDelegate
                                                    , FGameplayTag  , MessageType
                                                    , FText         , TitleText
                                                    , FText         , BodyText);


/**
 * Subsystem to handle errors that occur in accessing online services
 * 
 * Tips:
 *  By creating a derivative of this class in each project, you can implement in-game error message display, etc.
 */
UCLASS(BlueprintType)
class GCONLINE_API UOnlineErrorSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UOnlineErrorSubsystem() {}

    ///////////////////////////////////////////////////////////////////////
    // Initialization
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;


    ///////////////////////////////////////////////////////////////////////
    // System Message
protected:
    //
    // Delegate called when the system sends an error/warning message
    //
    UPROPERTY(BlueprintAssignable, Category = "System Message")
    FOnlineServiceSystemMessageDelegate OnOnlineServiceSystemMessage;

public:
    /** 
     * Send a system message via OnOnlineServiceSystemMessage 
     */
    UFUNCTION(BlueprintCallable, Category = "System Message", meta = (GameplayTagFilter = "Message.Online"))
    virtual void SendSystemMessage(FGameplayTag MessageType, FText TitleText, FText BodyText);

};
