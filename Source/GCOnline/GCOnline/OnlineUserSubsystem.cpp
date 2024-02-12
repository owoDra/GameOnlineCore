// Copyright (C) 2024 owoDra

#include "OnlineUserSubsystem.h"

#include "GameplayTag/GCOnlineTags_Platform.h"
#include "GameplayTag/GCOnlineTags_Message.h"
#include "OnlinePrivilegeSubsystem.h"
#include "OnlineErrorSubsystem.h"
#include "GCOnlineLogs.h"

// OSS v2
#include "Online/Auth.h"
#include "Online/ExternalUI.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEngineUtils.h"

#include "ICommonUIModule.h"
#include "CommonUISettings.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineUserSubsystem)


// Initialization

void UOnlineUserSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ContextCaches.CreateOnlineServiceContexts(GetWorld());

	BindOnlineDelegates();

	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	DeviceMapper.GetOnInputDeviceConnectionChange().AddUObject(this, &ThisClass::HandleInputDeviceConnectionChanged);

	SetMaxLocalPlayers(MAX_LOCAL_PLAYERS);

	ResetUserState();

	bIsDedicatedServer = GetGameInstance()->IsDedicatedServerInstance();
}

void UOnlineUserSubsystem::Deinitialize()
{
	Super::Deinitialize();

	ContextCaches.DestroyOnlineServiceContexts();

	IPlatformInputDeviceMapper& DeviceMapper = IPlatformInputDeviceMapper::Get();
	DeviceMapper.GetOnInputDeviceConnectionChange().RemoveAll(this);

	LocalUserInfos.Reset();
	ActiveLoginRequests.Reset();
}

bool UOnlineUserSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}


// Context Cache

void UOnlineUserSubsystem::BindOnlineDelegates()
{
	auto BindServiceDelegates
	{
		[this](EOnlineServiceContext Context)
		{
			auto* ContextCache{ ContextCaches.GetContextCache(Context) };
			if (ContextCache && ContextCache->IsValid())
			{
				if (auto AuthInterface{ ContextCache->OnlineServices->GetAuthInterface() })
				{
					ContextCache->LoginStatusChangedHandle =
						AuthInterface->OnLoginStatusChanged().Add(this, &ThisClass::HandleAuthLoginStatusChanged, Context);
				}

				if (auto ConnectivityInterface{ ContextCache->OnlineServices->GetConnectivityInterface() })
				{
					ContextCache->ConnectionStatusChangedHandle =
						ConnectivityInterface->OnConnectionStatusChanged().Add(this, &ThisClass::HandleNetworkConnectionStatusChanged, Context);
				}

				CacheConnectionStatus(Context);
			}
		}
	};

	// Default Service

	BindServiceDelegates(EOnlineServiceContext::Default);
	
	// Platform Service

	BindServiceDelegates(EOnlineServiceContext::Platform);
}


IAuthPtr UOnlineUserSubsystem::GetOnlineAuth(EOnlineServiceContext Context) const
{
	const auto* System{ ContextCaches.GetContextCache(Context) };

	if (System && System->IsValid())
	{
		return System->OnlineServices->GetAuthInterface();
	}

	return nullptr;
}

IConnectivityPtr UOnlineUserSubsystem::GetOnlineConnectivity(EOnlineServiceContext Context) const
{
	const auto* System{ ContextCaches.GetContextCache(Context) };

	if (System && System->IsValid())
	{
		return System->OnlineServices->GetConnectivityInterface();
	}

	return nullptr;
}


// Local User

void UOnlineUserSubsystem::SetMaxLocalPlayers(int32 InMaxLocalPlayers)
{
	if (ensure(InMaxLocalPlayers >= 1))
	{
		// We can have more local players than MAX_LOCAL_PLAYERS, the rest are treated as guests

		MaxNumberOfLocalPlayers = InMaxLocalPlayers;
		
		auto* GameInstance{ GetGameInstance() };
		auto* ViewportClient = GameInstance ? GameInstance->GetGameViewportClient() : nullptr;

		if (ViewportClient)
		{
			ViewportClient->MaxSplitscreenPlayers = MaxNumberOfLocalPlayers;
		}
	}
}

int32 UOnlineUserSubsystem::GetMaxLocalPlayers() const
{
	return MaxNumberOfLocalPlayers;
}

int32 UOnlineUserSubsystem::GetNumLocalPlayers() const
{
	auto* GameInstance{ GetGameInstance() };

	if (ensure(GameInstance))
	{
		return GameInstance->GetNumLocalPlayers();
	}

	return 1;
}


const ULocalUserAccountInfo* UOnlineUserSubsystem::GetUserInfoForLocalPlayerIndex(int32 LocalPlayerIndex) const
{
	if (auto* const Found{ LocalUserInfos.Find(LocalPlayerIndex) })
	{
		return *Found;
	}

	return nullptr;
}

const ULocalUserAccountInfo* UOnlineUserSubsystem::GetUserInfoForPlatformUser(FPlatformUserId PlatformUser) const
{
	if (!IsRealPlatformUser(PlatformUser))
	{
		return nullptr;
	}

	for (const auto& KVP : LocalUserInfos)
	{
		// Don't include guest users

		if (ensure(KVP.Value) && KVP.Value->PlatformUser == PlatformUser && !KVP.Value->bIsGuest)
		{
			return KVP.Value;
		}
	}

	return nullptr;
}

const ULocalUserAccountInfo* UOnlineUserSubsystem::GetUserInfoForUniqueNetId(const FUniqueNetIdRepl& NetId) const
{
	if (!NetId.IsValid())
	{
		return nullptr;
	}

	for (const auto& UserKVP : LocalUserInfos)
	{
		if (ensure(UserKVP.Value))
		{
			for (const auto& CachedKVP : UserKVP.Value->CachedAccountInfos)
			{
				if (NetId == CachedKVP.Value.CachedNetId)
				{
					return UserKVP.Value;
				}
			}
		}
	}

	return nullptr;
}

const ULocalUserAccountInfo* UOnlineUserSubsystem::GetUserInfoForInputDevice(FInputDeviceId InputDevice) const
{
	return GetUserInfoForPlatformUser(GetPlatformUserIdForInputDevice(InputDevice));
}

