// Copyright (C) 2024 owoDra

#include "AsyncAction_JoinLobby.h"

#include "OnlineLobbySubsystem.h"

#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_JoinLobby)


UAsyncAction_JoinLobby* UAsyncAction_JoinLobby::JoinLobby(UOnlineLobbySubsystem* Target, APlayerController* PlayerController, ULobbyJoinRequest* JoinRequest)
{
	auto* Action{ NewObject<UAsyncAction_JoinLobby>() };

	Action->RegisterWithGameInstance(Target);
	Action->Subsystem = Target;
	Action->PC = PlayerController;
	Action->Request = JoinRequest;

	return Action;
}


void UAsyncAction_JoinLobby::Activate()
{
	if (Subsystem.IsValid() && IsRegistered())
	{
		auto NewDelegate{ FLobbyJoinCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinLobbyComplete) };

		if (Subsystem->JoinLobby(PC.Get(), Request.Get(), NewDelegate))
		{
			return;
		}
	}

	HandleFailure();
}

void UAsyncAction_JoinLobby::HandleFailure()
{
	if (ShouldBroadcastDelegates())
	{
		FOnlineServiceResult Result;
		Result.bWasSuccessful = false;
		Result.ErrorId = TEXT("Join Lobby Failed");
		Result.ErrorText = NSLOCTEXT("GameOnlineCore", "JoinLobbyFailed", "Join Lobby Failed");

		OnComplete.Broadcast(PC.Get(), Request.Get(), Result);
	}

	SetReadyToDestroy();
}

void UAsyncAction_JoinLobby::HandleJoinLobbyComplete(ULobbyJoinRequest* JoinRequest, FOnlineServiceResult Result)
{
	if (ShouldBroadcastDelegates())
	{
		OnComplete.Broadcast(PC.Get(), JoinRequest, Result);
	}

	SetReadyToDestroy();
}
