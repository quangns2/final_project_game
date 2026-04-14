// Copyright 2024 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "Toolkits/BaseToolkit.h"

#include "Dialogs/Dialogs.h"

#include "Communication/LiveSyncProtocol.h"

class IDetailsView;
class SButton;
class SCheckBox;
class SUniformGridPanel;
class SCloLiveSyncOptionWidget;
class SCloLiveSyncEditor;

namespace CloLiveSyncModePaletteNames
{
    static const FName LiveSync(TEXT("Palette_LiveSync"));
    static const FName Extensions(TEXT("Palette_Extensions"));
}

namespace LiveSyncToolNames
{
    static const FName LiveSync(TEXT("Tool_LiveSync"));
}

namespace ExtensionToolNames
{
    static const FName MeshMaterialCombiner(TEXT("Tool_MeshMatCombiner"));
    static const FName PoseToAnimationCombiner(TEXT("Tool_PoseToAnimCombiner")); 
}

class CLOLIVESYNCEDITOR_API FCloLiveSyncModeToolkit : public FModeToolkit
{
    friend SCloLiveSyncOptionWidget;

public:
    /** Initializes the geometry mode toolkit */
    virtual void Init(const TSharedPtr< class IToolkitHost >& InitToolkitHost) override;

    /** IToolkit interface */
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual class FEdMode* GetEditorMode() const override;
    virtual TSharedPtr<class SWidget> GetInlineContent() const override;

    /** FModeToolkit interface */
    virtual void GetToolPaletteNames(TArray<FName>& InPaletteName) const override;
    virtual FText GetToolPaletteDisplayName(FName PaletteName) const override;
    virtual void BuildToolPalette(FName PaletteName, class FToolBarBuilder& ToolbarBuilder) override;
    virtual void OnToolPaletteChanged(FName PaletteName) override;

    TSharedPtr<SCompoundWidget> GetEditorWidget(const FName& InName);

private:
    // Tool
    void OnChangeTool(FName InToolName);
    bool IsToolEnabled(FName InToolName) const;
    bool IsToolActive(FName InToolName) const;
    bool IsToolAvailable(FName InToolName) const;

    // Toolbar Button click
    void OnUpdateButtonClick();
    void OnSaveButtonClick();
    void OnSaveAsClothAssetButtonClick();

    void OnFilePathReceived(const FString& FilePath);

    bool IsConnected();

    EAppReturnType::Type OpenPickPathDlg(FString InTitleKey, FString InTitleDefault, FString& OutPackagePath);

    void Update(const CloLiveSync::USDOptions& InUpdateOption);
    void SaveUsdStage();
    void SaveAsClothAsset();

    FSuppressableWarningDialog::EResult ShowLiveSyncUpdateWarningDialog();

private:
    TSharedPtr<SCloLiveSyncEditor> LiveSyncEditor;

    const static TArray<FName> PaletteNames;
};