bool UOnlineUserSubsystem::IsRealPlatformUser(FPlatformUserId PlatformUser) const
{
	// Validation is done at conversion/allocation time so trust the type

	if (!PlatformUser.IsValid())
	{
		return false;
	}

	// TODO: Validate against OSS or input mapper somehow

	if (ICommonUIModule::GetSettings().GetPlatformTraits().HasTag(TAG_Platform_Trait_SingleOnlineUser))
	{
		// Only the default user is supports online functionality 

		if (PlatformUser != IPlatformInputDeviceMapper::Get().GetPrimaryPlatformUser())
		{
			return false;
		}
	}

	return true;
}

FPlatformUserId UOnlineUserSubsystem::GetPlatformUserIdForInputDevice(FInputDeviceId InputDevice) const
{
	return IPlatformInputDeviceMapper::Get().GetUserForInputDevice(InputDevice);
}

FInputDeviceId UOnlineUserSubsystem::GetPrimaryInputDeviceForPlatformUser(FPlatformUserId PlatformUser) const
{
	return IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(PlatformUser);
}

FUniqueNetIdRepl UOnlineUserSubsystem::GetLocalUserNetId(FPlatformUserId PlatformUser, EOnlineServiceContext Context) const
{
	if (!IsRealPlatformUser(PlatformUser))
	{
		return FUniqueNetIdRepl();
	}

	if (auto AuthInterface{ GetOnlineAuth(Context) })
	{
		if (auto AccountInfo{ GetOnlineServiceAccountInfo(AuthInterface, PlatformUser) })
		{
			return FUniqueNetIdRepl(AccountInfo->AccountId);
		}
	}

	return FUniqueNetIdRepl();
}

TSharedPtr<FAccountInfo> UOnlineUserSubsystem::GetOnlineServiceAccountInfo(IAuthPtr AuthService, FPlatformUserId InUserId) const
{
	TSharedPtr<FAccountInfo> AccountInfo;
	FAuthGetLocalOnlineUserByPlatformUserId::Params GetAccountParams = { InUserId };
	auto GetAccountResult{ AuthService->GetLocalOnlineUserByPlatformUserId(MoveTemp(GetAccountParams)) };

	if (GetAccountResult.IsOk())
	{
		AccountInfo = GetAccountResult.GetOkValue().AccountInfo;
	}

	return AccountInfo;
}


void UOnlineUserSubsystem::ResetUserState()
{
	// Manually purge existing info objects

	for (auto& KVP : LocalUserInfos)
	{
		if (KVP.Value)
		{
			KVP.Value->MarkAsGarbage();
		}
	}

	LocalUserInfos.Reset();

	// Cancel in-progress logins

	ActiveLoginRequests.Reset();

	// Create player info for id 0

	auto* FirstUser{ CreateLocalUserInfo(0) };

	FirstUser->PlatformUser = IPlatformInputDeviceMapper::Get().GetPrimaryPlatformUser();
	FirstUser->PrimaryInputDevice = IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(FirstUser->PlatformUser);

	/// @TODO: Schedule a refresh of player 0 for next frame?

	RefreshLocalUserInfo(FirstUser);
}

ULocalUserAccountInfo* UOnlineUserSubsystem::CreateLocalUserInfo(int32 LocalPlayerIndex)
{
	ULocalUserAccountInfo* NewUser{ nullptr };

	if (ensure(!LocalUserInfos.Contains(LocalPlayerIndex)))
	{
		NewUser = NewObject<ULocalUserAccountInfo>(this);
		NewUser->LocalPlayerIndex = LocalPlayerIndex;
		NewUser->LoginState = ELocalUserLoginState::Unknown;

		// Always create game and default cache

		NewUser->CachedAccountInfos.Add(EOnlineServiceContext::Game, ULocalUserAccountInfo::FAccountInfoCache());
		NewUser->CachedAccountInfos.Add(EOnlineServiceContext::Default, ULocalUserAccountInfo::FAccountInfoCache());

		// Add platform if needed

		if (ContextCaches.HasSeparatePlatformContext())
		{
			NewUser->CachedAccountInfos.Add(EOnlineServiceContext::Platform, ULocalUserAccountInfo::FAccountInfoCache());
		}

		LocalUserInfos.Add(LocalPlayerIndex, NewUser);
	}

	return NewUser;
}

void UOnlineUserSubsystem::RefreshLocalUserInfo(ULocalUserAccountInfo* UserInfo)
{
	if (ensure(UserInfo))
	{
		// Always update default

		UserInfo->UpdateCachedNetId(GetLocalUserNetId(UserInfo->PlatformUser, EOnlineServiceContext::Default), EOnlineServiceContext::Default);

		if (ContextCaches.HasSeparatePlatformContext())
		{
			// Also update platform

			UserInfo->UpdateCachedNetId(GetLocalUserNetId(UserInfo->PlatformUser, EOnlineServiceContext::Platform), EOnlineServiceContext::Platform);
		}
	}
}

void UOnlineUserSubsystem::SetLocalPlayerUserInfo(ULocalPlayer* LocalPlayer, const ULocalUserAccountInfo* UserInfo)
{
	if (!bIsDedicatedServer && ensure(LocalPlayer && UserInfo))
	{
		LocalPlayer->SetPlatformUserId(UserInfo->GetPlatformUserId());

		auto NetId{ UserInfo->GetNetId(EOnlineServiceContext::Game) };
		LocalPlayer->SetCachedUniqueNetId(NetId);

		// Also update player state if possible

		auto* PlayerController{ LocalPlayer->GetPlayerController(nullptr) };
		auto PlayerState{ PlayerController ? PlayerController->PlayerState : nullptr };

		if (PlayerState)
		{
			PlayerState->SetUniqueId(NetId);
		}
	}
}


// Login

ELoginStatus UOnlineUserSubsystem::GetLocalUserLoginStatus(FPlatformUserId PlatformUser, EOnlineServiceContext Context) const
{
	if (!IsRealPlatformUser(PlatformUser))
	{
		return ELoginStatus::NotLoggedIn;
	}

	if (auto AuthInterface{GetOnlineAuth(Context)})
	{
		if (auto AccountInfo{ GetOnlineServiceAccountInfo(AuthInterface, PlatformUser) })
		{
			return AccountInfo->LoginStatus;
		}
	}

	return ELoginStatus::NotLoggedIn;
}

