// Copyright (C) 2024 owoDra

#include "OnlineAuthSubsystem.h"

#include "OnlineServiceSubsystem.h"
#include "OnlinePrivilegeSubsystem.h"
#include "OnlineLocalUserSubsystem.h"
#include "OnlineLocalUserManagerSubsystem.h"
#include "GCOnlineLogs.h"

// OSS v2
#include "Online/Auth.h"
#include "Online/ExternalUI.h"
#include "Online/OnlineResult.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineAuthSubsystem)


// Initialization

void UOnlineAuthSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	OnlineServiceSubsystem = Collection.InitializeDependency<UOnlineServiceSubsystem>();
	OnlineLocalUserManagerSubsystem = Collection.InitializeDependency<UOnlineLocalUserManagerSubsystem>();

	check(OnlineServiceSubsystem);
	check(OnlineLocalUserManagerSubsystem);

	BindLoginDelegates();
}

void UOnlineAuthSubsystem::Deinitialize()
{
	Super::Deinitialize();

	OnlineServiceSubsystem = nullptr;
	OnlineLocalUserManagerSubsystem = nullptr;

	UnbindLoginDelegates();

	ActiveLoginRequests.Reset();
}

bool UOnlineAuthSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}


void UOnlineAuthSubsystem::BindLoginDelegates()
{
	auto BindServiceDelegates
	{
		[this](EOnlineServiceContext Context)
		{
			if (auto AuthInterface{ GetAuthInterface(Context) })
			{
				auto& Handle{ LoginHandles.FindOrAdd(Context) };

				Handle = AuthInterface->OnLoginStatusChanged().Add(this, &ThisClass::HandleAuthLoginStatusChanged, Context);
			}
		}
	};

	// Default Service

	BindServiceDelegates(EOnlineServiceContext::Default);

	// Platform Service

	BindServiceDelegates(EOnlineServiceContext::Platform);
}

void UOnlineAuthSubsystem::UnbindLoginDelegates()
{
	for (auto It{ LoginHandles.CreateIterator() }; It; ++It)
	{
		It->Value.Unbind();
		It.RemoveCurrent();
	}
}


IAuthPtr UOnlineAuthSubsystem::GetAuthInterface(EOnlineServiceContext Context) const
{
	auto OnlineService{ OnlineServiceSubsystem->GetContextCache() };

	if (ensure(OnlineService))
	{
		return OnlineService->GetAuthInterface();
	}

	return nullptr;
}

TSharedPtr<FAccountInfo> UOnlineAuthSubsystem::GetOnlineServiceAccountInfo(IAuthPtr AuthService, FPlatformUserId InUserId) const
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

FAccountId UOnlineAuthSubsystem::GetLocalUserNetId(FPlatformUserId PlatformUser, EOnlineServiceContext Context) const
{
	if (OnlineLocalUserManagerSubsystem->IsRealPlatformUser(PlatformUser))
	{
		if (auto AuthInterface{ GetAuthInterface(Context) })
		{
			if (auto AccountInfo{ GetOnlineServiceAccountInfo(AuthInterface, PlatformUser) })
			{
				return AccountInfo->AccountId;
			}
		}
	}

	return FAccountId();
}


// Login

bool UOnlineAuthSubsystem::TryLogin(const ULocalPlayer* LocalPlayer, FLocalUserLoginParams Params)
{
	// Check is local player valid

	if (!LocalPlayer)
	{
		UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Try login failed: Invalid local player"));
		return false;
	}

	// Check has local user initialized

	auto LocalUser{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer) };
	check(LocalUser);

	if (!LocalUser->HasLocalUserInitialized())
	{
		UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Try login failed: Local user not initialized (Player: %d)"), LocalPlayer->GetLocalPlayerIndex());
		return false;
	}

	// Check has not start logging in

	if ((LocalUser->LoginState != ELocalUserLoginState::Unknown) && 
		(LocalUser->LoginState != ELocalUserLoginState::FailedToLogin))
	{
		UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Try login failed: Already started the login process (Player: %d)"), LocalPlayer->GetLocalPlayerIndex());
		return false;
	}

	// Either doing an initial or network login

	if ((LocalUser->GetPrivilegeAvailability(EOnlinePrivilege::CanPlay) == ELocalUserOnlineAvailability::NowAvailable) && 
		(Params.RequestedPrivilege == EOnlinePrivilege::CanPlayOnline))
	{
		LocalUser->LoginState = ELocalUserLoginState::DoingNetworkLogin;
	}
	else
	{
		LocalUser->LoginState = ELocalUserLoginState::DoingInitialLogin;
	}

	LoginLocalUser(LocalUser, Params.OnlineContext, Params.RequestedPrivilege, FLocalUserLoginCompleteDelegate::CreateUObject(this, &ThisClass::HandleLoginForUserInitialize, Params));

	return true;
}

