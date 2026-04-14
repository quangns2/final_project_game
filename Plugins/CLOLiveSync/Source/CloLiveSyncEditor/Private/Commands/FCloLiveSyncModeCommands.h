// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FCloLiveSyncModeCommands : public TCommands<FCloLiveSyncModeCommands>
{
public:
    /** Constructor */
    FCloLiveSyncModeCommands()
        : TCommands<FCloLiveSyncModeCommands>(
            "CloLiveSyncEditorModuleCommands",
            NSLOCTEXT("Contexts", "CloLiveSyncEditor", "CloLiveSyncEditor"),
            NAME_None,
            FAppStyle::GetAppStyleSetName()) // Icon Style Set
    {
    }

    // LiveSync Button
    TSharedPtr<FUICommandInfo> Update;
    TSharedPtr<FUICommandInfo> Save;
    TSharedPtr<FUICommandInfo> SaveAsClothAsset;

    // Extensions
    TSharedPtr<FUICommandInfo> GCMatCombiner;
    TSharedPtr<FUICommandInfo> PoseToAnimCombiner;
    TSharedPtr<FUICommandInfo> FurBinding;

    /** Initialize commands */
    virtual void RegisterCommands() override;
};