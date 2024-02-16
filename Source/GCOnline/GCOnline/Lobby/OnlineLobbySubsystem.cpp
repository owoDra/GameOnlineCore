// Copyright (C) 2024 owoDra

#include "OnlineLobbySubsystem.h"

#include "OnlineServiceSubsystem.h"
#include "GCOnlineLogs.h"

// OSS v2
#include "Online/OnlineResult.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineLobbySubsystem)


// Initialization

void UOnlineLobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	OnlineServiceSubsystem = Collection.InitializeDependency<UOnlineServiceSubsystem>();

	check(OnlineServiceSubsystem);

	BindLobbiesDelegates();
}

void UOnlineLobbySubsystem::Deinitialize()
{
	OnlineServiceSubsystem = nullptr;

	UnbindLobbiesDelegates();
}

bool UOnlineLobbySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}


void UOnlineLobbySubsystem::BindLobbiesDelegates()
{
}

void UOnlineLobbySubsystem::UnbindLobbiesDelegates()
{
}

ILobbiesPtr UOnlineLobbySubsystem::GetLobbiesInterface(EOnlineServiceContext Context) const
{
	auto OnlineService{ OnlineServiceSubsystem->GetContextCache() };

	if (ensure(OnlineService))
	{
		return OnlineService->GetLobbiesInterface();
	}

	return nullptr;
}


// Events

void UOnlineLobbySubsystem::NotifyUserLobbyRequest(const FPlatformUserId& PlatformUserId, ULobbySearchResult* RequestedLobby, const FOnlineServiceResult& RequestedLobbyResult)
{
	OnUserJoinLobbyRequestEvent.Broadcast(PlatformUserId, RequestedLobby, RequestedLobbyResult);
	K2_OnUserJoinLobbyRequestEvent.Broadcast(PlatformUserId, RequestedLobby, RequestedLobbyResult);
}

void UOnlineLobbySubsystem::NotifyJoinLobbyComplete(const FOnlineServiceResult& Result)
{
	OnJoinLobbyCompleteEvent.Broadcast(Result);
	K2_OnJoinLobbyCompleteEvent.Broadcast(Result);
}


// Create Lobby

ULobbyCreateRequest* UOnlineLobbySubsystem::CreateOnlineLobbyCreateRequest()
{
	auto* NewRequest{ NewObject<ULobbyCreateRequest>(this) };
	NewRequest->OnlineMode = ELobbyOnlineMode::Online;

	return NewRequest;
}

bool UOnlineLobbySubsystem::CreateLobby(APlayerController* HostingPlayer, ULobbyCreateRequest* HostRequest, FLobbyCreateCompleteSingleDelegate Delegate)
{
	if (!HostRequest)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Create Lobby failed: passed an invalid request."));
		return false;
	}

	auto* LocalPlayer{ HostingPlayer ? HostingPlayer->GetLocalPlayer() : nullptr };
	if (!LocalPlayer && !bIsDedicatedServer)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Create Lobby failed: HostingPlayer is invalid."));
		return false;
	}

	FString OutError;
	if (!HostRequest->ValidateAndLogErrors(OutError))
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Create Lobby failed: %s"), *OutError);
		return false;
	}

	CreateOnlineLobbyInternal(LocalPlayer, HostRequest, Delegate);
	return true;
}

void UOnlineLobbySubsystem::CreateOnlineLobbyInternal(ULocalPlayer* LocalPlayer, ULobbyCreateRequest* HostRequest, FLobbyCreateCompleteSingleDelegate Delegate)
{
	check(HostRequest);

	auto LobbiesInterface{ GetLobbiesInterface() };
	check(LobbiesInterface);

	// Make lobby creation parameters

	auto CreateParams{ HostRequest->GenerateCreationParameters() };

	if (LocalPlayer)
	{
		CreateParams.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();
	}
	else if (bIsDedicatedServer)
	{
		/// @TODO what should this do for v2?
	}

	///@ TODO: Add splitscreen players

	// Set Pending Travel URL

	SetPendingLobbyTravelURL(HostRequest->ConstructTravelURL());

	// Start Create Lobby

	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("Start Create New Lobby"));

	auto Handle{ LobbiesInterface->CreateLobby(MoveTemp(CreateParams)) };
	Handle.OnComplete(this, &ThisClass::HandleCreateOnlineLobbyComplete, Delegate);
}

