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
	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere, Category = "Privileges", meta = (EditFixedSize, ReadOnlyKeys, ForceInlineRow))
	TMap<EOnlineServiceContext, FPrivilegesDescriptionSetting> PrivilegesDescriptions;

public:
	FText GetPrivilegesDescription(EOnlineServiceContext Context, EOnlinePrivilege Privilege) const;
	FText GetPrivilegesResultDescription(EOnlineServiceContext Context, EOnlinePrivilegeResult Result) const;


	///////////////////////////////////////////////
	// Lobbies
protected:
	//
	// Redirect list of Lobby attribute names
	// 
	// Tips:
	//	Redirect to the name used in the project when some online services, such as EOS, may change the name of the lobby attribute to a specific name.
	// 
	// Key	 : Name to be used for the project
	// Value : Name on online service
	//
	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere, Category = "Lobbies", meta = (ForceInlineRow))
	TMap<FName, FName> LobbyAttributeRedirects;

	//
	// Redirect list of Lobby user attribute names
	// 
	// Tips:
	//	Redirect to the name used in the project when some online services, such as EOS, may change the name of the lobby attribute to a specific name.
	// 
	// Key	 : Name to be used for the project
	// Value : Name on online service
	//
	UPROPERTY(Config, BlueprintReadOnly, EditAnywhere, Category = "Lobbies", meta = (ForceInlineRow))
	TMap<FName, FName> LobbyUserAttributeRedirects;

public:
	FName RedirectLobbyAttribute_ToOnlineService(const FName& InName) const;
	FName RedirectLobbyAttribute_ToProject(const FName& InName) const;

	FName RedirectUserLobbyAttribute_ToOnlineService(const FName& InName) const;
	FName RedirectUserLobbyAttribute_ToProject(const FName& InName) const;

};

