// Copyright (C) 2024 owoDra

#pragma once

#include "Subsystems/GameInstanceSubsystem.h"

#include "Type/OnlineServiceResultTypes.h"

// OSSv2
#include "Online/TitleFile.h"
#include "Online/OnlineAsyncOpHandle.h"

#include "OnlineTitleFileSubsystem.generated.h"

///////////////////////////////////////////////////

namespace UE::Online
{
    using ITitleFilePtr = TSharedPtr<class ITitleFile>;
}
using namespace UE::Online;

class UOnlineServiceSubsystem;
class APlayerController;

///////////////////////////////////////////////////


/**
 * Event triggered when file enumeration is complete.
 */
DECLARE_DELEGATE_TwoParams(FEnumerateFilesCompleteDelegate, const TArray<FString>& /*FileNames*/, FOnlineServiceResult /*Result*/);


/**
 * Event triggered when file reading is complete.
 */
DECLARE_DELEGATE_TwoParams(FReadFileCompleteDelegate, const TArray<uint8>& /*FileData*/, FOnlineServiceResult /*Result*/);


/**
 * Subsystem with features to extend the functionality of Online Servicies (OSSv2) and make it easier to use in projects
 * This subsystem assists in accessing title storage provided by online services.
 * 
 * Include OSSv2 Interface Feature:
 *  - TitleFile Interface
 */
UCLASS(BlueprintType)
class GCONLINE_API UOnlineTitleFileSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    UOnlineTitleFileSubsystem() {}

    ///////////////////////////////////////////////////////////////////////
    // Initialization
protected:
	UPROPERTY(Transient)
    TObjectPtr<UOnlineServiceSubsystem> OnlineServiceSubsystem{ nullptr };

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

protected:
    void BindTitleFileDelegates();
    void UnbindTitleFileDelegates();

    /**
     * Returns title storage interface of specific type, will return null if there is no type
     */
    ITitleFilePtr GetTitleFileInterface(EOnlineServiceContext Context = EOnlineServiceContext::Default) const;

  
    ///////////////////////////////////////////////////////////////////////
    // Enumerate Files
public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Title File")
    virtual TArray<FString> GetEnumerateFiles(const APlayerController* PlayerController) const;

    virtual bool EnumerateFiles(
        const APlayerController* PlayerController
        , FEnumerateFilesCompleteDelegate Delegate = FEnumerateFilesCompleteDelegate());

protected:
    virtual void EnumerateFilesInternal(
        const APlayerController* PlayerController
        , FEnumerateFilesCompleteDelegate Delegate = FEnumerateFilesCompleteDelegate());

    virtual void HandleEnumerateFilesComplete(
        const TOnlineResult<FTitleFileEnumerateFiles>& EnumerateResult
        , const APlayerController* PlayerController
        , FEnumerateFilesCompleteDelegate Delegate);


    ///////////////////////////////////////////////////////////////////////
    // Read File
public:
    virtual bool ReadFile(
        const APlayerController* PlayerController
        , const FString& Filename
        , FReadFileCompleteDelegate Delegate = FReadFileCompleteDelegate());

protected:
    virtual void ReadFileInternal(
        const APlayerController* PlayerController
        , const FString& Filename
        , FReadFileCompleteDelegate Delegate = FReadFileCompleteDelegate());

    virtual void HandleReadFileComplete(
        const TOnlineResult<FTitleFileReadFile>& ReadResult
        , FReadFileCompleteDelegate Delegate);

};
