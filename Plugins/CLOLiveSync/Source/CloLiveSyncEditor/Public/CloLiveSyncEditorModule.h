// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"
#include "Objects/USDSchemaTranslator.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCloLiveSyncEditor, Log, All);

namespace CloLiveSyncCore
{
    class LiveSyncClient;
}

class FCloLiveSyncEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

    TSharedPtr<FUICommandList> EditorCommandList;
    TSharedPtr<class FCloAnimationToolbarExtensionHelper> AnimationExtenderHelper;
};
