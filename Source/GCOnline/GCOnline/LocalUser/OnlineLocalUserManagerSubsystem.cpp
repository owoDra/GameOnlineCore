// Copyright (C) 2024 owoDra

#include "OnlineLocalUserManagerSubsystem.h"

#include "OnlineLocalUserSubsystem.h"
#include "GCOnlineLogs.h"

#include "Engine/LocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineLocalUserManagerSubsystem)


// Initialization

void UOnlineLocalUserManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	SetMaxLocalUsers(MAX_LOCAL_PLAYERS);
}

void UOnlineLocalUserManagerSubsystem::Deinitialize()
{
}

bool UOnlineLocalUserManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (Cast<UGameInstance>(Outer)->IsDedicatedServerInstance())
	{
		return false;
	}

	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}


// Max Login User

void UOnlineLocalUserManagerSubsystem::SetMaxLocalUsers(int32 InMaxLoginPlayers)
{
	if (ensure(InMaxLoginPlayers >= 1))
	{
		// We can have more local players than MAX_LOCAL_PLAYERS, the rest are treated as guests

		MaxLocalUsers = InMaxLoginPlayers;

		auto* GameInstance{ GetGameInstance() };
		auto* ViewportClient = GameInstance ? GameInstance->GetGameViewportClient() : nullptr;

		if (ViewportClient)
		{
			ViewportClient->MaxSplitscreenPlayers = MaxLocalUsers;
		}
	}
}

int32 UOnlineLocalUserManagerSubsystem::GetMaxLocalUsers() const
{
	return MaxLocalUsers;
}

int32 UOnlineLocalUserManagerSubsystem::GetNumLocalPlayers() const
{
	auto* GameInstance{ GetGameInstance() };

	if (ensure(GameInstance))
	{
		return GameInstance->GetNumLocalPlayers();
	}

	return 1;
}


UOnlineLocalUserSubsystem* UOnlineLocalUserManagerSubsystem::GetUserInfoForLocalPlayerIndex(int32 LocalPlayerIndex) const
{
	if (auto* GameInstance{ GetGameInstance() })
	{
		return ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(GameInstance->GetLocalPlayerByIndex(LocalPlayerIndex));
	}

	return nullptr;
}

UOnlineLocalUserSubsystem* UOnlineLocalUserManagerSubsystem::GetUserInfoForPlatformUser(FPlatformUserId PlatformUser) const
{
	if (!IsRealPlatformUser(PlatformUser))
	{
		return nullptr;
	}

	if (auto* GameInstance{ GetGameInstance() })
	{
		for (auto It{ GameInstance->GetLocalPlayerIterator() }; It; ++It)
		{
			// Don't include guest users

			auto LocalPlayer{ *It };

			if (ensure(LocalPlayer) && LocalPlayer->GetPlatformUserId() == PlatformUser)
			{
				auto* LocalUser{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer) };

				if (ensure(LocalUser) && !LocalUser->bIsGuest)
				{
					return LocalUser;
				}
			}
		}
	}

	return nullptr;
}

UOnlineLocalUserSubsystem* UOnlineLocalUserManagerSubsystem::GetUserInfoForUniqueNetId(const FUniqueNetIdRepl& NetId) const
{
	if (!NetId.IsValid())
	{
		return nullptr;
	}

	if (auto* GameInstance{ GetGameInstance() })
	{
		for (auto It{ GameInstance->GetLocalPlayerIterator() }; It; ++It)
		{
			auto LocalPlayer{ *It };

			if (ensure(LocalPlayer) && LocalPlayer->GetPreferredUniqueNetId() == NetId)
			{
				return ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer);
			}
		}
	}

	return nullptr;
}

UOnlineLocalUserSubsystem* UOnlineLocalUserManagerSubsystem::GetUserInfoForInputDevice(FInputDeviceId InputDevice) const
{
	return GetUserInfoForPlatformUser(GetPlatformUserIdForInputDevice(InputDevice));
}


