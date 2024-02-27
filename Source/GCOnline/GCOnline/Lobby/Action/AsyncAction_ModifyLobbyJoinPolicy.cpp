// Copyright (C) 2024 owoDra

#include "AsyncAction_ModifyLobbyJoinPolicy.h"

#include "OnlineLobbySubsystem.h"

#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_ModifyLobbyJoinPolicy)


UAsyncAction_ModifyLobbyJoinPolicy* UAsyncAction_ModifyLobbyJoinPolicy::ModifyLobbyJoinPolicy(UOnlineLobbySubsystem* Target, APlayerController* PlayerController, const ULobbyResult* LobbyResult, ELobbyJoinablePolicy NewPolicy)
{
	auto* Action{ NewObject<UAsyncAction_ModifyLobbyJoinPolicy>() };

	Action->RegisterWithGameInstance(Target);
	Action->Subsystem = Target;
	Action->PC = PlayerController;
	Action->Lobby = LobbyResult;
	Action->Policy = NewPolicy;

	return Action;
}


void UAsyncAction_ModifyLobbyJoinPolicy::Activate()
{
	if (Subsystem.IsValid() && IsRegistered())
	{
		auto NewDelegate{ FLobbyModifyCompleteDelegate::CreateUObject(this, &ThisClass::HandleModifyLobbyJoinPolicyComplete) };

		if (Subsystem->ModifyLobbyJoinPolicy(PC.Get(), Lobby.Get(), Policy, NewDelegate))
		{
			return;
		}
	}

	HandleFailure();
}

void UAsyncAction_ModifyLobbyJoinPolicy::HandleFailure()
{
	if (ShouldBroadcastDelegates())
	{
		FOnlineServiceResult Result;
		Result.bWasSuccessful = false;
		Result.ErrorId = TEXT("Modify Lobby Join Policy Failed");
		Result.ErrorText = NSLOCTEXT("GameOnlineCore", "ModifyLobbyJoinPolicyFailed", "Modify Lobby Join Policy Failed");

		OnComplete.Broadcast(PC.Get(), Lobby.Get(), Result);
	}

	SetReadyToDestroy();
}

void UAsyncAction_ModifyLobbyJoinPolicy::HandleModifyLobbyJoinPolicyComplete(const ULobbyResult* LobbyResult, FOnlineServiceResult Result)
{
	if (ShouldBroadcastDelegates())
	{
		OnComplete.Broadcast(PC.Get(), LobbyResult, Result);
	}

	SetReadyToDestroy();
}
