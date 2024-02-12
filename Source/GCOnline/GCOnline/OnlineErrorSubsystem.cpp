// Copyright (C) 2024 owoDra

#include "OnlineErrorSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineErrorSubsystem)


// Initialization

bool UOnlineErrorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}


// System Message

void UOnlineErrorSubsystem::SendSystemMessage(FGameplayTag MessageType, FText TitleText, FText BodyText)
{
	OnOnlineServiceSystemMessage.Broadcast(MessageType, TitleText, BodyText);
}
