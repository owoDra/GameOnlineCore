// Copyright (C) 2024 owoDra

#include "OnlineLobbySearchTypes.h"

#include "OnlineDeveloperSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineLobbySearchTypes)


/////////////////////////////////////////////////////////////////
// ULobbySearchRequest

ULobbySearchRequest::ULobbySearchRequest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


FFindLobbies::Params ULobbySearchRequest::GenerateFindParameters() const
{
	const auto* DevSettings{ GetDefault<UOnlineDeveloperSettings>() };

	FFindLobbies::Params Prams;
	Prams.MaxResults = MaxResult;

	for (const auto& Filter : Filters)
	{
		auto FilterParam{ Filter.ToSearchFilter() };

		FilterParam.AttributeName = DevSettings->RedirectLobbyAttribute_ToOnlineService(FilterParam.AttributeName);

		Prams.Filters.Emplace(FilterParam);
	}

	return Prams;
}
