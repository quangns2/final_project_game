// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#include "CloXmlFileWrapper.h"
#include "CloXmlNodeWrapper.h"
#include "XmlNode.h"
#include "XmlFile.h"

void UCloXmlFileWrapper::Initialize(TSharedPtr<FXmlFile> InXmlFile)
{
    XmlFilePtr = InXmlFile;
    if (XmlFilePtr.IsValid())
    {
        LastErrorMessage = XmlFilePtr->GetLastError();
    }
    else
    {
        LastErrorMessage = TEXT("Invalid FXmlFile pointer provided.");
    }
}

bool UCloXmlFileWrapper::IsFileValid() const
{
    return XmlFilePtr.IsValid() && XmlFilePtr->IsValid();
}

UCloXmlNodeWrapper* UCloXmlFileWrapper::GetRootNode()
{
    if (IsFileValid())
    {
        const FXmlNode* RootNode = XmlFilePtr->GetRootNode();
        if (RootNode)
        {
            UCloXmlNodeWrapper* RootWrapper = NewObject<UCloXmlNodeWrapper>(this);
            RootWrapper->Initialize(RootNode, this);
            return RootWrapper;
        }
    }
    return nullptr;
}

FString UCloXmlFileWrapper::GetLastError() const
{
    if (XmlFilePtr.IsValid())
    {
        return XmlFilePtr->GetLastError();
    }
    return LastErrorMessage;
}

void UCloXmlFileWrapper::Clear()
{
    if (XmlFilePtr.IsValid())
    {
        XmlFilePtr->Clear();
    }
}