// Copyright (C) 2024 owoDra

#pragma once

#include "Online/OnlineError.h"

#include "OnlineServiceResultTypes.generated.h"

using FOnlineErrorType = UE::Online::FOnlineError;


/** 
 * Information on the results of attempted actions performed using the online service
 * 
 * Tips:
 *	Mainly has information on whether the execution results in success or failure and error data in case of failure.
 *	It is also an efficient wrapper for OSS "FOnlineError"
 */
USTRUCT(BlueprintType)
struct GCONLINE_API FOnlineServiceResult
{
	GENERATED_BODY()
public:
	FOnlineServiceResult() = default;
	FOnlineServiceResult(const FOnlineErrorType& InOnlineError);

public:
	//
	// Whether the operation was successful or not
	// 
	// Tips:
	//	If it was successful, the error fields of this struct will not contain extra information
	//
	UPROPERTY(BlueprintReadOnly)
	bool bWasSuccessful{ true };

	//
	// The unique error id. Can be used to compare against specific handled errors
	//
	UPROPERTY(BlueprintReadOnly)
	FString ErrorId;

	//
	// Error text to display to the user
	//
	UPROPERTY(BlueprintReadOnly)
	FText ErrorText;

};


/**
 * Delegate to notifies information on the results of attempted actions performed using the online service
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnlineServiceResultDelegate, FOnlineServiceResult, Result);

