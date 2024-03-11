// Copyright (C) 2024 owoDra

#include "AsyncAction_ReadTitleFile.h"

#include "OnlineTitleFileSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_ReadTitleFile)


UAsyncAction_ReadTitleFile* UAsyncAction_ReadTitleFile::ReadTitleFile(UOnlineTitleFileSubsystem* Target, const APlayerController* InPlayerController, FString InFileName)
{
	auto* Action{ NewObject<UAsyncAction_ReadTitleFile>() };

	Action->RegisterWithGameInstance(Target);
	Action->Subsystem = Target;
	Action->PC = InPlayerController;
	Action->FileName = InFileName;

	return Action;
}


void UAsyncAction_ReadTitleFile::Activate()
{
	if (Subsystem.IsValid() && IsRegistered())
	{
		auto NewDelegate{ FReadFileCompleteDelegate::CreateUObject(this, &ThisClass::HandleReadComplete) };

		if (Subsystem->ReadFile(PC.Get(), FileName, NewDelegate))
		{
			return;
		}
	}

	HandleFailure();
}

void UAsyncAction_ReadTitleFile::HandleFailure()
{
	if (ShouldBroadcastDelegates())
	{
		FOnlineServiceResult Result;
		Result.bWasSuccessful = false;
		Result.ErrorId = TEXT("Read File Failed");
		Result.ErrorText = NSLOCTEXT("GameOnlineCore", "ReadFileFailed", "Read File Failed");

		OnRead.Broadcast(TArray<uint8>(), Result);
	}

	SetReadyToDestroy();
}

void UAsyncAction_ReadTitleFile::HandleReadComplete(const TArray<uint8>& Data, FOnlineServiceResult Result)
{
	if (ShouldBroadcastDelegates())
	{
		OnRead.Broadcast(Data, Result);
	}

	SetReadyToDestroy();
}
