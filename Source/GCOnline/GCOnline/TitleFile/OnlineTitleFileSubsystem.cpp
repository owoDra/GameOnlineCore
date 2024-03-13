// Copyright (C) 2024 owoDra

#include "OnlineTitleFileSubsystem.h"

#include "OnlineDeveloperSettings.h"
#include "OnlineServiceSubsystem.h"
#include "OnlineLocalUserSubsystem.h"
#include "GCOnlineLogs.h"

// OSS v2
#include "Online/OnlineResult.h"
#include "Online/OnlineServices.h"
#include "Online/OnlineServicesEngineUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineTitleFileSubsystem)


// Initialization

void UOnlineTitleFileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	OnlineServiceSubsystem = Collection.InitializeDependency<UOnlineServiceSubsystem>();

	check(OnlineServiceSubsystem);

	BindTitleFileDelegates();
}

void UOnlineTitleFileSubsystem::Deinitialize()
{
	OnlineServiceSubsystem = nullptr;

	UnbindTitleFileDelegates();
}

bool UOnlineTitleFileSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);

	// Only create an instance if there is not a game-specific subclass

	return ChildClasses.Num() == 0;
}


void UOnlineTitleFileSubsystem::BindTitleFileDelegates()
{
}

void UOnlineTitleFileSubsystem::UnbindTitleFileDelegates()
{
}

ITitleFilePtr UOnlineTitleFileSubsystem::GetTitleFileInterface(EOnlineServiceContext Context) const
{
	if (!OnlineServiceSubsystem->IsOnlineServiceReady())
	{
		return nullptr;
	}

	auto OnlineService{ OnlineServiceSubsystem->GetContextCache() };

	if (ensure(OnlineService))
	{
		return OnlineService->GetTitleFileInterface();
	}

	return nullptr;
}


// Enumerate Files

TArray<FString> UOnlineTitleFileSubsystem::GetEnumerateFiles(const APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		UE_LOG(LogGameCore_OnlineTitleFile, Error, TEXT("Get Enumerate Files Failed: Invalid Player Controller"));
		return TArray<FString>();
	}

	auto* LocalPlayer{ PlayerController->GetLocalPlayer() };
	if (!LocalPlayer)
	{
		UE_LOG(LogGameCore_OnlineTitleFile, Error, TEXT("Get Enumerate Files Failed: Can't get LocalPlayer from PlayerController(%s)"), *GetNameSafe(PlayerController));
		return TArray<FString>();
	}

	const auto AccountId{ LocalPlayer->GetPreferredUniqueNetId().GetV2() };
	if (!AccountId.IsValid())
	{
		UE_LOG(LogGameCore_OnlineTitleFile, Error, TEXT("Get Enumerate Files Failed: Invalid AccountId from LocalPlayer(%s)"), *GetNameSafe(LocalPlayer));
		return TArray<FString>();
	}

	auto TitleFile{ GetTitleFileInterface() };
	check(TitleFile);

	FTitleFileGetEnumeratedFiles::Params Param;
	Param.LocalAccountId = AccountId;

	auto Result{ TitleFile->GetEnumeratedFiles(MoveTemp(Param)) };

	if (Result.IsOk())
	{
		return TitleFile->GetEnumeratedFiles(MoveTemp(Param)).GetOkValue().Filenames;
	}

	return TArray<FString>();
}

bool UOnlineTitleFileSubsystem::EnumerateFiles(const APlayerController* PlayerController, FEnumerateFilesCompleteDelegate Delegate)
{
	if (!PlayerController)
	{
		UE_LOG(LogGameCore_OnlineTitleFile, Error, TEXT("Enumerate Files Failed: Invalid Player Controller"));
		return false;
	}

	auto* LocalPlayer{ PlayerController->GetLocalPlayer() };
	if (!LocalPlayer)
	{
		UE_LOG(LogGameCore_OnlineTitleFile, Error, TEXT("Enumerate Files Failed: Can't get LocalPlayer from PlayerController(%s)"), *GetNameSafe(PlayerController));
		return false;
	}

	const auto AccountId{ LocalPlayer->GetPreferredUniqueNetId().GetV2() };
	if (!AccountId.IsValid())
	{
		UE_LOG(LogGameCore_OnlineTitleFile, Error, TEXT("Enumerate Files Failed: Invalid AccountId from LocalPlayer(%s)"), *GetNameSafe(LocalPlayer));
		return false;
	}

	EnumerateFilesInternal(PlayerController, Delegate);
	return true;
}

