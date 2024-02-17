// Copyright (C) 2024 owoDra

#include "AsyncAction_CreateLobby.h"

#include "OnlineLobbySubsystem.h"

#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_CreateLobby)


UAsyncAction_CreateLobby* UAsyncAction_CreateLobby::CreateLobby(UOnlineLobbySubsystem* Target, APlayerController* PlayerController, ULobbyCreateRequest* CreateRequest)
{
	auto* Action{ NewObject<UAsyncAction_CreateLobby>() };

	Action->RegisterWithGameInstance(Target);
	Action->Subsystem = Target;
	Action->PC = PlayerController;
	Action->Request = CreateRequest;

	return Action;
}


void UAsyncAction_CreateLobby::Activate()
{
	if (Subsystem.IsValid() && IsRegistered())
	{
		auto NewDelegate{ FLobbyCreateCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateLobbyComplete) };

		if (Subsystem->CreateLobby(PC.Get(), Request.Get(), NewDelegate))
		{
			return;
		}
	}

	HandleFailure();
}

void UAsyncAction_CreateLobby::HandleFailure()
{
	if (ShouldBroadcastDelegates())
	{
		FOnlineServiceResult Result;
		Result.bWasSuccessful = false;
		Result.ErrorId = TEXT("Create Lobby Failed");
		Result.ErrorText = NSLOCTEXT("GameOnlineCore", "CreateLobbyFailed", "Create Lobby Failed");

		OnComplete.Broadcast(PC.Get(), Request.Get(), Result);
	}

	SetReadyToDestroy();
}

void UAsyncAction_CreateLobby::HandleCreateLobbyComplete(ULobbyCreateRequest* CreateRequest, FOnlineServiceResult Result)
{
	if (ShouldBroadcastDelegates())
	{
		OnComplete.Broadcast(PC.Get(), CreateRequest, Result);
	}

	SetReadyToDestroy();
}
