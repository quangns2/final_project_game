// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CloBlueprintXmlTypes.h"
#include "CloXmlNodeWrapper.generated.h"

class FXmlNode;
class UCloXmlFileWrapper;

/**
 * Wraps an FXmlNode pointer for Blueprint access.
 * NOTE: This object does NOT own the FXmlNode pointer. Its lifetime is tied to the UCloXmlFileWrapper that created it.
 */
UCLASS(BlueprintType, Blueprintable)
class CLOBLUEPRINTXMLPARSER_API UCloXmlNodeWrapper : public UObject
{
    GENERATED_BODY()

private:
    const FXmlNode* NodePtr = nullptr;

    UPROPERTY()
    TObjectPtr<UCloXmlFileWrapper> OwningFileWrapper = nullptr;

public:
    /** Initializes the wrapper with the actual FXmlNode. Internal use only. */
    void Initialize(const FXmlNode* InNode, UCloXmlFileWrapper* InOwner);

    /** Checks if the underlying node pointer is valid. */
    UFUNCTION(BlueprintPure, Category = "XML Node")
    bool IsValidNode() const;

    /** Gets the tag of the node (e.g., <tag>). */
    UFUNCTION(BlueprintPure, Category = "XML Node")
    FString GetTag() const;

    /** Gets the content of the node (text between tags). */
    UFUNCTION(BlueprintPure, Category = "XML Node")
    FString GetContent() const;

    /** Gets a specific attribute's value by its tag. Returns empty string if not found. */
    UFUNCTION(BlueprintPure, Category = "XML Node")
    FString GetAttribute(const FString& AttributeTag) const;

    /** Gets all attributes of this node. */
    UFUNCTION(BlueprintPure, Category = "XML Node")
    TArray<FCloBlueprintXmlAttribute> GetAttributes() const;

    /** Gets all direct child nodes of this node. */
    UFUNCTION(BlueprintPure, Category = "XML Node")
    TArray<UCloXmlNodeWrapper*> GetChildrenNodes() const;

    /** Finds the first direct child node with the specified tag. Returns null if not found. */
    UFUNCTION(BlueprintPure, Category = "XML Node")
    UCloXmlNodeWrapper* FindChildNode(const FString& ChildTag) const;

    /** Gets the owning UCloXmlFileWrapper. */
    UFUNCTION(BlueprintPure, Category = "XML Node")
    UCloXmlFileWrapper* GetOwningFile() const;
};