void UOnlineTitleFileSubsystem::EnumerateFilesInternal(const APlayerController* PlayerController, FEnumerateFilesCompleteDelegate Delegate)
{
	check(PlayerController);
	auto* LocalPlayer{ PlayerController->GetLocalPlayer() };
	check(LocalPlayer);
	ensure(Delegate.IsBound());

	auto TitleFile{ GetTitleFileInterface() };
	check(TitleFile);

	FTitleFileEnumerateFiles::Params Param;
	Param.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();

	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("Start Enumerate Files"));
	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("| AccountId: %s"), *ToLogString(Param.LocalAccountId));

	auto Handle{ TitleFile->EnumerateFiles(MoveTemp(Param)) };
	Handle.OnComplete(this, &ThisClass::HandleEnumerateFilesComplete, PlayerController, Delegate);
}

void UOnlineTitleFileSubsystem::HandleEnumerateFilesComplete(const TOnlineResult<FTitleFileEnumerateFiles>& EnumerateResult, const APlayerController* PlayerController, FEnumerateFilesCompleteDelegate Delegate)
{
	if (!PlayerController)
	{
		return;
	}

	const auto bSuccess{ EnumerateResult.IsOk() };
	
	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("Enumerate Files Completed"));
	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("| Result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));
	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("| Error: %s"), bSuccess ? TEXT("") : *EnumerateResult.GetErrorValue().GetLogString());

	FOnlineServiceResult ServiceResult;

	if (!bSuccess)
	{
		ServiceResult = FOnlineServiceResult(EnumerateResult.GetErrorValue());
	}

	if (Delegate.IsBound())
	{
		Delegate.Execute(GetEnumerateFiles(PlayerController), ServiceResult);
	}
}


// Read File

bool UOnlineTitleFileSubsystem::ReadFile(const APlayerController* PlayerController, const FString& Filename, FReadFileCompleteDelegate Delegate)
{
	if (Filename.IsEmpty())
	{
		UE_LOG(LogGameCore_OnlineTitleFile, Error, TEXT("Read File Failed: Invalid Filename"));
		return false;
	}

	if (!PlayerController)
	{
		UE_LOG(LogGameCore_OnlineTitleFile, Error, TEXT("Read File Failed: Invalid Player Controller"));
		return false;
	}

	auto* LocalPlayer{ PlayerController->GetLocalPlayer() };
	if (!LocalPlayer)
	{
		UE_LOG(LogGameCore_OnlineTitleFile, Error, TEXT("Read File Failed: Can't get LocalPlayer from PlayerController(%s)"), *GetNameSafe(PlayerController));
		return false;
	}

	const auto AccountId{ LocalPlayer->GetPreferredUniqueNetId().GetV2() };
	if (!AccountId.IsValid())
	{
		UE_LOG(LogGameCore_OnlineTitleFile, Error, TEXT("Read File Failed: Invalid AccountId from LocalPlayer(%s)"), *GetNameSafe(LocalPlayer));
		return false;
	}

	ReadFileInternal(PlayerController, Filename, Delegate);
	return true;
}

void UOnlineTitleFileSubsystem::ReadFileInternal(const APlayerController* PlayerController, const FString& Filename, FReadFileCompleteDelegate Delegate)
{
	check(PlayerController);
	auto* LocalPlayer{ PlayerController->GetLocalPlayer() };
	check(LocalPlayer);
	ensure(Delegate.IsBound());

	auto TitleFile{ GetTitleFileInterface() };
	check(TitleFile);

	FTitleFileReadFile::Params Param;
	Param.LocalAccountId = LocalPlayer->GetPreferredUniqueNetId().GetV2();
	Param.Filename = Filename;

	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("Start Read File"));
	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("| AccountId: %s"), *ToLogString(Param.LocalAccountId));
	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("| Filename: %s"), *Param.Filename);

	auto Handle{ TitleFile->ReadFile(MoveTemp(Param)) };
	Handle.OnComplete(this, &ThisClass::HandleReadFileComplete, Delegate);
}

void UOnlineTitleFileSubsystem::HandleReadFileComplete(const TOnlineResult<FTitleFileReadFile>& ReadResult, FReadFileCompleteDelegate Delegate)
{
	ensure(Delegate.IsBound());

	const auto bSuccess{ ReadResult.IsOk() };

	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("Read File Completed"));
	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("| Result: %s"), bSuccess ? TEXT("Success") : TEXT("Failed"));
	UE_LOG(LogGameCore_OnlineTitleFile, Log, TEXT("| Error: %s"), bSuccess ? TEXT("") : *ReadResult.GetErrorValue().GetLogString());

	FOnlineServiceResult ServiceResult;

	if (bSuccess)
	{
		Delegate.ExecuteIfBound(ReadResult.GetOkValue().FileContents.Get(), ServiceResult);
	}
	else
	{
		ServiceResult = FOnlineServiceResult(ReadResult.GetErrorValue());

		Delegate.ExecuteIfBound(TArray<uint8>(), ServiceResult);
	}
}
