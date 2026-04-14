// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "Style/CloLiveSyncEditorStyle.h"

#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateStyleMacros.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleColors.h"

TSharedPtr<FCloLiveSyncEditorStyle> FCloLiveSyncEditorStyle::LiveSyncEditorStyle = nullptr;

void FCloLiveSyncEditorStyle::Register()
{
    FSlateStyleRegistry::RegisterSlateStyle(Get());
}

void FCloLiveSyncEditorStyle::Unregister()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(Get());
}

void FCloLiveSyncEditorStyle::Shutdown()
{
    Unregister();
    LiveSyncEditorStyle.Reset();
}

void FCloLiveSyncEditorStyle::ReloadTextures()
{
    FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const FCloLiveSyncEditorStyle& FCloLiveSyncEditorStyle::Get()
{
    if (!LiveSyncEditorStyle.IsValid())
    {
        LiveSyncEditorStyle = MakeShareable(new FCloLiveSyncEditorStyle());
    }

    return *LiveSyncEditorStyle;
}

void FCloLiveSyncEditorStyle::ReinitializeStyle()
{
    Unregister();
    LiveSyncEditorStyle.Reset();
    Register();
}

FName FCloLiveSyncEditorStyle::GetStyleName()
{
    return Get().GetStyleSetName();
}

FCloLiveSyncEditorStyle::FCloLiveSyncEditorStyle() : FSlateStyleSet(TEXT("CloLiveSyncStyle"))
{
    const FVector2D Icon20x20(20.0f, 20.0f);
    const FVector2D Icon40x40(40.0f, 40.0f);
    const FVector2D Icon64x64(64.0f, 64.0f);
    const FVector2D Icon128x128(128.0f, 128.0f);


    static FString ResourcesDir = IPluginManager::Get().FindPlugin(TEXT("CLOLiveSync"))->GetBaseDir() / TEXT("Resources");
    SetContentRoot(ResourcesDir);

    Set("CloLiveSyncIcon", new IMAGE_BRUSH(TEXT("Icon128"), Icon40x40));
    Set("CloLiveSyncIcon.Small", new IMAGE_BRUSH(TEXT("Icon128"), Icon20x20));
    Set("CloLiveSyncIcon.Medium", new IMAGE_BRUSH(TEXT("Icon128"), Icon40x40));
    Set("CloLiveSyncIcon.Large", new IMAGE_BRUSH(TEXT("Icon128"), Icon64x64));
    Set("CloLiveSyncIcon.XLarge", new IMAGE_BRUSH(TEXT("Icon128"), Icon128x128));

    Set("CloLiveSync.LiveSync.Update", new IMAGE_BRUSH(TEXT("SyncNormal40"), Icon40x40));
    Set("CloLiveSync.LiveSync.Update.Small", new IMAGE_BRUSH(TEXT("SyncNormal40"), Icon20x20));
    Set("CloLiveSync.LiveSync.Update.Medium", new IMAGE_BRUSH(TEXT("SyncNormal40"), Icon40x40));

    Set("CloLiveSync.LiveSync.Save", new IMAGE_BRUSH(TEXT("SaveNormal40"), Icon40x40));
    Set("CloLiveSync.LiveSync.Save.Small", new IMAGE_BRUSH(TEXT("SaveNormal40"), Icon20x20));
    Set("CloLiveSync.LiveSync.Save.Medium", new IMAGE_BRUSH(TEXT("SaveNormal40"), Icon40x40));


    Set("CloLiveSync.LiveSync.AnimCombine", new IMAGE_BRUSH(TEXT("AnimCombine40"), Icon40x40));
    Set("CloLiveSync.LiveSync.AnimCombine.Small", new IMAGE_BRUSH(TEXT("AnimCombine20"), Icon20x20));
    Set("CloLiveSync.LiveSync.AnimCombine.Medium", new IMAGE_BRUSH(TEXT("AnimCombine40"), Icon40x40));


    SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
    SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

    Set("CloLiveSync.Extensions.Tool", new IMAGE_BRUSH("/Icons/GeneralTools/Fix_40x", Icon20x20));
}
