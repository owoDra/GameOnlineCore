// Copyright (C) 2024 owoDra

#include "AsyncAction_LeaveLobby.h"

#include "OnlineLobbySubsystem.h"

#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_LeaveLobby)


UAsyncAction_LeaveLobby* UAsyncAction_LeaveLobby::LeaveLobby(UOnlineLobbySubsystem* Target, APlayerController* PlayerController, FName InLocalName)
{
	auto* Action{ NewObject<UAsyncAction_LeaveLobby>() };

	Action->RegisterWithGameInstance(Target);
	Action->Subsystem = Target;
	Action->PC = PlayerController;
	Action->LocalName = InLocalName;

	return Action;
}


void UAsyncAction_LeaveLobby::Activate()
{
	if (Subsystem.IsValid() && IsRegistered())
	{
		auto NewDelegate{ FLobbyLeaveCompleteDelegate::CreateUObject(this, &ThisClass::HandleLeaveLobbyComplete) };

		if (Subsystem->CleanUpLobby(LocalName, PC.Get(), NewDelegate))
		{
			return;
		}
	}

	HandleFailure();
}

void UAsyncAction_LeaveLobby::HandleFailure()
{
	if (ShouldBroadcastDelegates())
	{
		FOnlineServiceResult Result;
		Result.bWasSuccessful = false;
		Result.ErrorId = TEXT("Leave Lobby Failed");
		Result.ErrorText = NSLOCTEXT("GameOnlineCore", "LeaveLobbyFailed", "Leave Lobby Failed");

		OnComplete.Broadcast(PC.Get(), Result);
	}

	SetReadyToDestroy();
}

void UAsyncAction_LeaveLobby::HandleLeaveLobbyComplete(FOnlineServiceResult Result)
{
	if (ShouldBroadcastDelegates())
	{
		OnComplete.Broadcast(PC.Get(), Result);
	}

	SetReadyToDestroy();
}