bool UOnlineAuthSubsystem::CancelLogin(const ULocalPlayer* LocalPlayer)
{
	// Check is local player valid

	if (!LocalPlayer)
	{
		UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Cancel login failed: Invalid local player"));
		return false;
	}

	// Check has local user initialized

	auto LocalUser{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer) };
	check(LocalUser);

	if (!LocalUser->IsDoingLogin())
	{
		UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Cancel login failed: Has not start login process (Player: %d)"), LocalPlayer->GetLocalPlayerIndex());
		return false;
	}

	// Remove from login queue

	auto RequestsCopy{ ActiveLoginRequests };
	for (auto& Request : RequestsCopy)
	{
		if (Request->LocalUser == LocalUser)
		{
			ActiveLoginRequests.Remove(Request);
		}
	}

	LocalUser->LoginState = ELocalUserLoginState::Unknown;

	return true;
}

bool UOnlineAuthSubsystem::TryLogout(ULocalPlayer* LocalPlayer, bool bDestroyPlayer)
{
	// Check is local player valid

	if (!LocalPlayer)
	{
		UE_LOG(LogGameCore_OnlineAuth, Error, TEXT("Try logout failed: Invalid local player"));
		return false;
	}

	// Check has local user initialized

	auto LocalUser{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer) };
	check(LocalUser);

	// Cancel login first if it is logging in progress

	CancelLogin(LocalPlayer);

	// Logout if not guest

	if (!LocalUser->bIsGuest)
	{
		/// @TODO Should we imple true logout?
	}

	// Reset user state

	LocalUser->ResetLocalUser();

	// Remove local player if it is not primary player

	if (bDestroyPlayer && !LocalPlayer->IsPrimaryPlayer())
	{
		auto* GameInstance{ GetGameInstance() };

		if (ensure(GameInstance))
		{
			GameInstance->RemoveLocalPlayer(LocalPlayer);
		}
	}

	return true;
}


bool UOnlineAuthSubsystem::LoginLocalUser(UOnlineLocalUserSubsystem* LocalUser, EOnlineServiceContext Context, EOnlinePrivilege RequestedPrivilege, FLocalUserLoginCompleteDelegate OnComplete)
{
	check(LocalUser);

	auto NewRequest{ MakeShared<FUserLoginRequest>(LocalUser, RequestedPrivilege, Context, MoveTemp(OnComplete)) };
	ActiveLoginRequests.Add(NewRequest);

	// This will execute callback or start login process

	ProcessLoginRequest(NewRequest);

	return true;
}

