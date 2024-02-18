// Copyright (C) 2024 owoDra

#include "OnlineLobbySubsystem.h"

#include "Type/OnlineLobbyResultTypes.h"
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
	if (auto LobbiesInterface{ GetLobbiesInterface() })
	{
		LobbyDelegateHandles.Emplace(LobbiesInterface->OnUILobbyJoinRequested().Add(this, &ThisClass::HandleUserJoinLobbyRequest));
		LobbyDelegateHandles.Emplace(LobbiesInterface->OnLobbyMemberJoined().Add(this, &ThisClass::HandleLobbyMemberJoined));
		LobbyDelegateHandles.Emplace(LobbiesInterface->OnLobbyMemberLeft().Add(this, &ThisClass::HandleLobbyMemberLeft));

		/// @TODO Support Leader change and host migration
		// LobbyDelegateHandles.Emplace(LobbiesInterface->OnLobbyLeaderChanged().Add(this, &ThisClass::HandleLobbyMemberLeft));
	}
}

void UOnlineLobbySubsystem::UnbindLobbiesDelegates()
{
	for (auto& Handle : LobbyDelegateHandles)
	{
		Handle.Unbind();
	}

	LobbyDelegateHandles.Reset();
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


// Lobby Events

void UOnlineLobbySubsystem::HandleUserJoinLobbyRequest(const FUILobbyJoinRequested& EventParams)
{
	check(OnlineServiceSubsystem);

	auto OnlineService{ OnlineServiceSubsystem->GetContextCache() };
	check(OnlineService);

	auto AuthInterface{ OnlineService->GetAuthInterface() };
	check(AuthInterface);

	auto Account{ AuthInterface->GetLocalOnlineUserByOnlineAccountId({ EventParams.LocalAccountId }) };
	if (Account.IsOk())
	{
		auto PlatformUserId{ Account.GetOkValue().AccountInfo->PlatformUserId };

		FOnlineServiceResult ServiceResult;
		ULobbyResult* RequestedLobbyResult{ nullptr };

		if (EventParams.Result.IsOk())
		{
			RequestedLobbyResult = NewObject<ULobbyResult>(this);
			RequestedLobbyResult->InitializeResult(EventParams.Result.GetOkValue());
		}
		else
		{
			ServiceResult = FOnlineServiceResult(EventParams.Result.GetErrorValue());
		}

		NotifyUserJoinLobbyRequest(PlatformUserId, RequestedLobbyResult, ServiceResult);
	}
	else
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("HandleUserJoinLobbyRequest: Failed to get account by local user id (%s)"), *UE::Online::ToLogString(EventParams.LocalAccountId));
	}
}

void UOnlineLobbySubsystem::HandleLobbyMemberJoined(const FLobbyMemberJoined& EventParams)
{
	if (!EventParams.Member->bIsLocalMember)
	{
		const auto LocalName{ EventParams.Lobby->LocalName };
		const auto CurrentMembers{ EventParams.Lobby->Members.Num() };
		const auto MaxMembers{ EventParams.Lobby->MaxMembers };

		NotifyLobbyMemberChanged(LocalName, CurrentMembers, MaxMembers);
	}
}

void UOnlineLobbySubsystem::HandleLobbyMemberLeft(const FLobbyMemberLeft& EventParams)
{
	const auto LocalName{ EventParams.Lobby->LocalName };
	const auto CurrentMembers{ EventParams.Lobby->Members.Num() };
	const auto MaxMembers{ EventParams.Lobby->MaxMembers };

	NotifyLobbyMemberChanged(LocalName, CurrentMembers, MaxMembers);
}


// Create Lobby

ULobbyCreateRequest* UOnlineLobbySubsystem::CreateOnlineLobbyCreateRequest()
{
	auto* NewRequest{ NewObject<ULobbyCreateRequest>(this) };
	NewRequest->OnlineMode = ELobbyOnlineMode::Online;

	return NewRequest;
}

bool UOnlineLobbySubsystem::CreateLobby(APlayerController* HostingPlayer, ULobbyCreateRequest* CreateRequest, FLobbyCreateCompleteDelegate Delegate)
{
	if (!CreateRequest)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Create Lobby failed: passed an invalid request."));
		return false;
	}

	if (OngoingCreateRequest)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Warning, TEXT("Create Lobby failed: A request already in progress exists."));
		return false;
	}

	auto* LocalPlayer{ HostingPlayer ? HostingPlayer->GetLocalPlayer() : nullptr };
	if (!LocalPlayer && !bIsDedicatedServer)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Create Lobby failed: HostingPlayer is invalid."));
		return false;
	}

	FString OutError;
	if (!CreateRequest->ValidateAndLogErrors(OutError))
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Create Lobby failed: %s"), *OutError);
		return false;
	}

	CreateOnlineLobbyInternal(LocalPlayer, CreateRequest, Delegate);
	return true;
}