ELocalUserLoginState UOnlineUserSubsystem::GetLocalPlayerLoginState(int32 LocalPlayerIndex) const
{
	if (const auto* UserInfo{ GetUserInfoForLocalPlayerIndex(LocalPlayerIndex) })
	{
		return UserInfo->LoginState;
	}

	if (LocalPlayerIndex < 0 || LocalPlayerIndex >= GetMaxLocalPlayers())
	{
		return ELocalUserLoginState::Invalid;
	}

	return ELocalUserLoginState::Unknown;
}


bool UOnlineUserSubsystem::TryLoginForLocalPlay(int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin)
{
	// If PrimaryInputDevice is not specified, Set to default device

	if (!PrimaryInputDevice.IsValid())
	{
		PrimaryInputDevice = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	}

	// Create login parameter

	FLocalUserLoginParams Params;
	Params.LocalPlayerIndex = LocalPlayerIndex;
	Params.PrimaryInputDevice = PrimaryInputDevice;
	Params.bCanUseGuestLogin = bCanUseGuestLogin;
	Params.bCanCreateNewLocalPlayer = true;
	Params.RequestedPrivilege = ELocalUserPrivilege::CanPlay;

	return TryGenericLogin(Params);
}

bool UOnlineUserSubsystem::TryLoginForOnlinePlay(int32 LocalPlayerIndex)
{
	FLocalUserLoginParams Params;
	Params.LocalPlayerIndex = LocalPlayerIndex;
	Params.bCanCreateNewLocalPlayer = false;
	Params.RequestedPrivilege = ELocalUserPrivilege::CanPlayOnline;

	return TryGenericLogin(Params);
}

bool UOnlineUserSubsystem::TryGenericLogin(FLocalUserLoginParams Params)
{
	if (Params.LocalPlayerIndex < 0 || (!Params.bCanCreateNewLocalPlayer && Params.LocalPlayerIndex >= GetNumLocalPlayers()))
	{
		if (!bIsDedicatedServer)
		{
			UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Try login for local player(%d) failed with current %d and max %d, invalid index"),
				Params.LocalPlayerIndex, GetNumLocalPlayers(), GetMaxLocalPlayers());
			return false;
		}
	}

	if (Params.LocalPlayerIndex > GetNumLocalPlayers() || Params.LocalPlayerIndex >= GetMaxLocalPlayers())
	{
		UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Try login for local player(%d) failed with current %d and max %d, can only create in order up to max players"),
			Params.LocalPlayerIndex, GetNumLocalPlayers(), GetMaxLocalPlayers());
		return false;
	}

	if (Params.PrimaryInputDevice.IsValid() && !Params.PlatformUser.IsValid())
	{
		Params.PlatformUser = GetPlatformUserIdForInputDevice(Params.PrimaryInputDevice);
	}
	else if (Params.PlatformUser.IsValid() && !Params.PrimaryInputDevice.IsValid())
	{
		Params.PrimaryInputDevice = GetPrimaryInputDeviceForPlatformUser(Params.PlatformUser);
	}

	auto* LocalUserInfo{ ModifyInfo(GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex)) };
	auto* LocalUserInfoForController{ ModifyInfo(GetUserInfoForInputDevice(Params.PrimaryInputDevice)) };

	if (LocalUserInfoForController && LocalUserInfo && LocalUserInfoForController != LocalUserInfo)
	{
		UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Try login for local player(%d) failed because controller %d is already assigned to player %d"),
			Params.LocalPlayerIndex, Params.PrimaryInputDevice.GetId(), LocalUserInfoForController->LocalPlayerIndex);
		return false;
	}

	if (Params.LocalPlayerIndex == 0 && Params.bCanUseGuestLogin)
	{
		UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Try login failed because player 0 cannot be a guest"));
		return false;
	}

	if (!LocalUserInfo)
	{
		LocalUserInfo = CreateLocalUserInfo(Params.LocalPlayerIndex);
	}
	else
	{
		// Copy from existing user info

		if (!Params.PrimaryInputDevice.IsValid())
		{
			Params.PrimaryInputDevice = LocalUserInfo->PrimaryInputDevice;
		}

		if (!Params.PlatformUser.IsValid())
		{
			Params.PlatformUser = LocalUserInfo->PlatformUser;
		}
	}

	if (LocalUserInfo->LoginState != ELocalUserLoginState::Unknown && LocalUserInfo->LoginState != ELocalUserLoginState::FailedToLogin)
	{
		// Not allowed to change parameters during login

		if (LocalUserInfo->PrimaryInputDevice != Params.PrimaryInputDevice || LocalUserInfo->PlatformUser != Params.PlatformUser || LocalUserInfo->bCanBeGuest != Params.bCanUseGuestLogin)
		{
			UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Try login failed because player %d has already started the login process with diffrent settings!"), Params.LocalPlayerIndex);
			return false;
		}
	}

	// Set desired index now so if it creates a player it knows what controller to use

	LocalUserInfo->PrimaryInputDevice = Params.PrimaryInputDevice;
	LocalUserInfo->PlatformUser = Params.PlatformUser;
	LocalUserInfo->bCanBeGuest = Params.bCanUseGuestLogin;
	RefreshLocalUserInfo(LocalUserInfo);

	// Either doing an initial or network login

	if (LocalUserInfo->GetPrivilegeAvailability(ELocalUserPrivilege::CanPlay) == ELocalUserAvailability::NowAvailable && Params.RequestedPrivilege == ELocalUserPrivilege::CanPlayOnline)
	{
		LocalUserInfo->LoginState = ELocalUserLoginState::DoingNetworkLogin;
	}
	else
	{
		LocalUserInfo->LoginState = ELocalUserLoginState::DoingInitialLogin;
	}

	LoginLocalUser(LocalUserInfo, Params.OnlineContext, Params.RequestedPrivilege, FLocalUserLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleLoginForUserInitialize, Params));

	return true;
}

