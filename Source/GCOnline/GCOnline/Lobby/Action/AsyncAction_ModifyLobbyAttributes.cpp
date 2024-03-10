// Copyright (C) 2024 owoDra

#include "AsyncAction_ModifyLobbyAttributes.h"

#include "OnlineLobbySubsystem.h"

#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_ModifyLobbyAttributes)


UAsyncAction_ModifyLobbyAttributes* UAsyncAction_ModifyLobbyAttributes::ModifyLobbyAttributes(UOnlineLobbySubsystem* Target, APlayerController* PlayerController, const ULobbyResult* LobbyResult, const TSet<FLobbyAttribute>& AttrToChange, const TSet<FLobbyAttribute>& AttrToRemove)
{
	auto* Action{ NewObject<UAsyncAction_ModifyLobbyAttributes>() };

	Action->RegisterWithGameInstance(Target);
	Action->Subsystem = Target;
	Action->PC = PlayerController;
	Action->Lobby = LobbyResult;
	Action->ToChange = AttrToChange;
	Action->ToRemove = AttrToRemove;

	return Action;
}


void UAsyncAction_ModifyLobbyAttributes::Activate()
{
	if (Subsystem.IsValid() && IsRegistered())
	{
		auto NewDelegate{ FLobbyModifyCompleteDelegate::CreateUObject(this, &ThisClass::HandleModifyLobbyAttributeComplete) };

		if (Subsystem->ModifyLobbyAttribute(PC.Get(), Lobby.Get(), ToChange, ToRemove, NewDelegate))
		{
			return;
		}
	}

	HandleFailure();
}

void UAsyncAction_ModifyLobbyAttributes::HandleFailure()
{
	if (ShouldBroadcastDelegates())
	{
		FOnlineServiceResult Result;
		Result.bWasSuccessful = false;
		Result.ErrorId = TEXT("Modify Lobby Attributes Failed");
		Result.ErrorText = NSLOCTEXT("GameOnlineCore", "ModifyLobbyAttributesFailed", "Modify Lobby Attributes Failed");

		OnComplete.Broadcast(PC.Get(), Lobby.Get(), Result);
	}

	SetReadyToDestroy();
}

void UAsyncAction_ModifyLobbyAttributes::HandleModifyLobbyAttributeComplete(const ULobbyResult* LobbyResult, FOnlineServiceResult Result)
{
	if (ShouldBroadcastDelegates())
	{
		OnComplete.Broadcast(PC.Get(), LobbyResult, Result);
	}

	SetReadyToDestroy();
}