void UOnlineAuthSubsystem::ProcessLoginRequest(TSharedRef<FUserLoginRequest> Request)
{
	// User is gone, just delete this request

	auto* LocalUser{ Request->LocalUser.Get() };

	if (!LocalUser)
	{
		ActiveLoginRequests.Remove(Request);

		return;
	}

	// If the platform user id is invalid because this is a guest, skip right to failure

	const auto PlatformUserId{ LocalUser->PlatformUserId };

	if (!OnlineLocalUserManagerSubsystem->IsRealPlatformUser(PlatformUserId))
	{
		Request->Result = UE::Online::Errors::InvalidUser();

		// Remove from active array

		ActiveLoginRequests.Remove(Request);

		// Execute delegate if bound

		Request->Delegate.ExecuteIfBound(LocalUser, ELoginStatusType::NotLoggedIn, FUniqueNetIdRepl(), Request->Result, Request->DesiredContext);

		return;
	}

	// Figure out what context to process first

	if (Request->CurrentContext == EOnlineServiceContext::Invalid)
	{
		Request->CurrentContext = OnlineServiceSubsystem->ResolveOnlineServiceContext(Request->DesiredContext);
	}

	// Cache current infomations

	auto System{ OnlineServiceSubsystem->GetContextCache(Request->CurrentContext) };
	if (!ensure(System))
	{
		return;
	}

	auto AccountInfo{ LocalUser->GetCachedAccountInfo(Request->CurrentContext) };
	auto CurrentStatus{ AccountInfo ? AccountInfo->LoginStatus : ELoginStatus::NotLoggedIn };
	auto CurrentId{ AccountInfo ? AccountInfo->AccountId : FAccountId() };

	// Starting a new request

	if (Request->OverallLoginState == EOnlineServiceTaskState::NotStarted)
	{
		Request->OverallLoginState = EOnlineServiceTaskState::InProgress;
	}

	// If this is not an online required login, allow local profile to count as fully logged in

	bool bHasRequiredStatus = (CurrentStatus == ELoginStatusType::LoggedIn);
	if (Request->DesiredPrivilege == EOnlinePrivilege::CanPlay)
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

			if (TransferPlatformAuth(System, Request, PlatformUserId))
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

				if (AutoLogin(System, Request, PlatformUserId))
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

				if (ShowLoginUI(System, Request, PlatformUserId))
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

			auto CachedResult{ LocalUser->GetCachedPrivilegeResult(Request->DesiredPrivilege, Request->CurrentContext) };
			if (CachedResult == EOnlinePrivilegeResult::Available)
			{
				// Use cached success value
				Request->PrivilegeCheckState = EOnlineServiceTaskState::Done;
			}
			else
			{
				if (QueryLoginRequestedPrivilege(System, Request, PlatformUserId))
				{
					return;
				}
				else
				{
					CachedResult = EOnlinePrivilegeResult::Available;
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
			auto ResolvedDesiredContext{ OnlineServiceSubsystem->ResolveOnlineServiceContext(Request->DesiredContext) };

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

			if (Request->OverallLoginState == EOnlineServiceTaskState::Failed && Request->Result.ErrorId.IsEmpty())
			{
				Request->Result = UE::Online::Errors::RequestFailure();
			}

			// Remove from active array

			ActiveLoginRequests.Remove(Request);

			// Execute delegate if bound

			Request->Delegate.ExecuteIfBound(LocalUser, CurrentStatus, { CurrentId }, Request->Result, Request->DesiredContext);
		}
	}
}

void UOnlineAuthSubsystem::HandleAuthLoginStatusChanged(const FAuthLoginStatusChanged& EventParameters, EOnlineServiceContext Context)
{
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Player login status changed - System:%d, UserId:%s, NewStatus:%s"),
		static_cast<int32>(Context),
		*ToLogString(EventParameters.AccountInfo->AccountId),
		LexToString(EventParameters.LoginStatus));
}

void UOnlineAuthSubsystem::HandleLoginForUserInitialize(UOnlineLocalUserSubsystem* LocalUser, ELoginStatusType NewStatus, FUniqueNetIdRepl NetId, FOnlineServiceResult Result, EOnlineServiceContext Context, FLocalUserLoginParams Params)
{
	auto* GameInstance{ GetGameInstance() };
	check(GameInstance);

	auto& TimerManager{ GameInstance->GetTimerManager() };

	// Check local are users valid

	auto* PrimaryLocalUser{ OnlineLocalUserManagerSubsystem->GetUserInfoForLocalPlayerIndex(0) };

	if (!ensure(LocalUser && PrimaryLocalUser))
	{
		return;
	}

	// Check should be guest

	auto FirstPlayerId{ PrimaryLocalUser->GetNetId(Context) };

	if (LocalUser != PrimaryLocalUser && LocalUser->bCanBeGuest && (NewStatus == ELoginStatusType::NotLoggedIn || NetId == FirstPlayerId))
	{
		/// @TODO  OSSv2 FUniqueNetIdRepl wrapping FAccountId is in progress
		/// @TODO  OSSv2 - How to handle guest accounts?

		LocalUser->bIsGuest = true;
		NewStatus = ELoginStatusType::UsingLocalProfile;
		Result = FOnlineServiceResult();

		UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("HandleLoginForUserInitialize created guest id %s for local player %d"), *NetId.ToString(), LocalUser->GetLocalPlayer()->GetLocalPlayerIndex());
	}
	else
	{
		LocalUser->bIsGuest = false;
	}

	// Notify result on next tick

	if (Result.bWasSuccessful)
	{
		TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::HandleUserLoginSucceeded, LocalUser, Params, Result));
	}
	else
	{
		TimerManager.SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::HandleUserLoginFailed, LocalUser, Params, Result));
	}
}

