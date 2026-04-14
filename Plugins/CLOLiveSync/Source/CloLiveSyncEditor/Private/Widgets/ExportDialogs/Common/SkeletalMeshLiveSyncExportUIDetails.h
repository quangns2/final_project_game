// Copyright 2023-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/ExportDialogs/Common/CloLiveSyncExportUIDetails.h"
#include "Widgets/ExportDialogs/Common/CloLiveSyncExportOptions.h"


class SkeletalMeshLiveSyncExportUIDetails : public CloLiveSyncExportUIDetails
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance()
    {
        return MakeShareable(new SkeletalMeshLiveSyncExportUIDetails);
    }

protected:
    UCloSkeletalMeshLiveSyncExportOption* target;
};