bool UOnlineUserSubsystem::CancelLogin(int32 LocalPlayerIndex)
{
	auto* LocalUserAccountInfo{ ModifyInfo(GetUserInfoForLocalPlayerIndex(LocalPlayerIndex)) };
	if (!LocalUserAccountInfo)
	{
		return false;
	}

	if (!LocalUserAccountInfo->IsDoingLogin())
	{
		return false;
	}

	// Remove from login queue

	auto RequestsCopy{ ActiveLoginRequests };
	for (auto& Request : RequestsCopy)
	{
		if (Request->UserInfo.IsValid() && Request->UserInfo->LocalPlayerIndex == LocalPlayerIndex)
		{
			ActiveLoginRequests.Remove(Request);
		}
	}

	// Set state with best guess

	if (LocalUserAccountInfo->LoginState == ELocalUserLoginState::DoingNetworkLogin)
	{
		LocalUserAccountInfo->LoginState = ELocalUserLoginState::LoggedInLocalOnly;
	}
	else
	{
		LocalUserAccountInfo->LoginState = ELocalUserLoginState::FailedToLogin;
	}

	return true;
}

bool UOnlineUserSubsystem::TryLogout(int32 LocalPlayerIndex, bool bDestroyPlayer)
{
	UGameInstance* GameInstance = GetGameInstance();

	if (!ensure(GameInstance))
	{
		return false;
	}

	if (LocalPlayerIndex == 0 && bDestroyPlayer)
	{
		UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Try logout but cannot destroy player 0"));
		return false;
	}

	CancelLogin(LocalPlayerIndex);

	auto* LocalUserAccountInfo{ ModifyInfo(GetUserInfoForLocalPlayerIndex(LocalPlayerIndex)) };
	if (!LocalUserAccountInfo)
	{
		UE_LOG(LogGameCore_OnlineAuth, Warning, TEXT("Try logout failed to log out user %i because they are not logged in"), LocalPlayerIndex);
		return false;
	}

	// Currently this does not do platform logout in case they want to log back in immediately after

	auto UserId{ LocalUserAccountInfo->PlatformUser };
	if (IsRealPlatformUser(UserId))
	{
		UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Try logout succeeded for real platform user %d"), UserId.GetInternalId());

		LogoutLocalUser(UserId);
	}

	// For guest users just delete it

	else if (ensure(LocalUserAccountInfo->bIsGuest))
	{
		UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Try logout succeeded for guest player index %d"), LocalPlayerIndex);

		LocalUserInfos.Remove(LocalPlayerIndex);
	}

	if (bDestroyPlayer)
	{
		if (auto* ExistingPlayer{ GameInstance->FindLocalPlayerFromPlatformUserId(UserId) })
		{
			GameInstance->RemoveLocalPlayer(ExistingPlayer);
		}
	}

	return true;
}


bool UOnlineUserSubsystem::LoginLocalUser(const ULocalUserAccountInfo* UserInfo, EOnlineServiceContext Context, ELocalUserPrivilege RequestedPrivilege, FLocalUserLoginCompleteDelegate OnComplete)
{
	if (!ensure(UserInfo))
	{
		return false;
	}

	auto* LocalUserAccountInfo{ ModifyInfo(UserInfo) };

	TSharedRef<FUserLoginRequest> NewRequest = MakeShared<FUserLoginRequest>(LocalUserAccountInfo, RequestedPrivilege, Context, MoveTemp(OnComplete));
	ActiveLoginRequests.Add(NewRequest);

	// This will execute callback or start login process

	ProcessLoginRequest(NewRequest);

	return true;
}

void UOnlineUserSubsystem::LogoutLocalUser(FPlatformUserId PlatformUser)
{
	if (auto* LocalUserAccountInfo{ ModifyInfo(GetUserInfoForPlatformUser(PlatformUser)) })
	{
		// Don't need to do anything if the user has never logged in fully or is in the process of logging in

		if (LocalUserAccountInfo->LoginState == ELocalUserLoginState::LoggedInLocalOnly || LocalUserAccountInfo->LoginState == ELocalUserLoginState::LoggedInOnline)
		{
			auto OldAvailablity{ LocalUserAccountInfo->GetPrivilegeAvailability(ELocalUserPrivilege::CanPlay) };

			LocalUserAccountInfo->LoginState = ELocalUserLoginState::FailedToLogin;

			// This will broadcast the game delegate

			LocalUserAccountInfo->HandleChangedAvailability(ELocalUserPrivilege::CanPlay, OldAvailablity);
		}
	}
}

