// Copyright (C) 2024 owoDra

#include "AsyncAction_Login.h"

#include "OnlineUserSubsystem.h"

#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_Login)


UAsyncAction_Login* UAsyncAction_Login::LoginForLocalPlay(UOnlineUserSubsystem* Target, int32 LocalPlayerIndex, FInputDeviceId PrimaryInputDevice, bool bCanUseGuestLogin)
{
	// Set to default device

	if (!PrimaryInputDevice.IsValid())
	{
		PrimaryInputDevice = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
	}

	auto* Action{ NewObject<UAsyncAction_Login>() };
	Action->RegisterWithGameInstance(Target);

	if (Target && Action->IsRegistered())
	{
		Action->Subsystem = Target;

		Action->Params.RequestedPrivilege = ELocalUserPrivilege::CanPlay;
		Action->Params.LocalPlayerIndex = LocalPlayerIndex;
		Action->Params.PrimaryInputDevice = PrimaryInputDevice;
		Action->Params.bCanUseGuestLogin = bCanUseGuestLogin;
		Action->Params.bCanCreateNewLocalPlayer = true;
	}
	else
	{
		Action->SetReadyToDestroy();
	}

	return Action;
}

UAsyncAction_Login* UAsyncAction_Login::LoginForOnlinePlay(UOnlineUserSubsystem* Target, int32 LocalPlayerIndex)
{
	auto* Action{ NewObject<UAsyncAction_Login>() };

	Action->RegisterWithGameInstance(Target);

	if (Target && Action->IsRegistered())
	{
		Action->Subsystem = Target;

		Action->Params.RequestedPrivilege = ELocalUserPrivilege::CanPlayOnline;
		Action->Params.LocalPlayerIndex = LocalPlayerIndex;
		Action->Params.bCanCreateNewLocalPlayer = false;
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

		if (!Subsystem->TryGenericLogin(Params))
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
	const ULocalUserAccountInfo* UserInfo{ nullptr };

	if (Subsystem.IsValid())
	{
		UserInfo = Subsystem->GetUserInfoForLocalPlayerIndex(Params.LocalPlayerIndex);
	}

	HandleInitializationComplete(UserInfo, false, NSLOCTEXT("GameOnlineCore", "LoginFailedEarly", "Unable to start login process"), Params.RequestedPrivilege, Params.OnlineContext);
}

void UAsyncAction_Login::HandleInitializationComplete(const ULocalUserAccountInfo* UserInfo, bool bSuccess, FText Error, ELocalUserPrivilege RequestedPrivilege, EOnlineServiceContext OnlineContext)
{
	if (ShouldBroadcastDelegates())
	{
		OnInitializationComplete.Broadcast(UserInfo, bSuccess, Error, RequestedPrivilege, OnlineContext);
	}

	SetReadyToDestroy();
}
