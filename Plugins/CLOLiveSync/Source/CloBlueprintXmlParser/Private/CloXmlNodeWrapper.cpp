// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#include "CloXmlNodeWrapper.h"
#include "XmlNode.h"
#include "CloXmlFileWrapper.h"

void UCloXmlNodeWrapper::Initialize(const FXmlNode* InNode, UCloXmlFileWrapper* InOwner)
{
    NodePtr = InNode;
    OwningFileWrapper = InOwner;
}

bool UCloXmlNodeWrapper::IsValidNode() const
{
    return OwningFileWrapper != nullptr && OwningFileWrapper->IsFileValid() && NodePtr != nullptr;
}

FString UCloXmlNodeWrapper::GetTag() const
{
    if (IsValidNode())
    {
        return NodePtr->GetTag();
    }
    return FString();
}

FString UCloXmlNodeWrapper::GetContent() const
{
    if (IsValidNode())
    {
        return NodePtr->GetContent();
    }
    return FString();
}

FString UCloXmlNodeWrapper::GetAttribute(const FString& AttributeTag) const
{
    if (IsValidNode())
    {
        return NodePtr->GetAttribute(AttributeTag);
    }
    return FString();
}

TArray<FCloBlueprintXmlAttribute> UCloXmlNodeWrapper::GetAttributes() const
{
    TArray<FCloBlueprintXmlAttribute> ResultAttributes;
    if (IsValidNode())
    {
        const TArray<FXmlAttribute>& OriginalAttributes = NodePtr->GetAttributes();
        ResultAttributes.Reserve(OriginalAttributes.Num());
        for (const FXmlAttribute& Attr : OriginalAttributes)
        {
            ResultAttributes.Emplace(Attr.GetTag(), Attr.GetValue());
        }
    }
    return ResultAttributes;
}

TArray<UCloXmlNodeWrapper*> UCloXmlNodeWrapper::GetChildrenNodes() const
{
    TArray<UCloXmlNodeWrapper*> ChildWrappers;
    if (IsValidNode() && OwningFileWrapper != nullptr)
    {
        const TArray<FXmlNode*>& Children = NodePtr->GetChildrenNodes();
        ChildWrappers.Reserve(Children.Num());
        for (const FXmlNode* ChildNode : Children)
        {
             if (ChildNode != nullptr)
             {
                UCloXmlNodeWrapper* NewWrapper = NewObject<UCloXmlNodeWrapper>(OwningFileWrapper);
                NewWrapper->Initialize(ChildNode, OwningFileWrapper);
                ChildWrappers.Add(NewWrapper);
             }
        }
    }
    return ChildWrappers;
}

UCloXmlNodeWrapper* UCloXmlNodeWrapper::FindChildNode(const FString& ChildTag) const
{
    if (IsValidNode() && OwningFileWrapper != nullptr)
    {
        const FXmlNode* FoundNode = NodePtr->FindChildNode(ChildTag);
        if (FoundNode)
        {
            UCloXmlNodeWrapper* NewWrapper = NewObject<UCloXmlNodeWrapper>(OwningFileWrapper);
            NewWrapper->Initialize(FoundNode, OwningFileWrapper);
            return NewWrapper;
        }
    }
    return nullptr;
}

UCloXmlFileWrapper* UCloXmlNodeWrapper::GetOwningFile() const
{
    return OwningFileWrapper;
}