// Copyright (C) 2024 owoDra

#include "OnlineLobbyAttributeTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OnlineLobbyAttributeTypes)


//////////////////////////////////////////////////////////////////////////////
// FLobbyAttribute

void FLobbyAttribute::SetAttribute(const FString& InValue)
{
	Type = ELobbyAttributeValueType::String;
	Value = InValue;
}

void FLobbyAttribute::SetAttribute(const int32& InValue)
{
	Type = ELobbyAttributeValueType::Integer;
	Value = FString::FromInt(InValue);
}

void FLobbyAttribute::SetAttribute(const double& InValue)
{
	Type = ELobbyAttributeValueType::Double;
	Value = FString::SanitizeFloat(InValue);
}

void FLobbyAttribute::SetAttribute(const bool& InValue)
{
	Type = ELobbyAttributeValueType::Boolean;
	Value = InValue ? TEXT("true") : TEXT("false");
}

void FLobbyAttribute::SetAttribute(const TArray<FString>& InValue)
{
	Type = ELobbyAttributeValueType::String;

	FString NewValue;
	for (const auto& Each : InValue)
	{
		NewValue += Each;
		NewValue += TEXT(";");
	}

	Value = NewValue;
}


FString FLobbyAttribute::GetAttributeAsString() const
{
	return Value;
}

int32 FLobbyAttribute::GetAttributeAsInteger() const
{
	return FCString::Atoi(*Value);
}

double FLobbyAttribute::GetAttributeAsDouble() const
{
	return FCString::Atod(*Value);
}

bool FLobbyAttribute::GetAttributeAsBoolean() const
{
	return Value.ToBool();
}


FSchemaVariant FLobbyAttribute::ToSchemaVariant() const
{
	switch (Type)
	{
	case ELobbyAttributeValueType::String:
		return FSchemaVariant(GetAttributeAsString());
		break;
	case ELobbyAttributeValueType::Integer:
		return FSchemaVariant(static_cast<int64>(GetAttributeAsInteger()));
		break;
	case ELobbyAttributeValueType::Double:
		return FSchemaVariant(GetAttributeAsDouble());
		break;
	case ELobbyAttributeValueType::Boolean:
		return FSchemaVariant(GetAttributeAsBoolean());
		break;

	default:
		return FSchemaVariant();
		break;
	}
}


//////////////////////////////////////////////////////////////////////////////
// FLobbyAttributeFilter

FFindLobbySearchFilter FLobbyAttributeFilter::ToSearchFilter() const
{
	return FFindLobbySearchFilter(Attribute.GetAttributeName(), static_cast<ESchemaAttributeComparisonOp>(ComparisonOp), Attribute.ToSchemaVariant());
}
