// Copyright (C) 2024 owoDra

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "Online/Lobbies.h"

#include "OnlineLobbyAttributeTypes.generated.h"

using namespace UE::Online;


/////////////////////////////////////////////////////
// Enums

/**
 * Lobby Attribute Comparison Type
 * 
 * Tips:
 *	Same as ESchemaAttributeComparisonOp but has blueprint accessibility
 */
UENUM(BlueprintType)
enum class ELobbyAttributeComparisonOp : uint8
{
	Equals,
	NotEquals,
	GreaterThan,
	GreaterThanEquals,
	LessThan,
	LessThanEquals,
	Near,
	In,
	NotIn
};


/**
 * Lobby Attribute Value Type
 */
UENUM(BlueprintType)
enum class ELobbyAttributeValueType : uint8
{
	String,
	Integer,
	Double,
	Boolean
};


/////////////////////////////////////////////////////
// Structs

/**
 * Data for modifying lobby attributes
 */
USTRUCT(BlueprintType)
struct FLobbyAttribute
{
	GENERATED_BODY()
public:
	FLobbyAttribute() = default;
	FLobbyAttribute(const FName& InName) : Name(InName) {}
	FLobbyAttribute(const FName& InName, const FString& InValue) : Name(InName) { SetAttribute(InValue); }
	FLobbyAttribute(const FName& InName, const int32& InValue) : Name(InName) { SetAttribute(InValue); }
	FLobbyAttribute(const FName& InName, const double& InValue) : Name(InName) { SetAttribute(InValue); }
	FLobbyAttribute(const FName& InName, const bool& InValue) : Name(InName) { SetAttribute(InValue); }
	FLobbyAttribute(const FName& InName, const TArray<FString>& InValue) : Name(InName) { SetAttribute(InValue); }

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Value;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ELobbyAttributeValueType Type{ ELobbyAttributeValueType::String };

public:
	void SetAttributeName(const FName& InName) { Name = InName; }

	const FName& GetAttributeName() const { return Name; }
	const ELobbyAttributeValueType& GetValueType() const { return Type; }

	void SetAttribute(const FString& InValue);
	void SetAttribute(const int32& InValue);
	void SetAttribute(const double& InValue);
	void SetAttribute(const bool& InValue);
	void SetAttribute(const TArray<FString>& InValue);

	FString GetAttributeAsString() const;
	int32 GetAttributeAsInteger() const;
	double GetAttributeAsDouble() const;
	bool GetAttributeAsBoolean() const;

	FSchemaVariant ToSchemaVariant() const;

public:
	friend FORCEINLINE uint32 GetTypeHash(const FLobbyAttribute& Attr) { return GetTypeHash(Attr.Name); }

};

UCLASS(MinimalAPI)
class ULobbyAttributeLibrary : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Lobby Attribute")
	static GCONLINE_API void SetAttributeFromString(UPARAM(ref) FLobbyAttribute& Attribute, FString InValue) { Attribute.SetAttribute(InValue); }

	UFUNCTION(BlueprintCallable, Category = "Lobby Attribute")
	static GCONLINE_API void SetAttributeFromInteger(UPARAM(ref) FLobbyAttribute& Attribute, int32 InValue) { Attribute.SetAttribute(InValue); }

	UFUNCTION(BlueprintCallable, Category = "Lobby Attribute")
	static GCONLINE_API void SetAttributeFromDouble(UPARAM(ref) FLobbyAttribute& Attribute, double InValue) { Attribute.SetAttribute(InValue); }

	UFUNCTION(BlueprintCallable, Category = "Lobby Attribute")
	static GCONLINE_API void SetAttributeFromBoolean(UPARAM(ref) FLobbyAttribute& Attribute, bool InValue) { Attribute.SetAttribute(InValue); }

};


/**
 * Data used to filter lobby attributes
 */
USTRUCT(BlueprintType)
struct FLobbyAttributeFilter
{
	GENERATED_BODY()
public:
	FLobbyAttributeFilter() = default;
	FLobbyAttributeFilter(const FLobbyAttribute& InAttr, const ELobbyAttributeComparisonOp& InOp)
		: Attribute(InAttr), ComparisonOp(InOp)
	{}

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FLobbyAttribute Attribute;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ELobbyAttributeComparisonOp ComparisonOp{ ELobbyAttributeComparisonOp::Equals };

public:
	friend FORCEINLINE uint32 GetTypeHash(const FLobbyAttributeFilter& Filter) { return GetTypeHash(Filter.Attribute); }

	FFindLobbySearchFilter ToSearchFilter() const;

};