void UOnlineUserSubsystem::ProcessLoginRequest(TSharedRef<FUserLoginRequest> Request)
{
	// User is gone, just delete this request

	auto* UserInfo{ Request->UserInfo.Get() };

	if (!UserInfo)
	{
		ActiveLoginRequests.Remove(Request);

		return;
	}

	// If the platform user id is invalid because this is a guest, skip right to failure

	const auto PlatformUser{ UserInfo->GetPlatformUserId() };

	if (!IsRealPlatformUser(PlatformUser))
	{
		Request->Error = UE::Online::Errors::InvalidUser();

		// Remove from active array

		ActiveLoginRequests.Remove(Request);

		// Execute delegate if bound

		Request->Delegate.ExecuteIfBound(UserInfo, ELoginStatusType::NotLoggedIn, FUniqueNetIdRepl(), Request->Error, Request->DesiredContext);

		return;
	}

	// Figure out what context to process first

	if (Request->CurrentContext == EOnlineServiceContext::Invalid)
	{
		// First start with platform context if this is a game login

		if (Request->DesiredContext == EOnlineServiceContext::Game)
		{
			Request->CurrentContext = ContextCaches.ResolveOnlineServiceContext(EOnlineServiceContext::PlatformOrDefault);
		}
		else
		{
			Request->CurrentContext = ContextCaches.ResolveOnlineServiceContext(Request->DesiredContext);
		}
	}

	auto CurrentStatus{ GetLocalUserLoginStatus(PlatformUser, Request->CurrentContext) };
	auto CurrentId{ GetLocalUserNetId(PlatformUser, Request->CurrentContext) };
	auto* System{ ContextCaches.GetContextCache(Request->CurrentContext) };

	if (!ensure(System))
	{
		return;
	}

	// Starting a new request

	if (Request->OverallLoginState == EOnlineServiceTaskState::NotStarted)
	{
		Request->OverallLoginState = EOnlineServiceTaskState::InProgress;
	}

	// If this is not an online required login, allow local profile to count as fully logged in

	bool bHasRequiredStatus = (CurrentStatus == ELoginStatusType::LoggedIn);
	if (Request->DesiredPrivilege == ELocalUserPrivilege::CanPlay)
	{
		bHasRequiredStatus |= (CurrentStatus == ELoginStatusType::UsingLocalProfile);
	}

	// Check for overall success

	if (CurrentStatus != ELoginStatusType::NotLoggedIn && CurrentId.IsValid())
	{
		// Stall if we're waiting for the login UI to close

		if (Request->LoginUIState == EOnlineServiceTaskState::InProgress)
		{
			return;
		}

		Request->OverallLoginState = EOnlineServiceTaskState::Done;
	}
	else
	{
		// Try using platform auth to login

		if (Request->TransferPlatformAuthState == EOnlineServiceTaskState::NotStarted)
		{
			Request->TransferPlatformAuthState = EOnlineServiceTaskState::InProgress;

			if (TransferPlatformAuth(System, Request, PlatformUser))
			{
				return;
			}

			// We didn't start a login attempt, so set failure

			Request->TransferPlatformAuthState = EOnlineServiceTaskState::Failed;
		}

		// Next check AutoLogin

		if (Request->AutoLoginState == EOnlineServiceTaskState::NotStarted)
		{
			if (Request->TransferPlatformAuthState == EOnlineServiceTaskState::Done || Request->TransferPlatformAuthState == EOnlineServiceTaskState::Failed)
			{
				Request->AutoLoginState = EOnlineServiceTaskState::InProgress;

				// Try an auto login with default credentials, this will work on many platforms

				if (AutoLogin(System, Request, PlatformUser))
				{
					return;
				}

				// We didn't start an autologin attempt, so set failure

				Request->AutoLoginState = EOnlineServiceTaskState::Failed;
			}
		}

		// Next check login UI

		if (Request->LoginUIState == EOnlineServiceTaskState::NotStarted)
		{
			if ((Request->TransferPlatformAuthState == EOnlineServiceTaskState::Done || Request->TransferPlatformAuthState == EOnlineServiceTaskState::Failed)
				&& (Request->AutoLoginState == EOnlineServiceTaskState::Done || Request->AutoLoginState == EOnlineServiceTaskState::Failed))
			{
				Request->LoginUIState = EOnlineServiceTaskState::InProgress;

				if (ShowLoginUI(System, Request, PlatformUser))
				{
					return;
				}

				// We didn't show a UI, so set failure

				Request->LoginUIState = EOnlineServiceTaskState::Failed;
			}
		}
	}

	// Check for overall failure

	if (Request->LoginUIState == EOnlineServiceTaskState::Failed &&
		Request->AutoLoginState == EOnlineServiceTaskState::Failed &&
		Request->TransferPlatformAuthState == EOnlineServiceTaskState::Failed)
	{
		Request->OverallLoginState = EOnlineServiceTaskState::Failed;
	}

	// If none of the substates are still in progress but we haven't successfully logged in, mark this as a failure to avoid stalling forever

	else if (Request->OverallLoginState == EOnlineServiceTaskState::InProgress &&
		Request->LoginUIState != EOnlineServiceTaskState::InProgress &&
		Request->AutoLoginState != EOnlineServiceTaskState::InProgress &&
		Request->TransferPlatformAuthState != EOnlineServiceTaskState::InProgress)
	{
		Request->OverallLoginState = EOnlineServiceTaskState::Failed;
	}

	if (Request->OverallLoginState == EOnlineServiceTaskState::Done)
	{
		// Do the permissions check if needed

		if (Request->PrivilegeCheckState == EOnlineServiceTaskState::NotStarted)
		{
			Request->PrivilegeCheckState = EOnlineServiceTaskState::InProgress;

			ELocalUserPrivilegeResult CachedResult = UserInfo->GetCachedPrivilegeResult(Request->DesiredPrivilege, Request->CurrentContext);
			if (CachedResult == ELocalUserPrivilegeResult::Available)
			{
				// Use cached success value
				Request->PrivilegeCheckState = EOnlineServiceTaskState::Done;
			}
			else
			{
				if (QueryLoginRequestedPrivilege(System, Request, PlatformUser))
				{
					return;
				}
				else
				{
					CachedResult = ELocalUserPrivilegeResult::Available;
					Request->PrivilegeCheckState = EOnlineServiceTaskState::Done;
				}
			}
		}

		// Count a privilege failure as a login failure

		if (Request->PrivilegeCheckState == EOnlineServiceTaskState::Failed)
		{
			Request->OverallLoginState = EOnlineServiceTaskState::Failed;
		}

		// If platform context done but still need to do service context, do that next

		else if (Request->PrivilegeCheckState == EOnlineServiceTaskState::Done)
		{
			auto ResolvedDesiredContext{ ContextCaches.ResolveOnlineServiceContext(Request->DesiredContext) };

			if (Request->OverallLoginState == EOnlineServiceTaskState::Done && Request->CurrentContext != ResolvedDesiredContext)
			{
				Request->CurrentContext = ResolvedDesiredContext;
				Request->OverallLoginState = EOnlineServiceTaskState::NotStarted;
				Request->PrivilegeCheckState = EOnlineServiceTaskState::NotStarted;
				Request->TransferPlatformAuthState = EOnlineServiceTaskState::NotStarted;

				// Reprocess and immediately return

				ProcessLoginRequest(Request);
				return;
			}
		}
	}

	// Stall to wait for it to finish

	if (Request->PrivilegeCheckState == EOnlineServiceTaskState::InProgress)
	{
		return;
	}

	// If done, remove and do callback

	if (Request->OverallLoginState == EOnlineServiceTaskState::Done || Request->OverallLoginState == EOnlineServiceTaskState::Failed)
	{
		// Skip if this already happened in a nested function

		if (ActiveLoginRequests.Contains(Request))
		{
			// Add a generic error if none is set

			if (Request->OverallLoginState == EOnlineServiceTaskState::Failed && !Request->Error.IsSet())
			{
				Request->Error = UE::Online::Errors::RequestFailure();
			}

			// Remove from active array

			ActiveLoginRequests.Remove(Request);

			// Execute delegate if bound

			Request->Delegate.ExecuteIfBound(UserInfo, CurrentStatus, CurrentId, Request->Error, Request->DesiredContext);
		}
	}
}

