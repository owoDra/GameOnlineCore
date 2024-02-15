// Copyright (C) 2024 owoDra

#include "AsyncAction_QueryPrivilege.h"

#include "OnlinePrivilegeSubsystem.h"

#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_QueryPrivilege)


UAsyncAction_QueryPrivilege* UAsyncAction_QueryPrivilege::QueryLocalUserPrivilege(UOnlinePrivilegeSubsystem* Target, const APlayerController* InPlayerController, EOnlineServiceContext InContext, EOnlinePrivilege InDesiredPrivilege)
{
	auto* Action{ NewObject<UAsyncAction_QueryPrivilege>() };

	Action->RegisterWithGameInstance(Target);

	if (Target && Action->IsRegistered())
	{
		Action->Subsystem = Target;

		Action->LocalPlayer = InPlayerController ? InPlayerController->GetLocalPlayer() : nullptr;
		Action->Context = InContext;
		Action->DesiredPrivilege = InDesiredPrivilege;
	}
	else
	{
		Action->SetReadyToDestroy();
	}

	return Action;
}


void UAsyncAction_QueryPrivilege::Activate()
{
	if (Subsystem.IsValid())
	{
		auto NewDelegate{ FOnlinePrivilegeQueryDelegate::CreateUObject(this, &ThisClass::HandleQueryComplete) };

		Subsystem->QueryUserPrivilege(LocalPlayer.Get(), Context, DesiredPrivilege, NewDelegate);
	}
	else
	{
		SetReadyToDestroy();
	}
}

void UAsyncAction_QueryPrivilege::HandleQueryComplete(const ULocalPlayer* InLocalPlayer, EOnlineServiceContext InContext, EOnlinePrivilege InDesiredPrivilege, EOnlinePrivilegeResult InPrivilegeResult, FOnlineServiceResult InResult)
{
	if (ShouldBroadcastDelegates())
	{
		OnQuery.Broadcast(InLocalPlayer, InContext, InDesiredPrivilege, InPrivilegeResult, InResult);
	}

	SetReadyToDestroy();
}
