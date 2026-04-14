// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "XmlFile.h"
#include "CloXmlFileWrapper.generated.h"

class UCloXmlNodeWrapper;

/**
 * Wraps an FXmlFile for Blueprint access. Owns the underlying FXmlFile via TSharedPtr.
 */
UCLASS(BlueprintType, Blueprintable)
class CLOBLUEPRINTXMLPARSER_API UCloXmlFileWrapper : public UObject
{
    GENERATED_BODY()

private:
    TSharedPtr<FXmlFile> XmlFilePtr;
    FString LastErrorMessage;

public:
    /** Initializes the wrapper with the actual FXmlFile. Internal use only. */
    void Initialize(TSharedPtr<FXmlFile> InXmlFile);

    /** Checks if the underlying FXmlFile was loaded successfully and is valid. */
    UFUNCTION(BlueprintPure, Category = "XML File")
    bool IsFileValid() const;

    /** Gets the root node of the XML file. Returns null if the file is not valid or has no root node. */
    UFUNCTION(BlueprintPure, Category = "XML File")
    UCloXmlNodeWrapper* GetRootNode();

    /** Gets the last error message generated during loading or operations. */
    UFUNCTION(BlueprintPure, Category = "XML File")
    FString GetLastError() const;

    /** Clears the loaded XML data. Makes any existing Node Wrappers invalid. */
    UFUNCTION(BlueprintCallable, Category = "XML File")
    void Clear();
};