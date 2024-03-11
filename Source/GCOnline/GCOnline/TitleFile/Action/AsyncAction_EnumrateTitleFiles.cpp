// Copyright (C) 2024 owoDra

#include "AsyncAction_EnumrateTitleFiles.h"

#include "OnlineTitleFileSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_EnumrateTitleFiles)


UAsyncAction_EnumrateTitleFiles* UAsyncAction_EnumrateTitleFiles::EnumrateTitleFiles(UOnlineTitleFileSubsystem* Target, const APlayerController* InPlayerController)
{
	auto* Action{ NewObject<UAsyncAction_EnumrateTitleFiles>() };

	Action->RegisterWithGameInstance(Target);
	Action->Subsystem = Target;
	Action->PC = InPlayerController;

	return Action;
}


void UAsyncAction_EnumrateTitleFiles::Activate()
{
	if (Subsystem.IsValid() && IsRegistered())
	{
		auto NewDelegate{ FEnumerateFilesCompleteDelegate::CreateUObject(this, &ThisClass::HandleEnumrateComplete) };

		if (Subsystem->EnumerateFiles(PC.Get(), NewDelegate))
		{
			return;
		}
	}

	HandleFailure();
}

void UAsyncAction_EnumrateTitleFiles::HandleFailure()
{
	if (ShouldBroadcastDelegates())
	{
		FOnlineServiceResult Result;
		Result.bWasSuccessful = false;
		Result.ErrorId = TEXT("Enumrate Files Failed");
		Result.ErrorText = NSLOCTEXT("GameOnlineCore", "EnumrateFilesFailed", "Enumrate Files Failed");

		OnEnumrated.Broadcast(TArray<FString>(), Result);
	}

	SetReadyToDestroy();
}

void UAsyncAction_EnumrateTitleFiles::HandleEnumrateComplete(const TArray<FString>& FileNames, FOnlineServiceResult Result)
{
	if (ShouldBroadcastDelegates())
	{
		OnEnumrated.Broadcast(FileNames, Result);
	}

	SetReadyToDestroy();
}
