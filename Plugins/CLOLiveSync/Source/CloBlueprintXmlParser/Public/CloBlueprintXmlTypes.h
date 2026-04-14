// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CloBlueprintXmlTypes.generated.h"

UENUM(BlueprintType)
enum class ECloBlueprintXmlConstructMethod : uint8
{
    ConstructFromFile UMETA(DisplayName = "From File"),
    ConstructFromBuffer UMETA(DisplayName = "From Buffer")
};

USTRUCT(BlueprintType)
struct FCloBlueprintXmlAttribute
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "XML Attribute")
    FString Tag;

    UPROPERTY(BlueprintReadOnly, Category = "XML Attribute")
    FString Value;

    FCloBlueprintXmlAttribute() = default;

    FCloBlueprintXmlAttribute(const FString& InTag, const FString& InValue)
        : Tag(InTag), Value(InValue)
    {}
};