void UOnlineLobbySubsystem::HandleCreateOnlineLobbyComplete(const TOnlineResult<FCreateLobby>& CreateResult, FLobbyCreateCompleteSingleDelegate Delegate)
{
	const auto bSuccess{ CreateResult.IsOk() };
	const auto NewLobby{ bSuccess ? CreateResult.GetOkValue().Lobby : nullptr };

	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("Create Lobby Completed"));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| Result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| Error: %s"), bSuccess ? TEXT("") : *CreateResult.GetErrorValue().GetLogString());
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| LobbyId: %s"), *ToLogString(NewLobby ? NewLobby->LobbyId : FLobbyId()));

	FOnlineServiceResult ServiceResult;

	if (!bSuccess)
	{
		ServiceResult = FOnlineServiceResult(CreateResult.GetErrorValue());

		ClearPendingLobbyTravelURL();
	}

	Delegate.ExecuteIfBound(ServiceResult);
	NotifyCreateLobbyComplete(ServiceResult);
}

void UOnlineLobbySubsystem::NotifyCreateLobbyComplete(const FOnlineServiceResult& Result)
{
	OnCreateLobbyCompleteEvent.Broadcast(Result);
	K2_OnCreateLobbyCompleteEvent.Broadcast(Result);
}




// Lobby Travel

bool UOnlineLobbySubsystem::StartPendingLobbyTravel()
{
	if (PendingLobbyTravelURL.IsEmpty())
	{
		UE_LOG(LogGameCore_OnlineLobbies, Warning, TEXT("Lobby Travel Failed: No pending travel URL"));
		return false;
	}

	return true;
}

void UOnlineLobbySubsystem::SetPendingLobbyTravelURL(const FString& URL)
{
	PendingLobbyTravelURL = URL;
}

void UOnlineLobbySubsystem::ClearPendingLobbyTravelURL()
{
	PendingLobbyTravelURL.Empty();
}


// Lobby

USearchLobbyRequest* UOnlineLobbySubsystem::CreateOnlineSearchLobbyRequest()
{
	auto* NewRequest{ NewObject<USearchLobbyRequest>(this) };
	NewRequest->OnlineMode = ELobbyOnlineMode::Online;

	return NewRequest;
}

void UOnlineLobbySubsystem::QuickPlayLobby(APlayerController* JoiningOrHostingPlayer, USearchLobbyRequest* SearchRequest, UHostLobbyRequest* HostRequest)
{
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("QuickPlay Requested"));

	if (!HostRequest || !SearchRequest)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("QuickPlaySession passed a null request"));
		return;
	}

	auto HostRequestPtr{ TStrongObjectPtr<UHostLobbyRequest>(HostRequest) };
	auto JoiningOrHostingPlayerPtr{ TWeakObjectPtr<APlayerController>(JoiningOrHostingPlayer) };

	SearchRequest->OnSearchFinished.AddUObject(this, &ThisClass::HandleQuickPlaySearchFinished, JoiningOrHostingPlayerPtr, HostRequestPtr);

	FindLobbiesInternal(JoiningOrHostingPlayer, MakeShared<FLobbySearchSettings>(SearchRequest));
}

void UOnlineLobbySubsystem::JoinLobby(APlayerController* JoiningPlayer, USearchLobbyResult* SearchResult)
{
	if (SearchResult == nullptr)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("JoinLobby passed a null request"));
		return;
	}

	auto* LocalPlayer{ JoiningPlayer ? JoiningPlayer->GetLocalPlayer() : nullptr };
	if (!LocalPlayer)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Joining Player is invalid"));
		return;
	}

	JoinSessionInternal(LocalPlayer, SearchResult);
}

void UOnlineLobbySubsystem::FindLobbies(APlayerController* SearchingPlayer, USearchLobbyRequest* SearchRequest)
{
	if (SearchRequest == nullptr)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("FindLobbies passed a null request"));
		return;
	}

	FindLobbiesInternal(SearchingPlayer, MakeShared<FLobbySearchSettings>(SearchRequest));
}

void UOnlineLobbySubsystem::CleanUpLobbies()
{
	bWantToDestroyPendingLobby = true;

	auto LobbiesInterface{ GetOnlineLobbies() };
	check(LobbiesInterface);

	auto LocalPlayerId{ GetAccountId(GetGameInstance()->GetFirstLocalPlayerController()) };
	auto LobbyId{ GetLobbyId(NAME_GameSession, LocalPlayerId) };

	if (!LocalPlayerId.IsValid() || !LobbyId.IsValid())
	{
		return;
	}
	
	LobbiesInterface->LeaveLobby({ LocalPlayerId, LobbyId });
}


