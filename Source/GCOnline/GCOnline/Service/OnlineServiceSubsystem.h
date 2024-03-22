// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "Type/OnlineServiceContextTypes.h"
#include "Type/OnlineServiceResultTypes.h"

#include "OnlineServiceSubsystem.generated.h"

namespace UE::Online
{
    using IOnlineServicesPtr = TSharedPtr<class IOnlineServices>;
}
using namespace UE::Online;

////////////////////////////////////////////////////////////////////

/**
 * Subsystem that manages the context for accessing each online service used.
 * 
 * Tips:
 *  Helpful when using multiple online services in a project
 */
UCLASS(BlueprintType)
class GCONLINE_API UOnlineServiceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UOnlineServiceSubsystem() {}

    ///////////////////////////////////////////////////////////////////////
    // Initialization
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;


    ///////////////////////////////////////////////////////////////////////
    // Context Cache
protected:
    IOnlineServicesPtr DefaultService;
    IOnlineServicesPtr PlatformService;

protected:
	void CreateOnlineServiceContexts();
	void DestroyOnlineServiceContexts();

public:
	/**
	* Gets internal data for a type of online service, can return null for service
	*/
	IOnlineServicesPtr GetContextCache(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

	/**
	 * Resolves a context that has default behavior into a specific context
	 */
	EOnlineServiceContext ResolveOnlineServiceContext(EOnlineServiceContext Context) const;

	/**
	 * True if there is a separate platform and service interface
	 */
    bool HasSeparatePlatformContext() const;

    /**
     * Return is onliner service enabled
     */
    UFUNCTION(BlueprintCallable, Category = "Onliner Service")
    virtual bool IsOnlineServiceReady() const;

    /**
     * Returns the type of online service for the given context
     */
    UFUNCTION(BlueprintCallable, Category = "Onliner Service")
    virtual EOnlineServiceType GetOnlineServiceType(EOnlineServiceContext Context) const;


    ///////////////////////////////////////////////////////////////////////
    // Error Message
protected:
    //
    // Delegate called when the system sends an error/warning message
    //
    UPROPERTY(BlueprintAssignable, Category = "Error Message")
    FOnlineServiceResultDelegate OnOnlineServiceErrorMessage;

public:
    /** 
     * Send a system message via OnOnlineServiceErrorMessage 
     */
    UFUNCTION(BlueprintCallable, Category = "Error Message")
    virtual void SendErrorMessage(const FOnlineServiceResult& InResult);

};
