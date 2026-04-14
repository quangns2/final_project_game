// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "Commands/FCloLiveSyncModeCommands.h"

#define LOCTEXT_NAMESPACE "CloLiveSyncEditorCommands"

void FCloLiveSyncModeCommands::RegisterCommands()
{
    UI_COMMAND(Update, "Update", "Update", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(Save, "Save", "Save", EUserInterfaceActionType::Button, FInputChord());
    UI_COMMAND(SaveAsClothAsset, "SaveAsClothAsset", "SaveAsClothAsset", EUserInterfaceActionType::Button, FInputChord());

    // Extensions
    UI_COMMAND(GCMatCombiner, "GCMatCombiner", "GCMatCombiner", EUserInterfaceActionType::RadioButton, FInputChord());
    UI_COMMAND(PoseToAnimCombiner, "PoseToAnimCombiner", "PoseToAnimCombiner", EUserInterfaceActionType::RadioButton, FInputChord());
    UI_COMMAND(FurBinding, "FurBinding", "FurBinding", EUserInterfaceActionType::RadioButton, FInputChord());
}

#undef LOCTEXT_NAMESPACE