void UOnlineAuthSubsystem::HandleUserLoginFailed(UOnlineLocalUserSubsystem* LocalUser, FLocalUserLoginParams Params, FOnlineServiceResult Result)
{
	// The user info was reset since this was scheduled

	if (!ensure(LocalUser))
	{
		return;
	}

	// If state is wrong, abort as we might have gotten canceled

	if (!ensure(LocalUser->IsDoingLogin()))
	{
		return;
	}

	UE_LOG(LogGameCore_OnlineAuth, Warning, TEXT("Try login failed: (Player: %d, Error: %s)"), LocalUser->GetLocalPlayer()->GetLocalPlayerIndex(), *Result.ErrorText.ToString());

	LocalUser->LoginState = ELocalUserLoginState::FailedToLogin;

	if (!Params.bSuppressLoginErrors)
	{
		OnlineServiceSubsystem->SendErrorMessage(Result);
	}

	// Call callbacks

	Params.OnLocalUserLoginComplete.ExecuteIfBound(LocalUser->GetLocalPlayer(), Result, Params.OnlineContext);
	OnUserLoginComplete.Broadcast(LocalUser->GetLocalPlayer(), Result, Params.OnlineContext);
}

void UOnlineAuthSubsystem::HandleUserLoginSucceeded(UOnlineLocalUserSubsystem* LocalUser, FLocalUserLoginParams Params, FOnlineServiceResult Result)
{
	// The user info was reset since this was scheduled

	if (!ensure(LocalUser))
	{
		return;
	}

	// If state is wrong, abort as we might have gotten canceled

	if (!ensure(LocalUser->IsDoingLogin()))
	{
		return;
	}

	UE_LOG(LogGameCore_OnlineAuth, Warning, TEXT("Try login Success: (Player: %d)"), LocalUser->GetLocalPlayer()->GetLocalPlayerIndex());

	if (Params.RequestedPrivilege == EOnlinePrivilege::CanPlayOnline)
	{
		LocalUser->LoginState = ELocalUserLoginState::LoggedInOnline;
	}
	else
	{
		LocalUser->LoginState = ELocalUserLoginState::LoggedInLocalOnly;
	}

	// Call callbacks

	Params.OnLocalUserLoginComplete.ExecuteIfBound(LocalUser->GetLocalPlayer(), Result, Params.OnlineContext);
	OnUserLoginComplete.Broadcast(LocalUser->GetLocalPlayer(), Result, Params.OnlineContext);
}


// Transfer Platform Auth

bool UOnlineAuthSubsystem::TransferPlatformAuth(IOnlineServicesPtr OnlineService, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	auto PlatformAuthInterface{ GetAuthInterface(EOnlineServiceContext::Platform) };

	if (PlatformAuthInterface && (Request->CurrentContext != EOnlineServiceContext::Platform))
	{
		UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Start Transfer Platform Auth"));

		FAuthQueryExternalAuthToken::Params Params;
		Params.LocalAccountId = GetLocalUserNetId(PlatformUser, EOnlineServiceContext::Platform);

		auto Handle{ PlatformAuthInterface->QueryExternalAuthToken(MoveTemp(Params)) };
		Handle.OnComplete(this, &ThisClass::HandleTransferPlatformAuth, Request.ToWeakPtr(), PlatformUser);

		return true;
	}

	return false;
}

void UOnlineAuthSubsystem::HandleTransferPlatformAuth(const TOnlineResult<FAuthQueryExternalAuthToken>& Result, TWeakPtr<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	auto RequestPtr{ Request.Pin() };
	if (!RequestPtr)
	{
		return;
	}

	// User is gone, just delete this request

	auto* LocalUser{ RequestPtr->LocalUser.Get() };
	if (!LocalUser)
	{
		ActiveLoginRequests.Remove(RequestPtr.ToSharedRef());
		return;
	}

	if (Result.IsOk())
	{
		const auto& GenerateAuthTokenResult{ Result.GetOkValue() };

		FAuthLogin::Params Params;
		Params.PlatformUserId = PlatformUser;
		Params.CredentialsType = LoginCredentialsType::ExternalAuth;
		Params.CredentialsToken.Emplace<FExternalAuthToken>(GenerateAuthTokenResult.ExternalAuthToken);

		auto PrimaryAuthInterface{ GetAuthInterface(RequestPtr->CurrentContext) };
		auto Handle{ PrimaryAuthInterface->Login(MoveTemp(Params)) };
		Handle.OnComplete(this, &ThisClass::HandlePlatformLoginComplete, Request, PlatformUser);
	}
	else
	{
		RequestPtr->TransferPlatformAuthState = EOnlineServiceTaskState::Failed;
		RequestPtr->Result = Result.GetErrorValue();
		ProcessLoginRequest(RequestPtr.ToSharedRef());
	}
}