void UOnlineLobbySubsystem::CreateOnlineLobbyInternal(ULocalPlayer* LocalPlayer, ULobbyCreateRequest* CreateRequest, FLobbyCreateCompleteDelegate Delegate)
{
	check(CreateRequest);
	ensure(Delegate.IsBound());
	ensure(!OngoingCreateRequest);

	auto LobbiesInterface{ GetLobbiesInterface() };
	check(LobbiesInterface);

	// Make lobby creation parameters

	auto CreateParams{ CreateRequest->GenerateCreationParameters() };

	if (LocalPlayer)
	{
		CreateParams.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();
	}
	else if (bIsDedicatedServer)
	{
		/// @TODO what should this do for v2?
	}

	///@ TODO: Add splitscreen players

	// Set ongoing request

	OngoingCreateRequest = CreateRequest;

	// Start Create Lobby

	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("Start Create New Lobby"));

	auto Handle{ LobbiesInterface->CreateLobby(MoveTemp(CreateParams)) };
	Handle.OnComplete(this, &ThisClass::HandleCreateOnlineLobbyComplete, Delegate);
}

void UOnlineLobbySubsystem::HandleCreateOnlineLobbyComplete(const TOnlineResult<FCreateLobby>& CreateResult, FLobbyCreateCompleteDelegate Delegate)
{
	if (!ensure(OngoingCreateRequest))
	{
		return;
	}

	const auto bSuccess{ CreateResult.IsOk() };
	const auto NewLobby{ bSuccess ? CreateResult.GetOkValue().Lobby : nullptr };

	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("Create Lobby Completed"));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| Result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| Error: %s"), bSuccess ? TEXT("") : *CreateResult.GetErrorValue().GetLogString());
	
	FOnlineServiceResult ServiceResult;

	if (bSuccess)
	{
		auto* NewResult{ NewObject<ULobbyResult>(this) };
		NewResult->InitializeResult(NewLobby);

		UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| LobbyId: %s"), *ToLogString(NewLobby ? NewLobby->LobbyId : FLobbyId()));

		const auto TravelURL{ OngoingCreateRequest->ConstructTravelURL() };
		UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| URL: %s"), *TravelURL);

		NewResult->SetLobbyTravelURL(TravelURL);
		AddJoiningLobby(NewResult);

		OngoingCreateRequest->Result = NewResult;
	}
	else
	{
		ServiceResult = FOnlineServiceResult(CreateResult.GetErrorValue());

		OngoingCreateRequest->Result = nullptr;
	}

	ensure(Delegate.IsBound());
	Delegate.ExecuteIfBound(OngoingCreateRequest, ServiceResult);

	OngoingCreateRequest = nullptr;
}


// Search Lobby

ULobbySearchRequest* UOnlineLobbySubsystem::CreateOnlineLobbySearchRequest()
{
	auto* NewRequest{ NewObject<ULobbySearchRequest>(this) };
	return NewRequest;
}

bool UOnlineLobbySubsystem::SearchLobby(APlayerController* SearchingPlayer, ULobbySearchRequest* SearchRequest, FLobbySearchCompleteDelegate Delegate)
{
	if (!SearchRequest)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Search Lobby Failed: Invalid Request"));
		return false;
	}

	if (OngoingSearchRequest)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Warning, TEXT("Search Lobby failed: A request already in progress exists."));
		return false;
	}

	auto* LocalPlayer{ SearchingPlayer ? SearchingPlayer->GetLocalPlayer() : nullptr };
	if (!LocalPlayer)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Search Lobby Failed: HostingPlayer is invalid."));
		return false;
	}

	SearchOnlineLobbyInternal(LocalPlayer, SearchRequest, Delegate);
	return true;
}

void UOnlineLobbySubsystem::SearchOnlineLobbyInternal(ULocalPlayer* LocalPlayer, ULobbySearchRequest* SearchRequest, FLobbySearchCompleteDelegate Delegate)
{
	check(LocalPlayer);
	check(SearchRequest);
	ensure(Delegate.IsBound());
	ensure(!OngoingSearchRequest);

	auto LobbiesInterface{ GetLobbiesInterface() };
	check(LobbiesInterface);

	// Set Ongoing request

	OngoingSearchRequest = SearchRequest;

	// Make lobby search parameters

	auto FindLobbyParams{ SearchRequest->GenerateFindParameters() };
	FindLobbyParams.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();

	// Start lobby search

	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("Start Search Lobbies"));

	auto Handle{ LobbiesInterface->FindLobbies(MoveTemp(FindLobbyParams)) };
	Handle.OnComplete(this, &ThisClass::HandleSearchOnlineLobbyComplete, Delegate);
}

