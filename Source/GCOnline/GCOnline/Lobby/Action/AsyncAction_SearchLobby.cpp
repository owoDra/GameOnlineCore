// Copyright (C) 2024 owoDra

#include "AsyncAction_SearchLobby.h"

#include "OnlineLobbySubsystem.h"

#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_SearchLobby)


UAsyncAction_SearchLobby* UAsyncAction_SearchLobby::SearchLobby(UOnlineLobbySubsystem* Target, APlayerController* PlayerController, ULobbySearchRequest* SearchRequest)
{
	auto* Action{ NewObject<UAsyncAction_SearchLobby>() };

	Action->RegisterWithGameInstance(Target);
	Action->Subsystem = Target;
	Action->PC = PlayerController;
	Action->Request = SearchRequest;

	return Action;
}


void UAsyncAction_SearchLobby::Activate()
{
	if (Subsystem.IsValid() && IsRegistered())
	{
		auto NewDelegate{ FLobbySearchCompleteDelegate::CreateUObject(this, &ThisClass::HandleSearchLobbyComplete) };

		if (Subsystem->SearchLobby(PC.Get(), Request.Get(), NewDelegate))
		{
			return;
		}
	}

	HandleFailure();
}

void UAsyncAction_SearchLobby::HandleFailure()
{
	if (ShouldBroadcastDelegates())
	{
		FOnlineServiceResult Result;
		Result.bWasSuccessful = false;
		Result.ErrorId = TEXT("Search Lobby Failed");
		Result.ErrorText = NSLOCTEXT("GameOnlineCore", "SearchLobbyFailed", "Search Lobby Failed");

		OnComplete.Broadcast(PC.Get(), Request.Get(), Result);
	}

	SetReadyToDestroy();
}

void UAsyncAction_SearchLobby::HandleSearchLobbyComplete(ULobbySearchRequest* SearchRequest, FOnlineServiceResult Result)
{
	if (ShouldBroadcastDelegates())
	{
		OnComplete.Broadcast(PC.Get(), SearchRequest, Result);
	}

	SetReadyToDestroy();
}