bool UOnlineUserSubsystem::TransferPlatformAuth(FOnlineServiceContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	auto PlatformAuthInterface{ GetOnlineAuth(EOnlineServiceContext::Platform) };

	if (PlatformAuthInterface && (Request->CurrentContext != EOnlineServiceContext::Platform))
	{
		FAuthQueryExternalAuthToken::Params Params;
		Params.LocalAccountId = GetLocalUserNetId(PlatformUser, EOnlineServiceContext::Platform).GetV2();

		PlatformAuthInterface->QueryExternalAuthToken(MoveTemp(Params)).OnComplete(this, 
			[this, Request](const TOnlineResult<FAuthQueryExternalAuthToken>& Result)
			{
				// User is gone, just delete this request

				auto* LocalUserAccountInfo{ Request->UserInfo.Get() };
				if (!LocalUserAccountInfo)
				{
					ActiveLoginRequests.Remove(Request);
					return;
				}

				if (Result.IsOk())
				{
					const auto& GenerateAuthTokenResult{ Result.GetOkValue() };

					FAuthLogin::Params Params;
					Params.PlatformUserId = LocalUserAccountInfo->GetPlatformUserId();
					Params.CredentialsType = LoginCredentialsType::ExternalAuth;
					Params.CredentialsToken.Emplace<FExternalAuthToken>(GenerateAuthTokenResult.ExternalAuthToken);

					auto PrimaryAuthInterface{ GetOnlineAuth(Request->CurrentContext) };
					PrimaryAuthInterface->Login(MoveTemp(Params)).OnComplete(this, 
						[this, Request](const TOnlineResult<FAuthLogin>& Result)
						{
							// User is gone, just delete this request

							auto* LocalUserAccountInfo{ Request->UserInfo.Get() };
							if (!LocalUserAccountInfo)
							{
								ActiveLoginRequests.Remove(Request);
								return;
							}

							if (Result.IsOk())
							{
								Request->TransferPlatformAuthState = EOnlineServiceTaskState::Done;
								Request->Error.Reset();
							}
							else
							{
								Request->TransferPlatformAuthState = EOnlineServiceTaskState::Failed;
								Request->Error = Result.GetErrorValue();
							}

							ProcessLoginRequest(Request);
						}
					);
				}
				else
				{
					Request->TransferPlatformAuthState = EOnlineServiceTaskState::Failed;
					Request->Error = Result.GetErrorValue();
					ProcessLoginRequest(Request);
				}
			}
		);

		return true;
	}

	return false;
}

bool UOnlineUserSubsystem::AutoLogin(FOnlineServiceContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	FAuthLogin::Params LoginParameters;
	LoginParameters.PlatformUserId = PlatformUser;
	LoginParameters.CredentialsType = LoginCredentialsType::Auto;

	// Leave other LoginParameters as default to allow the online service to determine how to try to automatically log in the user

	auto LoginHandle{ System->OnlineServices->GetAuthInterface()->Login(MoveTemp(LoginParameters))};
	LoginHandle.OnComplete(this, &ThisClass::HandleUserLoginCompleted, PlatformUser, Request->CurrentContext);

	return true;
}

bool UOnlineUserSubsystem::ShowLoginUI(FOnlineServiceContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	auto ExternalUI{ System->OnlineServices->GetExternalUIInterface() };

	if (ExternalUI.IsValid())
	{
		FExternalUIShowLoginUI::Params ShowLoginUIParameters;
		ShowLoginUIParameters.PlatformUserId = PlatformUser;

		auto LoginHandle{ ExternalUI->ShowLoginUI(MoveTemp(ShowLoginUIParameters)) };
		LoginHandle.OnComplete(this, &ThisClass::HandleOnLoginUIClosed, PlatformUser, Request->CurrentContext);

		return true;
	}

	return false;
}

bool UOnlineUserSubsystem::QueryLoginRequestedPrivilege(FOnlineServiceContextCache* System, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	const auto* GameInstance{ GetGameInstance() };
	ensure(GameInstance);

	if (auto* PrivilegeSubsystem{ UGameInstance::GetSubsystem<UOnlinePrivilegeSubsystem>(GameInstance) })
	{
		return PrivilegeSubsystem->QueryUserPrivilege(Request->UserInfo.Get(), Request->CurrentContext, Request->DesiredPrivilege, 
										FLocalUserPrivilegeQueryDelegate::CreateUObject(this, &ThisClass::HandleCheckPrivilegesComplete));
	}

	return false;
}


void UOnlineUserSubsystem::HandleAuthLoginStatusChanged(const FAuthLoginStatusChanged& EventParameters, EOnlineServiceContext Context)
{
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Player login status changed - System:%d, UserId:%s, NewStatus:%s"),
		static_cast<int32>(Context),
		*ToLogString(EventParameters.AccountInfo->AccountId),
		LexToString(EventParameters.LoginStatus));
}

void UOnlineUserSubsystem::HandleUserLoginCompleted(const TOnlineResult<FAuthLogin>& Result, FPlatformUserId PlatformUser, EOnlineServiceContext Context)
{
	const auto bWasSuccessful{ Result.IsOk() };
	const auto NewId{ bWasSuccessful ? Result.GetOkValue().AccountInfo->AccountId : FAccountId() };
	const auto NewStatus{ GetLocalUserLoginStatus(PlatformUser, Context) };

	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Player login Completed - System:%d, UserIdx:%d, Successful:%s, NewId:%s, ErrorIfAny:%s")
		, static_cast<int32>(Context)
		, PlatformUser.GetInternalId()
		, Result.IsOk() ? TEXT("TRUE") : TEXT("FASLE")
		, *ToLogString(NewId)
		, Result.IsError() ? *Result.GetErrorValue().GetLogString() : TEXT(""));

	// Update any waiting login requests

	auto RequestsCopy{ ActiveLoginRequests };
	for (auto& Request : RequestsCopy)
	{
		auto* LocalUserAccountInfo{ Request->UserInfo.Get() };

		// User is gone, just delete this request

		if (!LocalUserAccountInfo)
		{
			ActiveLoginRequests.Remove(Request);

			continue;
		}

		// On some platforms this gets called from the login UI with a failure

		if (LocalUserAccountInfo->PlatformUser == PlatformUser && Request->CurrentContext == Context)
		{
			if (Request->AutoLoginState == EOnlineServiceTaskState::InProgress)
			{
				Request->AutoLoginState = bWasSuccessful ? EOnlineServiceTaskState::Done : EOnlineServiceTaskState::Failed;
			}

			if (bWasSuccessful)
			{
				Request->Error.Reset();
			}
			else
			{
				Request->Error = Result.GetErrorValue();
			}

			ProcessLoginRequest(Request);
		}
	}
}