void UOnlineAuthSubsystem::HandlePlatformLoginComplete(const TOnlineResult<FAuthLogin>& Result, TWeakPtr<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	auto RequestPtr{ Request.Pin() };
	if (!RequestPtr)
	{
		return;
	}

	// User is gone, just delete this request

	auto* LocalUser{ RequestPtr->LocalUser.Get() };
	if (!LocalUser)
	{
		ActiveLoginRequests.Remove(RequestPtr.ToSharedRef());
		return;
	}

	const auto bSuccess{ Result.IsOk() };
	const auto NewAccountInfo{ bSuccess ? Result.GetOkValue().AccountInfo.ToSharedPtr() : nullptr };

	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Player platform login Completed"));
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| Result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| Error: %s"), bSuccess ? TEXT("") : *Result.GetErrorValue().GetLogString());
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| Context: %s"), *StaticEnum<EOnlineServiceContext>()->GetDisplayValueAsText(RequestPtr->CurrentContext).ToString());
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| PlatformUserId: %d"), PlatformUser.GetInternalId());
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| AccountId: %s"), *ToLogString(NewAccountInfo ? NewAccountInfo->AccountId : FAccountId()));

	if (bSuccess)
	{
		RequestPtr->TransferPlatformAuthState = EOnlineServiceTaskState::Done;
		RequestPtr->Result = FOnlineServiceResult();
		LocalUser->UpdateCachedAccountInfo(NewAccountInfo, RequestPtr->CurrentContext);
	}
	else
	{
		RequestPtr->TransferPlatformAuthState = EOnlineServiceTaskState::Failed;
		RequestPtr->Result = Result.GetErrorValue();
	}

	ProcessLoginRequest(RequestPtr.ToSharedRef());
}


// Auto Login

bool UOnlineAuthSubsystem::AutoLogin(IOnlineServicesPtr OnlineService, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Start Auto Login"));

	FAuthLogin::Params LoginParameters;
	LoginParameters.PlatformUserId = PlatformUser;
	LoginParameters.CredentialsType = LoginCredentialsType::Auto;

	// Leave other LoginParameters as default to allow the online service to determine how to try to automatically log in the user

	auto LoginHandle{ OnlineService->GetAuthInterface()->Login(MoveTemp(LoginParameters))};
	LoginHandle.OnComplete(this, &ThisClass::HandleAutoLoginComplete, Request.ToWeakPtr(), PlatformUser);

	return true;
}

void UOnlineAuthSubsystem::HandleAutoLoginComplete(const TOnlineResult<FAuthLogin>& Result, TWeakPtr<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	auto RequestPtr{ Request.Pin() };
	if (!RequestPtr)
	{
		return;
	}

	// User is gone, just delete this request

	auto* LocalUser{ RequestPtr->LocalUser.Get() };
	if (!LocalUser)
	{
		ActiveLoginRequests.Remove(RequestPtr.ToSharedRef());
		return;
	}

	const auto bSuccess{ Result.IsOk() };
	const auto NewAccountInfo{ bSuccess ? Result.GetOkValue().AccountInfo.ToSharedPtr() : nullptr};

	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Player auto login Completed"));
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| Result: %s")		, bSuccess ? TEXT("Success") : TEXT("Failed"));
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| Error: %s")			, bSuccess ? TEXT("") : *Result.GetErrorValue().GetLogString());
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| Context: %s")		, *StaticEnum<EOnlineServiceContext>()->GetDisplayValueAsText(RequestPtr->CurrentContext).ToString());
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| PlatformUserId: %d"), PlatformUser.GetInternalId());
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| AccountId: %s")		, *ToLogString(NewAccountInfo ? NewAccountInfo->AccountId : FAccountId()));

	if (bSuccess)
	{
		RequestPtr->AutoLoginState = EOnlineServiceTaskState::Done;
		RequestPtr->Result = FOnlineServiceResult();
		LocalUser->UpdateCachedAccountInfo(NewAccountInfo, RequestPtr->CurrentContext);
	}
	else
	{
		RequestPtr->AutoLoginState = EOnlineServiceTaskState::Failed;
		RequestPtr->Result = Result.GetErrorValue();
	}

	ProcessLoginRequest(RequestPtr.ToSharedRef());
}


// Show Login UI

