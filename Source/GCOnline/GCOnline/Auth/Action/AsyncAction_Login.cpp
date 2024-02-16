// Copyright (C) 2024 owoDra

#include "AsyncAction_Login.h"

#include "OnlineAuthSubsystem.h"
#include "OnlineLocalUserSubsystem.h"

#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_Login)


UAsyncAction_Login* UAsyncAction_Login::LoginForLocalPlay(UOnlineAuthSubsystem* Target, APlayerController* InPlayerController)
{
	auto* Action{ NewObject<UAsyncAction_Login>() };
	Action->RegisterWithGameInstance(Target);

	if (Target && Action->IsRegistered())
	{
		Action->Subsystem = Target;
		Action->PlayerController = InPlayerController ;

		Action->Params.RequestedPrivilege = EOnlinePrivilege::CanPlay;
	}
	else
	{
		Action->SetReadyToDestroy();
	}

	return Action;
}

UAsyncAction_Login* UAsyncAction_Login::LoginForOnlinePlay(UOnlineAuthSubsystem* Target, APlayerController* InPlayerController)
{
	auto* Action{ NewObject<UAsyncAction_Login>() };
	Action->RegisterWithGameInstance(Target);

	if (Target && Action->IsRegistered())
	{
		Action->Subsystem = Target;
		Action->PlayerController = InPlayerController;

		Action->Params.RequestedPrivilege = EOnlinePrivilege::CanPlayOnline;
	}
	else
	{
		Action->SetReadyToDestroy();
	}

	return Action;
}


void UAsyncAction_Login::Activate()
{
	if (Subsystem.IsValid())
	{
		Params.OnLocalUserLoginComplete.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UAsyncAction_Login, HandleInitializationComplete));

		if (!Subsystem->TryLogin(PlayerController.Get(), Params))
		{
			// Call failure next frame

			if (auto* TimerManager{ GetTimerManager() })
			{
				TimerManager->SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::HandleFailure));
			}
		}
	}
	else
	{
		SetReadyToDestroy();
	}
}

void UAsyncAction_Login::HandleFailure()
{
	auto* LocalPlayer{ PlayerController.IsValid() ? PlayerController->GetLocalPlayer() : nullptr };
	auto* LocalUser{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer) };

	FOnlineServiceResult Result;
	Result.bWasSuccessful = false;
	Result.ErrorId = TEXT("Login Failed Early");
	Result.ErrorText = NSLOCTEXT("GameOnlineCore", "LoginFailedEarly", "Unable to start login process");

	HandleInitializationComplete(LocalUser, Result, Params.OnlineContext);
}

void UAsyncAction_Login::HandleInitializationComplete(UOnlineLocalUserSubsystem* LocalUser, FOnlineServiceResult Result, EOnlineServiceContext OnlineContext)
{
	if (ShouldBroadcastDelegates())
	{
		OnInitializationComplete.Broadcast(PlayerController.Get(), Result, OnlineContext);
	}

	SetReadyToDestroy();
}