// Create Lobby

void UOnlineLobbySubsystem::CreateOnlineLobbyInternal(ULocalPlayer* LocalPlayer, UHostLobbyRequest* HostRequest)
{
	CreateLobbyResult = FOnlineResultInformation();
	PendingTravelURL = HostRequest->ConstructTravelURL();

	const auto LobbyName{ NAME_GameSession };
	const auto MaxPlayers{ HostRequest->GetMaxPlayers() };

	auto LobbirsInterface{ GetOnlineLobbies() };
	check(LobbirsInterface);

	FCreateLobby::Params CreateParams;

	if (LocalPlayer)
	{
		CreateParams.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();
	}
	else if (bIsDedicatedServer)
	{
		/// @TODO what should this do for v2?
	}

	CreateParams.LocalName			= LobbyName;
	CreateParams.SchemaId			= FSchemaId(TEXT("GameLobby"));
	CreateParams.bPresenceEnabled	= true;
	CreateParams.MaxMembers			= MaxPlayers;
	CreateParams.JoinPolicy			= ELobbyJoinPolicy::PublicAdvertised;

	/// @TODO attribute support 

	//CreateParams.Attributes.Emplace(SETTING_GAMEMODE, HostRequest->ModeNameForAdvertisement);
	//CreateParams.Attributes.Emplace(SETTING_MAPNAME, HostRequest->GetMapName());
	//CreateParams.Attributes.Emplace(SETTING_MATCHING_TIMEOUT, 120.0f);
	//CreateParams.Attributes.Emplace(SETTING_SESSION_TEMPLATE_NAME, FString(TEXT("GameSession")));

	CreateParams.Attributes.Emplace(FName(TEXT("LOBBYSERVICEATTRIBUTE1")), true);
	CreateParams.Attributes.Emplace(FName(TEXT("LOBBYSERVICEATTRIBUTE2")), HostRequest->ModeNameForAdvertisement);
	CreateParams.Attributes.Emplace(FName(TEXT("LOBBYSERVICEATTRIBUTE3")), HostRequest->GetMapName());
	CreateParams.Attributes.Emplace(FName(TEXT("LOBBYSERVICEATTRIBUTE4")), 120.0f);
	CreateParams.Attributes.Emplace(FName(TEXT("LOBBYSERVICEATTRIBUTE5")), FString(TEXT("GameSession")));

	// Add presence setting so it can be searched for

	//CreateParams.Attributes.Emplace(SEARCH_PRESENCE, true);

	//CreateParams.UserAttributes.Emplace(SETTING_GAMEMODE, FString(TEXT("GameSession")));

	//CreateParams.UserAttributes.Emplace(FName(TEXT("LOBBYSERVICEATTRIBUTE2")), FString(TEXT("GameSession")));

	///@ TODO: Add splitscreen players

	LobbirsInterface->CreateLobby(MoveTemp(CreateParams)).OnComplete(this, 
		[this, LobbyName](const TOnlineResult<FCreateLobby>& CreateResult)
		{
			OnCreateLobbyComplete(LobbyName, CreateResult.IsOk());
		}
	);
}

void UOnlineLobbySubsystem::OnCreateLobbyComplete(FName LobbyName, bool bWasSuccessful)
{
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("OnCreateLobbyComplete(LobbyName: %s, bWasSuccessful: %d)"), *LobbyName.ToString(), bWasSuccessful);

	FinishLobbyCreation(bWasSuccessful);
}

void UOnlineLobbySubsystem::FinishLobbyCreation(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		CreateLobbyResult = FOnlineResultInformation();
		CreateLobbyResult.bWasSuccessful = true;

		NotifyCreateLobbyComplete(CreateLobbyResult);

		// Travel to the specified match URL

		GetWorld()->ServerTravel(PendingTravelURL);
	}
	else
	{
		if (CreateLobbyResult.bWasSuccessful || CreateLobbyResult.ErrorText.IsEmpty())
		{
			FString ReturnError = TEXT("GenericFailure"); /// @TODO: No good way to get session error codes out of OSSV1
			FText ReturnReason = NSLOCTEXT("GameOnlineCore", "CreateSessionFailed", "Failed to create session.");

			CreateLobbyResult.bWasSuccessful = false;
			CreateLobbyResult.ErrorId = ReturnError;
			CreateLobbyResult.ErrorText = ReturnReason;
		}

		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("FinishLobbyCreation(%s): %s"), *CreateLobbyResult.ErrorId, *CreateLobbyResult.ErrorText.ToString());

		NotifyCreateLobbyComplete(CreateLobbyResult);
	}
}