void UOnlineUserSubsystem::HandleOnLoginUIClosed(const TOnlineResult<FExternalUIShowLoginUI>& Result, FPlatformUserId PlatformUser, EOnlineServiceContext Context)
{
	// Update any waiting login requests

	auto RequestsCopy{ ActiveLoginRequests };
	for (auto& Request : RequestsCopy)
	{
		auto* LocalUserAccountInfo{ Request->UserInfo.Get() };

		// User is gone, just delete this request

		if (!LocalUserAccountInfo)
		{
			ActiveLoginRequests.Remove(Request);

			continue;
		}

		// Look for first user trying to log in on this context

		if (Request->CurrentContext == Context && Request->LoginUIState == EOnlineServiceTaskState::InProgress)
		{
			if (Result.IsOk())
			{
				// The platform user id that actually logged in may not be the same one who requested the UI,
				// so swap it if the returned id is actually valid

				if (LocalUserAccountInfo->PlatformUser != PlatformUser && PlatformUser != PLATFORMUSERID_NONE)
				{
					LocalUserAccountInfo->PlatformUser = PlatformUser;
				}

				Request->LoginUIState = EOnlineServiceTaskState::Done;
				Request->Error.Reset();
			}
			else
			{
				Request->LoginUIState = EOnlineServiceTaskState::Failed;
				Request->Error = Result.GetErrorValue();
			}

			ProcessLoginRequest(Request);
		}
	}
}

void UOnlineUserSubsystem::HandleCheckPrivilegesComplete(ULocalUserAccountInfo* UserInfo, EOnlineServiceContext Context, ELocalUserPrivilege DesiredPrivilege, ELocalUserPrivilegeResult PrivilegeResult, const TOptional<FOnlineErrorType>& Error)
{
	// See if a login request is waiting on this

	auto RequestsCopy{ ActiveLoginRequests };
	for (auto& Request : RequestsCopy)
	{
		if ((Request->UserInfo.Get() == UserInfo) && 
			(Request->CurrentContext == Context) && 
			(Request->DesiredPrivilege == DesiredPrivilege) &&
			(Request->PrivilegeCheckState == EOnlineServiceTaskState::InProgress))
		{
			if (PrivilegeResult == ELocalUserPrivilegeResult::Available)
			{
				Request->PrivilegeCheckState = EOnlineServiceTaskState::Done;
			}
			else
			{
				Request->PrivilegeCheckState = EOnlineServiceTaskState::Failed;
				Request->Error = Error.IsSet() ? Error.GetValue() : Errors::Unknown();
			}

			ProcessLoginRequest(Request);
		}
	}
}


void UOnlineUserSubsystem::HandleLoginForUserInitialize(const ULocalUserAccountInfo* UserInfo, ELoginStatusType NewStatus, FUniqueNetIdRepl NetId, const TOptional<FOnlineErrorType>& InError, EOnlineServiceContext Context, FLocalUserLoginParams Params)
{
	auto* GameInstance{ GetGameInstance() };
	check(GameInstance);

	auto& TimerManager{ GameInstance->GetTimerManager() };
	auto Error{ InError }; // Copy so we can reset on handled errors

	auto* LocalUserInfo{ ModifyInfo(UserInfo) };
	auto* FirstUserInfo{ ModifyInfo(GetUserInfoForLocalPlayerIndex(0)) };

	if (!ensure(LocalUserInfo && FirstUserInfo))
	{
		return;
	}

	// Check the hard platform/service ids

	RefreshLocalUserInfo(LocalUserInfo);

	auto FirstPlayerId{ FirstUserInfo->GetNetId(EOnlineServiceContext::PlatformOrDefault) };

	// Check to see if we should make a guest after a login failure. Some platforms return success but reuse the first player's id, count this as a failure

	if (LocalUserInfo != FirstUserInfo && LocalUserInfo->bCanBeGuest && (NewStatus == ELoginStatusType::NotLoggedIn || NetId == FirstPlayerId))
	{
		// TODO:  OSSv2 FUniqueNetIdRepl wrapping FAccountId is in progress
		// TODO:  OSSv2 - How to handle guest accounts?

		LocalUserInfo->bIsGuest = true;
		NewStatus = ELoginStatusType::UsingLocalProfile;
		Error.Reset();
		UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("HandleLoginForUserInitialize created guest id %s for local player %d"), *NetId.ToString(), LocalUserInfo->LocalPlayerIndex);
	}
	else
	{
		LocalUserInfo->bIsGuest = false;
	}

	ensure(LocalUserInfo->IsDoingLogin());

	if (Error.IsSet())
	{
		auto ErrorText{ Error.GetValue().GetText() };
		TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::HandleUserLoginFailed, Params, ErrorText));
		return;
	}

	if (Context == EOnlineServiceContext::Game)
	{
		LocalUserInfo->UpdateCachedNetId(NetId, EOnlineServiceContext::Game);
	}

	auto* CurrentPlayer{ GameInstance->GetLocalPlayerByIndex(LocalUserInfo->LocalPlayerIndex) };
	if (!CurrentPlayer && Params.bCanCreateNewLocalPlayer)
	{
		FString ErrorString;

		CurrentPlayer = GameInstance->CreateLocalPlayer(LocalUserInfo->PlatformUser, ErrorString, true);
		if (!CurrentPlayer)
		{
			TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::HandleUserLoginFailed, Params, FText::AsCultureInvariant(ErrorString)));
			return;
		}

		ensure(GameInstance->GetLocalPlayerByIndex(LocalUserInfo->LocalPlayerIndex) == CurrentPlayer);
	}

	// Updates controller and net id if needed

	SetLocalPlayerUserInfo(CurrentPlayer, LocalUserInfo);

	// Set a delayed callback

	TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::HandleUserLoginSucceeded, Params));
}

