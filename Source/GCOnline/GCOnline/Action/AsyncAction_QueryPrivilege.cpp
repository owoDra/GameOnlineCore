// Copyright (C) 2024 owoDra

#include "AsyncAction_QueryPrivilege.h"

#include "OnlinePrivilegeSubsystem.h"

#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_QueryPrivilege)


UAsyncAction_QueryPrivilege* UAsyncAction_QueryPrivilege::QueryLocalUserPrivilege(UOnlinePrivilegeSubsystem* Target, ULocalUserAccountInfo* InTargetUser, EOnlineServiceContext InContext, ELocalUserPrivilege InDesiredPrivilege)
{
	auto* Action{ NewObject<UAsyncAction_QueryPrivilege>() };

	Action->RegisterWithGameInstance(Target);

	if (Target && Action->IsRegistered())
	{
		Action->Subsystem = Target;

		Action->TargetUser = InTargetUser;
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
		auto NewDelegate{ FLocalUserPrivilegeQueryDelegate::CreateUObject(this, &ThisClass::HandleQueryComplete) };

		Subsystem->QueryUserPrivilege(TargetUser, Context, DesiredPrivilege, NewDelegate);
	}
	else
	{
		SetReadyToDestroy();
	}
}

void UAsyncAction_QueryPrivilege::HandleQueryComplete(ULocalUserAccountInfo* InUserInfo, EOnlineServiceContext InContext, ELocalUserPrivilege InDesiredPrivilege, ELocalUserPrivilegeResult InPrivilegeResult, const TOptional<FOnlineErrorType>& Error)
{
	if (ShouldBroadcastDelegates())
	{
		OnQuery.Broadcast(InUserInfo, InContext, InDesiredPrivilege, InPrivilegeResult);
	}

	SetReadyToDestroy();
}