void UOnlineLobbySubsystem::HandleSearchOnlineLobbyComplete(const TOnlineResult<FFindLobbies>& SearchResult, FLobbySearchCompleteDelegate Delegate)
{
	if (!ensure(OngoingSearchRequest))
	{
		return;
	}

	const auto bSuccess{ SearchResult.IsOk() };
	const auto NewLobbies{ bSuccess ? SearchResult.GetOkValue().Lobbies : TArray<TSharedRef<const FLobby>>() };
	
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("Search Lobby Completed"));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| Result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| Error: %s"), bSuccess ? TEXT("") : *SearchResult.GetErrorValue().GetLogString());
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| NumLobbies: %d"), NewLobbies.Num());

	FOnlineServiceResult ServiceResult;

	if (bSuccess)
	{
		for (const auto& Lobby : NewLobbies)
		{
			UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| +Lobby: %s"), *ToLogString(Lobby->LobbyId));

			auto* NewResult{ NewObject<ULobbyResult>(this) };
			NewResult->InitializeResult(Lobby);

			OngoingSearchRequest->Results.Emplace(NewResult);
		}
	}
	else
	{
		OngoingSearchRequest->Results.Reset();

		ServiceResult = FOnlineServiceResult(SearchResult.GetErrorValue());
	}

	ensure(Delegate.IsBound());
	Delegate.ExecuteIfBound(OngoingSearchRequest, ServiceResult);

	OngoingSearchRequest = nullptr;
}


// Join Lobby

const ULobbyResult* UOnlineLobbySubsystem::GetJoinedLobby(FName LocalName) const
{
	return JoiningLobbies.FindRef(LocalName);
}

void UOnlineLobbySubsystem::AddJoiningLobby(ULobbyResult* InLobbyResult)
{
	check(InLobbyResult);

	const auto& LocalName{ InLobbyResult->GetLocalName() };
	ensure(!JoiningLobbies.Contains(LocalName));

	JoiningLobbies.Emplace(LocalName, InLobbyResult);
}

void UOnlineLobbySubsystem::RemoveJoiningLobby(ULobbyResult* InLobbyResult)
{
	if (ensure(InLobbyResult))
	{
		RemoveJoiningLobby(InLobbyResult->GetLocalName());
	}
}

void UOnlineLobbySubsystem::RemoveJoiningLobby(FName LobbyLocalName)
{
	JoiningLobbies.Remove(LobbyLocalName);
}


ULobbyJoinRequest* UOnlineLobbySubsystem::CreateOnlineLobbyJoinRequest(ULobbyResult* LobbyResult)
{
	auto* NewRequest{ NewObject<ULobbyJoinRequest>(this) };
	NewRequest->LobbyToJoin = LobbyResult;
	return NewRequest;
}

bool UOnlineLobbySubsystem::JoinLobby(APlayerController* JoiningPlayer, ULobbyJoinRequest* JoinRequest, FLobbyJoinCompleteDelegate Delegate)
{
	if (!JoinRequest)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Join Lobby Failed: Invalid Join Request"));
		return false;
	}

	auto LobbyToJoin{ JoinRequest->LobbyToJoin };
	if (!LobbyToJoin)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Join Lobby Failed: Invalid Lobby Result"));
		return false;
	}

	auto Lobby{ LobbyToJoin->GetLobby() };
	if (!Lobby)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Join Lobby failed: Invalid Lobby"));
		return false;
	}

	if (JoiningLobbies.Contains(Lobby->LocalName))
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Join Lobby failed: Already Joined (LocalName: %s)"), *Lobby->LocalName.ToString());
		return false;
	}

	auto* LocalPlayer{ JoiningPlayer ? JoiningPlayer->GetLocalPlayer() : nullptr };
	if (!LocalPlayer)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Join Lobby Failed: JoiningPlayer is invalid."));
		return false;
	}

	JoinOnlineLobbyInternal(LocalPlayer, JoinRequest, Delegate);
	return true;
}

