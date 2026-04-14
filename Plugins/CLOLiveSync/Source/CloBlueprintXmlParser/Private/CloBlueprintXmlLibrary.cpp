// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#include "CloBlueprintXmlLibrary.h"
#include "CloXmlFileWrapper.h"
#include "XmlFile.h"

UCloXmlFileWrapper* UCloBlueprintXmlLibrary::LoadXml(const FString& FilePathOrBuffer, ECloBlueprintXmlConstructMethod ConstructMethod, bool& bSuccess, FString& ErrorMessage)
{
    TSharedPtr<FXmlFile> XmlFile = MakeShared<FXmlFile>();

    EConstructMethod::Type NativeConstructMethod = (ConstructMethod == ECloBlueprintXmlConstructMethod::ConstructFromFile)
                                                                ? EConstructMethod::Type::ConstructFromFile
                                                                : EConstructMethod::Type::ConstructFromBuffer;

    if (XmlFile->LoadFile(FilePathOrBuffer, NativeConstructMethod))
    {
        bSuccess = true;
        ErrorMessage = XmlFile->GetLastError();
        UCloXmlFileWrapper* Wrapper = NewObject<UCloXmlFileWrapper>();
        Wrapper->Initialize(XmlFile);
        return Wrapper;
    }
    else
    {
        bSuccess = false;
        ErrorMessage = XmlFile->GetLastError();
        return nullptr;
    }
}