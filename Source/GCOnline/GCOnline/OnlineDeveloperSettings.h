// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/DeveloperSettings.h"

#include "Type/OnlineServiceContextTypes.h"
#include "Type/OnlinePrivilegeTypes.h"

#include "OnlineDeveloperSettings.generated.h"


/**
 * Description of the user's privileges with respect to the online service
 */
USTRUCT(BlueprintType)
struct FPrivilegesDescriptionSetting
{
	GENERATED_BODY()
public:
	FPrivilegesDescriptionSetting();

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditFixedSize, ReadOnlyKeys, ForceInlineRow))
	TMap<EOnlinePrivilege, FText> PrivilegeDescriptions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (EditFixedSize, ReadOnlyKeys, ForceInlineRow))
	TMap<EOnlinePrivilegeResult, FText> PrivilegeResultDescriptions;
};


/**
 * Settings for a Game framework.
 */
UCLASS(Config = "Game", Defaultconfig, meta = (DisplayName = "Game Online Core"))
class GCONLINE_API UOnlineDeveloperSettings : public UDeveloperSettings
{
public:
	GENERATED_BODY()
public:
	UOnlineDeveloperSettings();


	///////////////////////////////////////////////
	// Privileges
protected:
	//
	// Description of the user's privileges with respect to the online service
	//
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Privileges", meta = (EditFixedSize, ReadOnlyKeys, ForceInlineRow))
	TMap<EOnlineServiceContext, FPrivilegesDescriptionSetting> PrivilegesDescriptions;

public:
	FText GetPrivilegesDescription(EOnlineServiceContext Context, EOnlinePrivilege Privilege) const;
	FText GetPrivilegesResultDescription(EOnlineServiceContext Context, EOnlinePrivilegeResult Result) const;

};