void UOnlineLobbySubsystem::JoinOnlineLobbyInternal(ULocalPlayer* LocalPlayer, ULobbyJoinRequest* JoinRequest, FLobbyJoinCompleteDelegate Delegate)
{
	check(LocalPlayer);
	check(JoinRequest);
	ensure(Delegate.IsBound());
	ensure(!OngoingJoinRequest);

	auto LobbiesInterface{ GetLobbiesInterface() };
	check(LobbiesInterface);

	// Set Ongoing request

	OngoingJoinRequest = JoinRequest;

	// Make lobby search parameters

	auto JoinParams{ JoinRequest->GenerateJoinParameters() };
	JoinParams.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();

	// Start lobby search

	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("Start Join Lobby"));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| LocalName: %s"), *JoinParams.LocalName.ToString());
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| AccountId: %s"), *ToLogString(JoinParams.LocalAccountId));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| LobbyId: %s"), *ToLogString(JoinParams.LobbyId));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| Presence: %s"), JoinParams.bPresenceEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));

	auto Handle{ LobbiesInterface->JoinLobby(MoveTemp(JoinParams)) };
	Handle.OnComplete(this, &ThisClass::HandleJoinOnlineLobbyComplete, JoinParams.LocalAccountId, Delegate);
}

void UOnlineLobbySubsystem::HandleJoinOnlineLobbyComplete(const TOnlineResult<FJoinLobby>& JoinResult, FAccountId JoiningAccountId, FLobbyJoinCompleteDelegate Delegate)
{
	if (!ensure(OngoingJoinRequest) || !ensure(JoiningAccountId.IsValid()))
	{
		return;
	}

	const auto bSuccess{ JoinResult.IsOk() };
	const auto NewLobby{ bSuccess ? JoinResult.GetOkValue().Lobby : nullptr };
	
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("Join Lobby Completed"));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| Result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| Error: %s"), bSuccess ? TEXT("") : *JoinResult.GetErrorValue().GetLogString());
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| LobbyId: %s"), *ToLogString(NewLobby ? NewLobby->LobbyId : FLobbyId()));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| OwnerId: %s"), *ToLogString(NewLobby ? NewLobby->OwnerAccountId : FAccountId()));
	UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| MyId: %s"), *ToLogString(JoiningAccountId));

	FOnlineServiceResult ServiceResult;

	if (bSuccess)
	{
		if (!ensure(OngoingJoinRequest->LobbyToJoin))
		{
			OngoingJoinRequest->LobbyToJoin = NewObject<ULobbyResult>(this);
		}
		OngoingJoinRequest->LobbyToJoin->InitializeResult(NewLobby);

		const auto TravelURL{ ConstructJoiningLobbyTravelURL(JoiningAccountId, NewLobby->LobbyId) };
		UE_LOG(LogGameCore_OnlineLobbies, Log, TEXT("| URL: %s"), *TravelURL);

		OngoingJoinRequest->LobbyToJoin->SetLobbyTravelURL(TravelURL);

		AddJoiningLobby(OngoingJoinRequest->LobbyToJoin);
	}
	else
	{
		ServiceResult = FOnlineServiceResult(JoinResult.GetErrorValue());
	}

	ensure(Delegate.IsBound());
	Delegate.ExecuteIfBound(OngoingJoinRequest, ServiceResult);

	OngoingJoinRequest = nullptr;
}

FString UOnlineLobbySubsystem::ConstructJoiningLobbyTravelURL(const FAccountId& AccountId, const FLobbyId& LobbyId)
{
	check(AccountId.IsValid());
	check(LobbyId.IsValid());

	auto OnlineServices{ OnlineServiceSubsystem ? OnlineServiceSubsystem->GetContextCache() : nullptr };
	check(OnlineServices);

	FString URL;

	auto Result{ OnlineServices->GetResolvedConnectString({ AccountId, LobbyId }) };
	if (ensure(Result.IsOk()))
	{
		URL = Result.GetOkValue().ResolvedConnectString;
	}

	return URL;
}


// Clean Up Lobby

void UOnlineLobbySubsystem::CleanUpAllLobbies(const APlayerController* InPlayerController)
{
	auto LobbiesInterface{ GetLobbiesInterface() };
	check(LobbiesInterface);

	CleanUpOngoingRequest();

	auto* PlayerController{ InPlayerController ? InPlayerController : GetGameInstance()->GetFirstLocalPlayerController() };
	auto* LocalPlayer{ PlayerController ? PlayerController->GetLocalPlayer() : nullptr };
	auto LocalAccountId{ LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId().GetV2() : FAccountId() };

	if (!LocalAccountId.IsValid())
	{
		return;
	}

	for (auto It{ JoiningLobbies.CreateIterator() }; It; ++It)
	{
		auto Lobby{ It->Value ? It->Value->GetLobby() : nullptr };
		auto LobbyId{ Lobby ? Lobby->LobbyId : FLobbyId() };

		if (LobbyId.IsValid())
		{
			LobbiesInterface->LeaveLobby({ LocalAccountId, LobbyId });
		}

		It.RemoveCurrent();
	}
}