bool UOnlineAuthSubsystem::ShowLoginUI(IOnlineServicesPtr OnlineService, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	auto ExternalUI{ OnlineService->GetExternalUIInterface() };

	if (ExternalUI.IsValid())
	{
		UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Start Login with External UI"));

		FExternalUIShowLoginUI::Params ShowLoginUIParameters;
		ShowLoginUIParameters.PlatformUserId = PlatformUser;

		auto Handle{ ExternalUI->ShowLoginUI(MoveTemp(ShowLoginUIParameters)) };
		Handle.OnComplete(this, &ThisClass::HandleLoginUIClosed, Request.ToWeakPtr(), PlatformUser);

		return true;
	}

	return false;
}

void UOnlineAuthSubsystem::HandleLoginUIClosed(const TOnlineResult<FExternalUIShowLoginUI>& Result, TWeakPtr<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	auto RequestPtr{ Request.Pin() };
	if (!RequestPtr)
	{
		return;
	}

	// User is gone, just delete this request

	auto* LocalUser{ RequestPtr->LocalUser.Get() };
	if (!LocalUser)
	{
		ActiveLoginRequests.Remove(RequestPtr.ToSharedRef());

		return;
	}

	const auto bSuccess{ Result.IsOk() };
	const auto NewAccountInfo{ bSuccess ? Result.GetOkValue().AccountInfo.ToSharedPtr() : nullptr };

	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("Player login with External UI Completed"));
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| Result: %s")		, bSuccess ? TEXT("Success") : TEXT("Failed"));
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| Error: %s")			, bSuccess ? TEXT("") : *Result.GetErrorValue().GetLogString());
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| Context: %s")		, *StaticEnum<EOnlineServiceContext>()->GetDisplayValueAsText(RequestPtr->CurrentContext).ToString());
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| PlatformUserId: %d"), PlatformUser.GetInternalId());
	UE_LOG(LogGameCore_OnlineAuth, Log, TEXT("| AccountId: %s")		, *ToLogString(NewAccountInfo ? NewAccountInfo->AccountId : FAccountId()));

	if (bSuccess)
	{
		RequestPtr->LoginUIState = EOnlineServiceTaskState::Done;
		RequestPtr->Result = FOnlineServiceResult();
		LocalUser->UpdateCachedAccountInfo(NewAccountInfo, RequestPtr->CurrentContext);
	}
	else
	{
		RequestPtr->LoginUIState = EOnlineServiceTaskState::Failed;
		RequestPtr->Result = Result.GetErrorValue();
	}

	ProcessLoginRequest(RequestPtr.ToSharedRef());
}


// Privilege Check

bool UOnlineAuthSubsystem::QueryLoginRequestedPrivilege(IOnlineServicesPtr OnlineService, TSharedRef<FUserLoginRequest> Request, FPlatformUserId PlatformUser)
{
	const auto* GameInstance{ GetGameInstance() };
	ensure(GameInstance);

	if (auto* PrivilegeSubsystem{ UGameInstance::GetSubsystem<UOnlinePrivilegeSubsystem>(GameInstance) })
	{
		return PrivilegeSubsystem->QueryUserPrivilege(Request->LocalUser->GetLocalPlayer(), Request->CurrentContext, Request->DesiredPrivilege,
										FOnlinePrivilegeQueryDelegate::CreateUObject(this, &ThisClass::HandleCheckPrivilegesComplete));
	}

	return false;
}

void UOnlineAuthSubsystem::HandleCheckPrivilegesComplete(const ULocalPlayer* LocalPlayer, EOnlineServiceContext Context, EOnlinePrivilege DesiredPrivilege, EOnlinePrivilegeResult PrivilegeResult, FOnlineServiceResult ServiceResult)
{
	auto* CheckingLocalUser{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer) };

	// See if a login request is waiting on this

	auto RequestsCopy{ ActiveLoginRequests };
	for (auto& Request : RequestsCopy)
	{
		auto* LocalUser{ Request->LocalUser.Get() };
		if (!LocalUser)
		{
			ActiveLoginRequests.Remove(Request);
			continue;
		}

		if ((LocalUser == CheckingLocalUser) &&
			(Request->CurrentContext == Context) &&
			(Request->DesiredPrivilege == DesiredPrivilege) &&
			(Request->PrivilegeCheckState == EOnlineServiceTaskState::InProgress))
		{
			if (PrivilegeResult == EOnlinePrivilegeResult::Available)
			{
				Request->PrivilegeCheckState = EOnlineServiceTaskState::Done;
				Request->Result = FOnlineServiceResult();
			}
			else
			{
				Request->PrivilegeCheckState = EOnlineServiceTaskState::Failed;
				Request->Result = ServiceResult;
			}

			ProcessLoginRequest(Request);
			return;
		}
	}
}