void UOnlineUserSubsystem::HandleUserLoginFailed(FLocalUserLoginParams Params, FText Error)
{
	auto* LocalUserAccountInfo{ ModifyInfo(GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex)) };

	// The user info was reset since this was scheduled

	if (!LocalUserAccountInfo)
	{
		return;
	}

	UE_LOG(LogGameCore_OnlineAuth, Warning, TEXT("Try login local player(%d) failed with error %s"), LocalUserAccountInfo->LocalPlayerIndex, *Error.ToString());

	// If state is wrong, abort as we might have gotten canceled

	if (!ensure(LocalUserAccountInfo->IsDoingLogin()))
	{
		return;
	}

	// If initial login failed or we ended up totally logged out, set to complete failure

	auto NewStatus{ GetLocalUserLoginStatus(Params.PlatformUser, Params.OnlineContext) };
	if (NewStatus == ELoginStatusType::NotLoggedIn || LocalUserAccountInfo->LoginState == ELocalUserLoginState::DoingInitialLogin)
	{
		LocalUserAccountInfo->LoginState = ELocalUserLoginState::FailedToLogin;
	}
	else
	{
		LocalUserAccountInfo->LoginState = ELocalUserLoginState::LoggedInLocalOnly;
	}

	FText TitleText{ NSLOCTEXT("GameOnlineCore", "LoginFailedTitle", "Login Failure") };

	if (!Params.bSuppressLoginErrors)
	{
		SendSystemMessage(TAG_Message_Online_Error_LoginLocalPlayerFailed.GetTag(), TitleText, Error);
	}

	// Call callbacks

	Params.OnLocalUserLoginComplete.ExecuteIfBound(LocalUserAccountInfo, false, Error, Params.RequestedPrivilege, Params.OnlineContext);
	OnUserLoginComplete.Broadcast(LocalUserAccountInfo, false, Error, Params.RequestedPrivilege, Params.OnlineContext);
}

void UOnlineUserSubsystem::HandleUserLoginSucceeded(FLocalUserLoginParams Params)
{
	auto* LocalUserAccountInfo{ ModifyInfo(GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex)) };

	// The user info was reset since this was scheduled

	if (!LocalUserAccountInfo)
	{
		return;
	}

	// If state is wrong, abort as we might have gotten cancelled

	if (!ensure(LocalUserAccountInfo->IsDoingLogin()))
	{
		return;
	}

	// Fix up state

	if (Params.RequestedPrivilege == ELocalUserPrivilege::CanPlayOnline)
	{
		LocalUserAccountInfo->LoginState = ELocalUserLoginState::LoggedInOnline;
	}
	else
	{
		LocalUserAccountInfo->LoginState = ELocalUserLoginState::LoggedInLocalOnly;
	}

	ensure(LocalUserAccountInfo->GetPrivilegeAvailability(Params.RequestedPrivilege) == ELocalUserAvailability::NowAvailable);

	// Call callbacks

	Params.OnLocalUserLoginComplete.ExecuteIfBound(LocalUserAccountInfo, true, FText(), Params.RequestedPrivilege, Params.OnlineContext);
	OnUserLoginComplete.Broadcast(LocalUserAccountInfo, true, FText(), Params.RequestedPrivilege, Params.OnlineContext);
}


// Connection

void UOnlineUserSubsystem::HandleNetworkConnectionStatusChanged(const FConnectionStatusChanged& EventParameters, EOnlineServiceContext Context)
{
	UE_LOG(LogGameCore_OnlineConnectivity, Log, TEXT("HandleNetworkConnectionStatusChanged(Context:%d, ServiceName:%s, OldStatus:%s, NewStatus:%s)"),
		static_cast<int32>(Context),
		*EventParameters.ServiceName,
		LexToString(EventParameters.PreviousStatus),
		LexToString(EventParameters.CurrentStatus));

	// Cache old availablity for current users

	TMap<ULocalUserAccountInfo*, ELocalUserAvailability> AvailabilityMap;

	for (const auto& KVP : LocalUserInfos)
	{
		AvailabilityMap.Add(KVP.Value, KVP.Value->GetPrivilegeAvailability(ELocalUserPrivilege::CanPlayOnline));
	}

	// Update connection status

	auto* ContextCache{ ContextCaches.GetContextCache(Context) };
	if (ensure(ContextCache))
	{
		ContextCache->CurrentConnectionStatus = EventParameters.CurrentStatus;
	}

	// Notify other systems when someone goes online/offline

	for (const auto& KVP : AvailabilityMap)
	{
		//HandleChangedAvailability(KVP.Key, ELocalUserPrivilege::CanPlayOnline, KVP.Value);
	}
}


void UOnlineUserSubsystem::CacheConnectionStatus(EOnlineServiceContext Context)
{
	auto ConnectionStatus{ EOnlineServicesConnectionStatus::NotConnected };

	if (auto ConnectivityInterface{ GetOnlineConnectivity(Context) })
	{
		const auto Result{ ConnectivityInterface->GetConnectionStatus(FGetConnectionStatus::Params()) };

		if (Result.IsOk())
		{
			ConnectionStatus = Result.GetOkValue().Status;
		}
	}
	else
	{
		ConnectionStatus = EOnlineServicesConnectionStatus::Connected;
	}

	FConnectionStatusChanged EventParams;
	EventParams.PreviousStatus = GetConnectionStatus(Context);
	EventParams.CurrentStatus = ConnectionStatus;
	HandleNetworkConnectionStatusChanged(EventParams, Context);
}

EOnlineServicesConnectionStatus UOnlineUserSubsystem::GetConnectionStatus(EOnlineServiceContext Context) const
{
	if (const auto* System{ ContextCaches.GetContextCache(Context) })
	{
		return System->CurrentConnectionStatus;
	}

	return EOnlineServicesConnectionStatus::NotConnected;
}

bool UOnlineUserSubsystem::HasOnlineConnection(EOnlineServiceContext Context) const
{
	return GetConnectionStatus(Context) == EOnlineServicesConnectionStatus::Connected;
}


// Input

void UOnlineUserSubsystem::HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId)
{
	auto InputDeviceIDString{ FString::Printf(TEXT("%d"), InputDeviceId.GetId()) };
	const auto bIsConnected{ NewConnectionState == EInputDeviceConnectionState::Connected };

	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Controller connection changed - UserIdx:%d, UserID:%d, Connected:%d"), *InputDeviceIDString, PlatformUserId.GetInternalId(), bIsConnected ? 1 : 0);
}


// Utilities

void UOnlineUserSubsystem::SendSystemMessage(FGameplayTag MessageType, FText TitleText, FText BodyText)
{
	if (auto* ErrorSubsystem{ UGameInstance::GetSubsystem<UOnlineErrorSubsystem>(GetGameInstance()) })
	{
		ErrorSubsystem->SendSystemMessage(MessageType, TitleText, BodyText);
	}
}