bool UOnlineLocalUserManagerSubsystem::IsRealPlatformUser(FPlatformUserId PlatformUser) const
{
	// Validation is done at conversion/allocation time so trust the type

	if (!PlatformUser.IsValid())
	{
		return false;
	}

	// Only the default user is supports online functionality 

	if (PlatformUser != IPlatformInputDeviceMapper::Get().GetPrimaryPlatformUser())
	{
		return false;
	}
	
	return true;
}

FPlatformUserId UOnlineLocalUserManagerSubsystem::GetPlatformUserIdForInputDevice(FInputDeviceId InputDevice) const
{
	return IPlatformInputDeviceMapper::Get().GetUserForInputDevice(InputDevice);
}

FInputDeviceId UOnlineLocalUserManagerSubsystem::GetPrimaryInputDeviceForPlatformUser(FPlatformUserId PlatformUser) const
{
	return IPlatformInputDeviceMapper::Get().GetPrimaryInputDeviceForUser(PlatformUser);
}


// Local User State

void UOnlineLocalUserManagerSubsystem::ResetAllLocalUserStates(bool bDestroyPlayer)
{
	if (auto* GameInstance{ GetGameInstance() })
	{
		TArray<ULocalPlayer*> LocalPlayersToDestroy;

		for (auto It{ GameInstance->GetLocalPlayerIterator() }; It; ++It)
		{
			auto* LocalPlayer{ *It };
			auto* LocalUser{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer) };

			if (ensure(LocalUser))
			{
				LocalUser->ResetLocalUser();
			}

			// Remove but the primary local player.

			if (bDestroyPlayer && !LocalPlayer->IsPrimaryPlayer())
			{
				LocalPlayersToDestroy.Add(LocalPlayer);
			}
		}

		for (auto* LocalPlayer : LocalPlayersToDestroy)
		{
			GameInstance->RemoveLocalPlayer(LocalPlayer);
		}
	}
}

bool UOnlineLocalUserManagerSubsystem::InitializeLocalUser(int32 LocaPlayerIndex, FInputDeviceId InPrimaryInputDevice, bool bCanUseGuestLogin)
{
	if (LocaPlayerIndex < 0 || LocaPlayerIndex >= GetMaxLocalUsers())
	{
		UE_LOG(LogGameCore_LocalUser, Warning, TEXT("InitializeLocalUser: Invalid Local player index(%d, MAX: %d)"), LocaPlayerIndex, GetMaxLocalUsers());
		return false;
	}

	auto* GameInstance{ GetGameInstance() };
	if (!GameInstance)
	{
		UE_LOG(LogGameCore_LocalUser, Error, TEXT("InitializeLocalUser: Invalid GameInstance"));
		return false;
	}

	// Get or create Local player

	auto* LocalPlayer{ GameInstance->GetLocalPlayerByIndex(LocaPlayerIndex) };
	if (!LocalPlayer)
	{
		FString ErrorString;

		if (!InPrimaryInputDevice.IsValid())
		{
			InPrimaryInputDevice = IPlatformInputDeviceMapper::Get().GetDefaultInputDevice();
		}

		LocalPlayer = GameInstance->CreateLocalPlayer(IPlatformInputDeviceMapper::Get().GetUserForInputDevice(InPrimaryInputDevice), ErrorString, true);

		if (!LocalPlayer)
		{
			UE_LOG(LogGameCore_LocalUser, Error, TEXT("InitializeLocalUser: Failed to create local player(Error: %s)"), *ErrorString);
			return false;
		}
	}

	// Initialize as local user

	auto* LocalUser{ ULocalPlayer::GetSubsystem<UOnlineLocalUserSubsystem>(LocalPlayer) };
	check(LocalUser);

	LocalUser->InitializeLocalUser(InPrimaryInputDevice, bCanUseGuestLogin);

	return true;
}
