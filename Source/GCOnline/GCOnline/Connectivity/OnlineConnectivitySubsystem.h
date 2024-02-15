// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "Type/OnlineServiceContextTypes.h"

// OSSv2
#include "Online/Connectivity.h"
#include "Online/OnlineAsyncOpHandle.h"

#include "OnlineConnectivitySubsystem.generated.h"

///////////////////////////////////////////////////

namespace UE::Online
{
    using IConnectivityPtr = TSharedPtr<class IConnectivity>;

    struct FConnectionStatusChanged;
}
using namespace UE::Online;

class UOnlineServiceSubsystem;

///////////////////////////////////////////////////

/**
 * Subsystem with features to extend the functionality of Online Servicies (OSSv2) and make it easier to use in projects
 * Subsystems to determine the status of connection to online services
 * 
 * Include OSSv2 Interface Feature:
 *  - Connectivity Interface
 */
UCLASS(BlueprintType)
class GCONLINE_API UOnlineConnectivitySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UOnlineConnectivitySubsystem() {}

    ///////////////////////////////////////////////////////////////////////
    // Initialization
protected:
    TMap<EOnlineServiceContext, FOnlineEventDelegateHandle> ConnectionHandles;
    TMap<EOnlineServiceContext, EOnlineServicesConnectionStatus> ConnectionStatusCaches;

	UPROPERTY(Transient)
    TObjectPtr<UOnlineServiceSubsystem> OnlineServiceSubsystem{ nullptr };

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
    void BindConnectivityDelegates();
    void UnbindConnectivityDelegates();

    /**
     * Returns connectivity interface of specific type, will return null if there is no type
     */
    IConnectivityPtr GetConecctivityInterface(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

    
    ///////////////////////////////////////////////////////////////////////
    // Connectivity
protected:
    virtual void HandleNetworkConnectionStatusChanged(const FConnectionStatusChanged& EventParameters, EOnlineServiceContext Context);

public:
    /**
     * Returns the current online connection status
     */
    EOnlineServicesConnectionStatus GetConnectionStatus(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

    /**
     * Returns true if we are currently connected to backend servers
     */
    bool HasOnlineConnection(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

};
