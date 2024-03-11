// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "AsyncAction_ReadTitleFile.generated.h"

class UOnlineTitleFileSubsystem;
class APlayerController;


/**
 * Delegate to notifies read title file complete
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAsyncReadTitleFileDelegate
	, const TArray<uint8>&, Data
	, FOnlineServiceResult, Result);


/**
 * Async action to read title file
 */
UCLASS()
class GCONLINE_API UAsyncAction_ReadTitleFile : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<UOnlineTitleFileSubsystem> Subsystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<const APlayerController> PC;

	UPROPERTY(Transient)
	FString FileName;

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncReadTitleFileDelegate OnRead;

public:
	/**
	 * Async action to read title file
	 */
	UFUNCTION(BlueprintCallable, Category = "Title File", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_ReadTitleFile* ReadTitleFile(
		UOnlineTitleFileSubsystem* Target
		, const APlayerController* InPlayerController
		, FString InFileName);

protected:
	virtual void Activate() override;

	virtual void HandleFailure();
	virtual void HandleReadComplete(const TArray<uint8>& Data, FOnlineServiceResult Result);

};
