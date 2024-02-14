// Copyright (C) 2024 owoDra

#include "OnlineServiceTypes.h"

#include "OnlineError.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineServiceTypes)


/////////////////////////////////////////////////////////////////////
// FOnlineResultInformation

void FOnlineResultInformation::FromOnlineError(const FOnlineErrorType& InOnlineError)
{
	bWasSuccessful = (InOnlineError != Errors::Success());
	ErrorId = InOnlineError.GetErrorId();
	ErrorText = InOnlineError.GetText();
}