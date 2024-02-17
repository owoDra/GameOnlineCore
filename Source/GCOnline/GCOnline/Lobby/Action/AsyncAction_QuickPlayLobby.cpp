// Copyright (C) 2024 owoDra

#include "AsyncAction_QuickPlayLobby.h"

#include "OnlineLobbySubsystem.h"
#include "Type/OnlineLobbyResultTypes.h"

#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_QuickPlayLobby)


UAsyncAction_QuickPlayLobby* UAsyncAction_QuickPlayLobby::QuickPlayLobby(UOnlineLobbySubsystem* Target, APlayerController* PlayerController, ULobbySearchRequest* SearchRequest, ULobbyCreateRequest* CreateRequest)
{
	auto* Action{ NewObject<UAsyncAction_QuickPlayLobby>() };

	Action->RegisterWithGameInstance(Target);
	Action->Subsystem = Target;
	Action->PC = PlayerController;
	Action->SearchReq = SearchRequest;
	Action->CreateReq = CreateRequest;

	return Action;
}


void UAsyncAction_QuickPlayLobby::Activate()
{
	if (Subsystem.IsValid() && IsRegistered() && PC.IsValid() && SearchReq && CreateReq)
	{
		StepA1_SearchLobby();
	}
	else
	{
		HandleFailure();
	}
}


// [Step A] Search and choose lobby

void UAsyncAction_QuickPlayLobby::StepA1_SearchLobby()
{
	auto NewDelegate
	{
		FLobbySearchCompleteDelegate::CreateUObject(this, &ThisClass::StepA2_SelectLobby)
	};

	// Start Search

	if (!Subsystem->SearchLobby(PC.Get(), SearchReq, NewDelegate))
	{
		HandleFailure();
	}
}

void UAsyncAction_QuickPlayLobby::StepA2_SelectLobby(ULobbySearchRequest* SearchRequest, FOnlineServiceResult Result)
{
	if (Result.bWasSuccessful)
	{
		const auto ResultCount{ SearchRequest ? SearchRequest->Results.Num() : INDEX_NONE };

		// Handle Failure if INDEX_NONE

		if (ResultCount == INDEX_NONE)
		{
			HandleFailure();
		}

		// If an item exists in the search results, select the preferred lobby from it.

		else if (ResultCount > 0)
		{
			StepB1_JoinLobby(ChoosePreferredLobby(SearchRequest->Results));
		}

		// Create a new lobby if there is no preferred lobby in the search results.

		else
		{
			StepC1_CreateLobby();
		}
	}
	else
	{
		HandleFailureWithResult(Result);
	}
}

ULobbyResult* UAsyncAction_QuickPlayLobby::ChoosePreferredLobby(const TArray<ULobbyResult*>& Results)
{
	return Results[0];
}


// [Step B] Join preffered lobby

void UAsyncAction_QuickPlayLobby::StepB1_JoinLobby(ULobbyResult* PrefferedLobbyResult)
{
	// Join only if there is a Preferred Lobby

	if (PrefferedLobbyResult && Subsystem.IsValid() && PC.IsValid())
	{
		auto NewDelegate
		{
			FLobbyJoinCompleteDelegate::CreateUObject(this, &ThisClass::StepB2_CompleteJoin)
		};

		if (Subsystem->JoinLobby(PC.Get(), CreatePreferredJoinRequest(PrefferedLobbyResult), NewDelegate))
		{
			return;
		}
	}

	// Handle failure if LobbyResult is nullptr

	HandleFailure();
}

void UAsyncAction_QuickPlayLobby::StepB2_CompleteJoin(ULobbyJoinRequest* JoinRequest, FOnlineServiceResult Result)
{
	// Handle quick play success if successed

	if (Result.bWasSuccessful)
	{
		// Check is lobby result valid

		if (auto LobbyResult{ JoinRequest ? JoinRequest->LobbyToJoin : nullptr })
		{
			HandleSuccess(LobbyResult);
		}
		else
		{
			HandleFailure();
		}
	}

	// Handle failure if not successed

	else
	{
		HandleFailureWithResult(Result);
	}
}

ULobbyJoinRequest* UAsyncAction_QuickPlayLobby::CreatePreferredJoinRequest(ULobbyResult* PrefferedLobbyResult)
{
	auto* NewRequest{ Subsystem->CreateOnlineLobbyJoinRequest(PrefferedLobbyResult) };
	NewRequest->LocalName = CreateReq->LocalName;
	NewRequest->bPresenceEnabled = CreateReq->bPresenceEnabled;

	return NewRequest;
}


// [Step C] Create new lobby

void UAsyncAction_QuickPlayLobby::StepC1_CreateLobby()
{
	auto NewDelegate
	{
		FLobbyCreateCompleteDelegate::CreateUObject(this, &ThisClass::StepC2_CompleteCreate)
	};

	// Start Create

	if (!Subsystem->CreateLobby(PC.Get(), CreateReq, NewDelegate))
	{
		HandleFailure();
	}
}

void UAsyncAction_QuickPlayLobby::StepC2_CompleteCreate(ULobbyCreateRequest* CreateRequest, FOnlineServiceResult Result)
{
	// Handle quick play success if successed

	if (Result.bWasSuccessful)
	{
		// Check is lobby result valid

		if (auto LobbyResult{ CreateRequest ? CreateRequest->Result : nullptr })
		{
			HandleSuccess(LobbyResult);
		}
		else
		{
			HandleFailure();
		}
	}

	// Handle failure if not successed

	else
	{
		HandleFailureWithResult(Result);
	}
}


// Success

void UAsyncAction_QuickPlayLobby::HandleSuccess(ULobbyResult* LobbyResult)
{
	if (ensure(LobbyResult))
	{
		OnComplete.Broadcast(PC.Get(), LobbyResult, FOnlineServiceResult());
	}
	else
	{
		HandleFailure();
	}
}


// Failure

void UAsyncAction_QuickPlayLobby::HandleFailure()
{
	if (ShouldBroadcastDelegates())
	{
		FOnlineServiceResult Result;
		Result.bWasSuccessful = false;
		Result.ErrorId = TEXT("Unknown");
		Result.ErrorText = NSLOCTEXT("GameOnlineCore", "QuickPlayLobbyUnknownFailed", "Unknown Reason");

		OnComplete.Broadcast(PC.Get(), nullptr, Result);
	}

	SetReadyToDestroy();
}

void UAsyncAction_QuickPlayLobby::HandleFailureWithResult(const FOnlineServiceResult& Result)
{
	if (ShouldBroadcastDelegates())
	{
		OnComplete.Broadcast(PC.Get(), nullptr, Result);
	}

	SetReadyToDestroy();
}
