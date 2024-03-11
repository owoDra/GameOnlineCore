// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/CancellableAsyncAction.h"

#include "AsyncAction_EnumrateTitleFiles.generated.h"

class UOnlineTitleFileSubsystem;
class APlayerController;


/**
 * Delegate to notifies enumrate title file complete
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAsyncEnumrateTitleFilesDelegate
												, const TArray<FString>&	, Filenames
												, FOnlineServiceResult		, Result);


/**
 * Async action to enumrate title file
 */
UCLASS()
class GCONLINE_API UAsyncAction_EnumrateTitleFiles : public UCancellableAsyncAction
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<UOnlineTitleFileSubsystem> Subsystem;

	UPROPERTY(Transient)
	TWeakObjectPtr<const APlayerController> PC;

public:
	UPROPERTY(BlueprintAssignable)
	FAsyncEnumrateTitleFilesDelegate OnEnumrated;

public:
	/**
	 * Async action to enumrate title file
	 */
	UFUNCTION(BlueprintCallable, Category = "Title File", meta = (BlueprintInternalUseOnly = "true"))
	static UAsyncAction_EnumrateTitleFiles* EnumrateTitleFiles(
		UOnlineTitleFileSubsystem* Target
		, const APlayerController* InPlayerController);

protected:
	virtual void Activate() override;

	virtual void HandleFailure();
	virtual void HandleEnumrateComplete(const TArray<FString>& FileNames, FOnlineServiceResult Result);
	
};
