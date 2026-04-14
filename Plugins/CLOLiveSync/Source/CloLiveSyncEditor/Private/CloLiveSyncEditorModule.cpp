// Copyright 2024 CLO Virtual Fashion. All rights reserved.

#include "CloLiveSyncEditorModule.h"

#include "EditorModeRegistry.h"

#include "Extensions/CloAnimationToolbarExtensionHelper.h"
#include "Mode/CloLiveSyncMode.h"
#include "Style/CloLiveSyncEditorStyle.h"

#define LOCTEXT_NAMESPACE "FCloLiveSyncEditorModule"

DEFINE_LOG_CATEGORY(LogCloLiveSyncEditor);

void FCloLiveSyncEditorModule::StartupModule()
{
    FCloLiveSyncEditorStyle::Register();

    FEditorModeRegistry::Get().RegisterMode<FCloLiveSyncMode>(
        FCloLiveSyncMode::EM_CloLiveSync,
        NSLOCTEXT("EditorModes", "CloLiveSyncMode", "CLO/MD LiveSync"),
        FSlateIcon(FCloLiveSyncEditorStyle::GetStyleName(), "CloLiveSyncIcon.Small"),
        true
        );

    AnimationExtenderHelper = MakeShareable(new FCloAnimationToolbarExtensionHelper);
    AnimationExtenderHelper->SubscribeExtender();
}

void FCloLiveSyncEditorModule::ShutdownModule()
{
    FCloLiveSyncEditorStyle::Shutdown();

    FEditorModeRegistry::Get().UnregisterMode(FCloLiveSyncMode::EM_CloLiveSync);

    if (AnimationExtenderHelper.IsValid())
    {
        AnimationExtenderHelper->UnsubscribeExtender();
    }
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCloLiveSyncEditorModule, CloLiveSyncEditor)