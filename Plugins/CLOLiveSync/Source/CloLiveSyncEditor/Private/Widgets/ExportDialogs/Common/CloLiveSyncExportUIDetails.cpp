// Copyright 2023-2025 CLO Virtual Fashion. All rights reserved.

#include "CloLiveSyncExportUIDetails.h"
#include "DetailLayoutBuilder.h"
#include "Editor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SCheckBox.h"

#define LOCTEXT_NAMESPACE "CloLiveSyncExportUIDetails"


void CloLiveSyncExportUIDetails::CustomizeDetails(IDetailLayoutBuilder& detailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> customizedObjects;
    detailBuilder.GetObjectsBeingCustomized(customizedObjects);

    for (TWeakObjectPtr<UObject> Object : customizedObjects)
    {
        if (Object.IsValid())
        {
            target = Cast<UCloLiveSyncExportOptionBase>(Object);
            if (target)
                break;
        }
    }
}

#undef LOCTEXT_NAMESPACE