void UOnlineLobbySubsystem::SetCreateLobbyError(const FText& ErrorText)
{
	CreateLobbyResult.bWasSuccessful = false;
	CreateLobbyResult.ErrorId = TEXT("InternalFailure");
	CreateLobbyResult.ErrorText = ErrorText;
}


// Quick Play Lobby

void UOnlineLobbySubsystem::HandleQuickPlaySearchFinished(bool bSucceeded, const FText& ErrorMessage, TWeakObjectPtr<APlayerController> JoiningOrHostingPlayer, TStrongObjectPtr<UHostLobbyRequest> HostRequest)
{
	const auto ResultCount{ SearchSettings->SearchRequest->Results.Num() };

	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("QuickPlay Search Finished %s (Results %d) (Error: %s)"), bSucceeded ? TEXT("Success") : TEXT("Failed"), ResultCount, *ErrorMessage.ToString());

	/// @TODO: We have to check if the error message is empty because some OSS layers report a failure just because there are no sessions.  Please fix with OSS 2.0.
	
	if (bSucceeded || ErrorMessage.IsEmpty())
	{
		// Join the best search result.

		if (ResultCount > 0)
		{
			/// @TODO: We should probably look at ping?  maybe some other factors to find the best.  Idk if they come pre-sorted or not.

			for (auto Result : SearchSettings->SearchRequest->Results)
			{
				JoinLobby(JoiningOrHostingPlayer.Get(), Result);
				return;
			}
		}
		else
		{
			HostLobby(JoiningOrHostingPlayer.Get(), HostRequest.Get());
		}
	}
	else
	{
		/// @TODO: This sucks, need to tell someone.
	}
}


// Join Lobby

void UOnlineLobbySubsystem::JoinSessionInternal(ULocalPlayer* LocalPlayer, USearchLobbyResult* Request)
{
	const auto SessionName{ NAME_GameSession };

	auto LobbiesInterface{ GetOnlineLobbies() };
	check(LobbiesInterface);

	FJoinLobby::Params JoinParams;
	JoinParams.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();
	JoinParams.LocalName = SessionName;
	JoinParams.LobbyId = Request->Lobby->LobbyId;
	JoinParams.bPresenceEnabled = true;

	// Add any splitscreen players if they exist 
	/// @TODO: See UCommonSessionSubsystem::OnJoinSessionComplete

	LobbiesInterface->JoinLobby(MoveTemp(JoinParams)).OnComplete(this, 
		[this, SessionName](const TOnlineResult<FJoinLobby>& JoinResult)
		{
			if (JoinResult.IsOk())
			{
				InternalTravelToLobby(SessionName);
			}
			else
			{
				/// @TODO: Error handling
				UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("JoinLobby Failed with Result: %s"), *ToLogString(JoinResult.GetErrorValue()));
			}
		}
	);
}


// Find Lobby

