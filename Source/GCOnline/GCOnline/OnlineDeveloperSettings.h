// Copyright (C) 2024 owoDra

#pragma once

#include "Engine/DeveloperSettings.h"

#include "Type/OnlineServiceTypes.h"
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
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ReadOnlyKeys, ForceInlineRow))
	TMap<ELocalUserPrivilege, FText> PrivilegeDescriptions;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ReadOnlyKeys, ForceInlineRow))
	TMap<ELocalUserPrivilegeResult, FText> PrivilegeResultDescriptions;
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
	// Online Services
public:
	//
	// List of online services to be used for addition
	// 
	// Tips:
	//	If not specified, it only accesses the project's default online services and platform-specific online services if they exist.
	//
	//UPROPERTY(Config, EditAnywhere, Category = "Online Services", meta = (NoElementDuplicate))
	//TArray<EOnlineServiceContext> ExtraOnlineServices;

	///////////////////////////////////////////////
	// Privileges
protected:
	//
	// Description of the user's privileges with respect to the online service
	//
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Privileges", meta = (ReadOnlyKeys, ForceInlineRow))
	TMap<EOnlineServiceContext, FPrivilegesDescriptionSetting> PrivilegesDescriptions;

public:
	FText GetPrivilegesDescription(EOnlineServiceContext Context, ELocalUserPrivilege Privilege) const;
	FText GetPrivilegesResultDescription(EOnlineServiceContext Context, ELocalUserPrivilegeResult Result) const;

};

