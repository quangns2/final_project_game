// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CloBlueprintXmlTypes.h"
#include "CloBlueprintXmlLibrary.generated.h"

class UCloXmlFileWrapper;

/**
 * Library for loading and handling XML files in Blueprint.
 */
UCLASS()
class CLOBLUEPRINTXMLPARSER_API UCloBlueprintXmlLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Loads an XML file from the specified path or parses an XML string.
     * @param FilePathOrBuffer - Either the file path or the XML string content.
     * @param ConstructMethod - Specifies whether FilePathOrBuffer is a path or a string buffer.
     * @param bSuccess - Output bool indicating if loading was successful.
     * @param ErrorMessage - Output string containing error details if loading failed.
     * @return A UCloXmlFileWrapper object if successful, nullptr otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "XML", meta = (DisplayName = "Load XML"))
    static UCloXmlFileWrapper* LoadXml(const FString& FilePathOrBuffer, ECloBlueprintXmlConstructMethod ConstructMethod, bool& bSuccess, FString& ErrorMessage);
};