void UOnlineLobbySubsystem::CleanUpLobby(FName LocalName, const APlayerController* InPlayerController)
{
	auto LobbiesInterface{ GetLobbiesInterface() };
	check(LobbiesInterface);

	CleanUpOngoingRequest();

	auto* PlayerController{ InPlayerController ? InPlayerController : GetGameInstance()->GetFirstLocalPlayerController() };
	auto* LocalPlayer{ PlayerController ? PlayerController->GetLocalPlayer() : nullptr };
	auto LocalAccountId{ LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId().GetV2() : FAccountId() };

	const auto* LobbyResult{ GetJoinedLobby(LocalName) };
	auto Lobby{ LobbyResult ? LobbyResult->GetLobby() : nullptr };
	auto LobbyId{ Lobby ? Lobby->LobbyId : FLobbyId() };

	if (LocalAccountId.IsValid() && LobbyId.IsValid())
	{
		LobbiesInterface->LeaveLobby({ LocalAccountId, LobbyId });
	}

	RemoveJoiningLobby(LocalName);
}

void UOnlineLobbySubsystem::CleanUpOngoingRequest()
{
	OngoingCreateRequest = nullptr;
	OngoingJoinRequest = nullptr;
	OngoingSearchRequest = nullptr;
}


// Join Lobby Request

void UOnlineLobbySubsystem::NotifyUserJoinLobbyRequest(const FPlatformUserId& LocalPlatformUserId, ULobbyResult* RequestedLobby, FOnlineServiceResult Result)
{
	OnUserJoinLobbyRequest.Broadcast(LocalPlatformUserId, RequestedLobby, Result);
	K2_OnUserJoinLobbyRequest.Broadcast(LocalPlatformUserId, RequestedLobby, Result);
}


// Lobby Member Change

void UOnlineLobbySubsystem::NotifyLobbyMemberChanged(FName LocalName, int32 CurrentMembers, int32 MaxMembers)
{
	OnLobbyMemberChanged.Broadcast(LocalName, CurrentMembers, MaxMembers);
	K2_OnLobbyMemberChanged.Broadcast(LocalName, CurrentMembers, MaxMembers);
}


// Travel Lobby

bool UOnlineLobbySubsystem::TravelToLobby(APlayerController* InPlayerController, const ULobbyResult* LobbyResult)
{
	if (!InPlayerController)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Travel To Lobby Failed: Invalid Player Controller"));
		return false;
	}

	auto* LocalPlayer{ InPlayerController->GetLocalPlayer() };
	if (!LocalPlayer)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Travel To Lobby Failed: Can't get LocalPlayer from PlayerController(%s)"), *GetNameSafe(InPlayerController));
		return false;
	}

	const auto AccountId{ LocalPlayer->GetPreferredUniqueNetId().GetV2() };
	if (!AccountId.IsValid())
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Travel To Lobby Failed: Invalid AccountId from LocalPlayer(%s)"), *GetNameSafe(LocalPlayer));
		return false;
	}

	if (!LobbyResult)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Travel To Lobby Failed: Invalid LobbyResult"));
		return false;
	}

	const auto URL{ LobbyResult->GetLobbyTravelURL() };
	if (URL.IsEmpty())
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Travel To Lobby Failed: No URL in LobbyResult(%s)"), *GetNameSafe(LobbyResult));
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("If you have not joined the lobby, the URL does not exist."));
		return false;
	}

	auto Lobby{ LobbyResult->GetLobby() };
	if (!Lobby)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Travel To Lobby Failed: Invalid Lobby in LobbyResult(%s)"), *GetNameSafe(LobbyResult));
		return false;
	}

	auto* World{ GetWorld() };
	if (!World)
	{
		UE_LOG(LogGameCore_OnlineLobbies, Error, TEXT("Travel To Lobby Failed: Invalid World"));
		return false;
	}

	// Start Travel

	const auto bIsHost{ Lobby->OwnerAccountId == AccountId };

	if (bIsHost)
	{
		return World->ServerTravel(URL);
	}
	else
	{
		InPlayerController->ClientTravel(URL, TRAVEL_Absolute);
		return true;
	}
}
