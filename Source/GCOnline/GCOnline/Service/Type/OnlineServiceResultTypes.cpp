// Copyright (C) 2024 owoDra

#include "OnlineServiceResultTypes.h"

#include "OnlineError.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineServiceResultTypes)


/////////////////////////////////////////////////////////////////////
// FOnlineResultInformation

FOnlineServiceResult::FOnlineServiceResult(const FOnlineErrorType& InOnlineError)
{
	bWasSuccessful = false;
	ErrorId = InOnlineError.GetErrorId();
	ErrorText = InOnlineError.GetText();
}