void UOnlineLobbySubsystem::FindLobbiesInternal(APlayerController* SearchingPlayer, const TSharedRef<FLobbySearchSettings>& InSearchSettings)
{
	if (SearchSettings.IsValid())
	{
		//@TODO: This is a poor user experience for the API user, we should let the additional search piggyback and
		// just give it the same results as the currently pending one
		// (or enqueue the request and service it when the previous one finishes or fails)

		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("A previous FindLobbies call is still in progress, aborting"));
		SearchSettings->SearchRequest->NotifySearchFinished(false, NSLOCTEXT("GameOnlineCore", "Error_FindLobbiesAlreadyInProgress", "Lobbies search already in progress"));
	}

	auto* LocalPlayer{ SearchingPlayer ? SearchingPlayer->GetLocalPlayer() : nullptr };
	if (LocalPlayer == nullptr)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("SearchingPlayer is invalid"));
		InSearchSettings->SearchRequest->NotifySearchFinished(false, NSLOCTEXT("GameOnlineCore", "Error_FindLobbiesBadPlayer", "Lobbies search was not provided a local player"));
		return;
	}

	SearchSettings = InSearchSettings;

	auto LobbiesInterface{ GetOnlineLobbies() };
	check(LobbiesInterface);

	auto FindLobbyParams{ InSearchSettings->FindLobbyParams };
	FindLobbyParams.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();

	LobbiesInterface->FindLobbies(MoveTemp(FindLobbyParams)).OnComplete(this, 
		[this, LocalSearchSettings = SearchSettings](const TOnlineResult<FFindLobbies>& FindResult)
		{
			// This was an abandoned search, ignore

			if (LocalSearchSettings != SearchSettings)
			{
				return;
			}

			const auto bWasSuccessful{ FindResult.IsOk() };
			UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("FindLobbies(bWasSuccessful: %s)"), *LexToString(bWasSuccessful));
			check(SearchSettings.IsValid());
			if (bWasSuccessful)
			{
				const auto& FindResults{ FindResult.GetOkValue() };
				SearchSettings->SearchRequest->Results.Reset(FindResults.Lobbies.Num());

				for (const auto& Lobby : FindResults.Lobbies)
				{
					if (!Lobby->OwnerAccountId.IsValid())
					{
						UE_LOG(LogGameCore_OnlineLobbies, Verbose, TEXT("\tIgnoring Lobby with no owner (LobbyId: %s)"), *ToLogString(Lobby->LobbyId));
					}
					else if (Lobby->Members.Num() == 0)
					{
						UE_LOG(LogGameCore_OnlineLobbies, Verbose, TEXT("\tIgnoring Lobby with no members (UserId: %s)"), *ToLogString(Lobby->OwnerAccountId));
					}
					else
					{
						auto* Entry{ NewObject<USearchLobbyResult>(SearchSettings->SearchRequest) };
						Entry->Lobby = Lobby;
						SearchSettings->SearchRequest->Results.Add(Entry);

						UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("\tFound lobby (UserId: %s, NumOpenConns: %d)"), *ToLogString(Lobby->OwnerAccountId), Lobby->MaxMembers - Lobby->Members.Num());
					}
				}
			}
			else
			{
				SearchSettings->SearchRequest->Results.Empty();
			}

			const auto ResultText{ bWasSuccessful ? FText() : FindResult.GetErrorValue().GetText() };

			SearchSettings->SearchRequest->NotifySearchFinished(bWasSuccessful, ResultText);
			SearchSettings.Reset();
		}
	);
}


// Travel Lobby

void UOnlineLobbySubsystem::InternalTravelToLobby(const FName LobbyName)
{
	/// @TODO: Ideally we'd use triggering player instead of first (they're all gonna go at once so it probably doesn't matter)

	auto* const PlayerController{ GetGameInstance()->GetFirstLocalPlayerController() };
	if (!PlayerController)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("InternalTravelToSession(Failed due to Invalid Player Controller)"));
		return;
	}

	FString URL;

	auto OnlineServices{ GetOnlineService() };
	check(OnlineServices);

	auto LocalUserId{ GetAccountId(PlayerController) };
	if (LocalUserId.IsValid())
	{
		auto Result{ OnlineServices->GetResolvedConnectString({ LocalUserId, GetLobbyId(LobbyName) }) };

		if (ensure(Result.IsOk()))
		{
			URL = Result.GetOkValue().ResolvedConnectString;
		}
	}

	// Allow modification of the URL prior to travel

	OnPreClientTravelEvent.Broadcast(URL);

	PlayerController->ClientTravel(URL, TRAVEL_Absolute);
}


// Utilities

FAccountId UOnlineLobbySubsystem::GetAccountId(APlayerController* PlayerController) const
{
	if (const auto* LocalPlayer{ PlayerController->GetLocalPlayer() })
	{
		auto LocalPlayerIdRepl{ LocalPlayer->GetPreferredUniqueNetId() };

		if (LocalPlayerIdRepl.IsValid())
		{
			return LocalPlayerIdRepl.GetV2();
		}
	}

	return FAccountId();
}

FLobbyId UOnlineLobbySubsystem::GetLobbyId(const FName LobbyName) const
{
	auto LocalUserId{ GetAccountId(GetGameInstance()->GetFirstLocalPlayerController()) };

	return GetLobbyId(LobbyName, LocalUserId);
}

FLobbyId UOnlineLobbySubsystem::GetLobbyId(const FName& LobbyName, const FAccountId& AccountId) const
{
	if (AccountId.IsValid())
	{
		auto LobbiesInterface{ GetOnlineLobbies() };
		check(LobbiesInterface);

		auto JoinedLobbies{ LobbiesInterface->GetJoinedLobbies({ AccountId }) };
		if (JoinedLobbies.IsOk())
		{
			for (const auto& Lobby : JoinedLobbies.GetOkValue().Lobbies)
			{
				if (Lobby->LocalName == LobbyName)
				{
					return Lobby->LobbyId;
				}
			}
		}
	}

	return FLobbyId();
}
