// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "Mode/CloLiveSyncModeToolkit.h"

#include "Dialogs/DlgPickPath.h"
#include "Dialogs/Dialogs.h"
#include "EditorModeManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "Commands/FCloLiveSyncModeCommands.h"
#include "Communication/LiveSyncProtocol.h"
#include "Editor/CloEditorNotification.h"
#include "Mode/CloLiveSyncMode.h"
#include "Style/CloLiveSyncEditorStyle.h"
#include "Widgets/ChildWidgets/CloLiveSyncOptionWidget.h"
#include "Widgets/CloLiveSyncEditor.h"

#define LOCTEXT_NAMESPACE "CloLiveSyncModeToolkit"

void FCloLiveSyncModeToolkit::Init(const TSharedPtr< class IToolkitHost >& InitToolkitHost)
{
    LiveSyncEditor = SNew(SCloLiveSyncEditor, SharedThis(this));

    FCloLiveSyncModeCommands::Register();
    auto& Commands = FCloLiveSyncModeCommands::Get();

    ToolkitCommands->MapAction(Commands.Update
        , FExecuteAction::CreateSP(this, &FCloLiveSyncModeToolkit::OnUpdateButtonClick));
    ToolkitCommands->MapAction(Commands.Save
        , FExecuteAction::CreateSP(this, &FCloLiveSyncModeToolkit::OnSaveButtonClick));
    ToolkitCommands->MapAction(Commands.SaveAsClothAsset
        , FExecuteAction::CreateSP(this, &FCloLiveSyncModeToolkit::OnSaveAsClothAssetButtonClick));

    ToolkitCommands->MapAction(Commands.GCMatCombiner
		, FUIAction(FExecuteAction::CreateSP(this, &FCloLiveSyncModeToolkit::OnChangeTool, FName(ExtensionToolNames::MeshMaterialCombiner))
		, FCanExecuteAction::CreateSP(this, &FCloLiveSyncModeToolkit::IsToolEnabled, FName(ExtensionToolNames::MeshMaterialCombiner))
		, FIsActionChecked::CreateSP(this, &FCloLiveSyncModeToolkit::IsToolActive, FName(ExtensionToolNames::MeshMaterialCombiner))
		, FIsActionButtonVisible::CreateSP(this, &FCloLiveSyncModeToolkit::IsToolAvailable, FName(ExtensionToolNames::MeshMaterialCombiner))));

    ToolkitCommands->MapAction(Commands.PoseToAnimCombiner
        , FUIAction(FExecuteAction::CreateSP(this, &FCloLiveSyncModeToolkit::OnChangeTool, FName(ExtensionToolNames::PoseToAnimationCombiner))
            , FCanExecuteAction::CreateSP(this, &FCloLiveSyncModeToolkit::IsToolEnabled, FName(ExtensionToolNames::PoseToAnimationCombiner))
            , FIsActionChecked::CreateSP(this, &FCloLiveSyncModeToolkit::IsToolActive, FName(ExtensionToolNames::PoseToAnimationCombiner))
            , FIsActionButtonVisible::CreateSP(this, &FCloLiveSyncModeToolkit::IsToolAvailable, FName(ExtensionToolNames::PoseToAnimationCombiner))));


    if (FCloLiveSyncMode* mode = static_cast<FCloLiveSyncMode*>(GetEditorMode()); mode != nullptr)
    {
        mode->OnFilePathReceivedDelegate.AddSP(this, &FCloLiveSyncModeToolkit::OnFilePathReceived);
    }

    SetCurrentPalette(CloLiveSyncModePaletteNames::LiveSync);

    FModeToolkit::Init(InitToolkitHost);
}

FName FCloLiveSyncModeToolkit::GetToolkitFName() const
{
    return FName("CLOLiveSyncMode");
}

FText FCloLiveSyncModeToolkit::GetBaseToolkitName() const
{
    return LOCTEXT("ToolkitName", "CLO LiveSync Mode");
}

class FEdMode* FCloLiveSyncModeToolkit::GetEditorMode() const
{
    return GLevelEditorModeTools().GetActiveMode(FCloLiveSyncMode::EM_CloLiveSync);
}

TSharedPtr<SWidget> FCloLiveSyncModeToolkit::GetInlineContent() const
{
    return LiveSyncEditor;
}

const TArray<FName> FCloLiveSyncModeToolkit::PaletteNames = { CloLiveSyncModePaletteNames::LiveSync, CloLiveSyncModePaletteNames::Extensions};

void FCloLiveSyncModeToolkit::GetToolPaletteNames(TArray<FName>& InPaletteNames) const
{
    InPaletteNames = PaletteNames;
}

FText FCloLiveSyncModeToolkit::GetToolPaletteDisplayName(FName Palette) const
{
    if (Palette == CloLiveSyncModePaletteNames::LiveSync)
    {
        return LOCTEXT("Mode.LiveSync", "LiveSync");
    }
    else if (Palette == CloLiveSyncModePaletteNames::Extensions)
    {
        return LOCTEXT("Mode.Extensions", "Extensions");
    }

    return FText::GetEmpty();
}

void FCloLiveSyncModeToolkit::BuildToolPalette(FName Palette, FToolBarBuilder& ToolbarBuilder)
{
    auto& Commands = FCloLiveSyncModeCommands::Get();

    if (Palette == CloLiveSyncModePaletteNames::LiveSync)
    {
        ToolbarBuilder.AddToolBarButton(
            Commands.Update,
            NAME_None,
            FText::FromString("Update"),
            FText::FromString("Bring avatars and garments from CLO or Marvelous Designer"),
            FSlateIcon(FCloLiveSyncEditorStyle::GetStyleName(), "CloLiveSync.LiveSync.Update.Small"));

        ToolbarBuilder.AddToolBarButton(
            Commands.Save,
            NAME_None,
            FText::FromString("Save"),
            FText::FromString("Save Object"),
            FSlateIcon(FCloLiveSyncEditorStyle::GetStyleName(), "CloLiveSync.LiveSync.Save.Small"));

        ToolbarBuilder.AddToolBarButton(
            Commands.SaveAsClothAsset,
            NAME_None,
            FText::FromString("AsCloth"),
            FText::FromString("Save As ClothAsset"),
            FSlateIcon(FCloLiveSyncEditorStyle::GetStyleName(), "CloLiveSync.LiveSync.Save.Small"));
    }
    else if (Palette == CloLiveSyncModePaletteNames::Extensions)
    {
        ToolbarBuilder.AddToolBarButton(
            Commands.GCMatCombiner,
            NAME_None,
            FText::FromString("Material"),
            FText::FromString("Mesh Material Combiner"),
            FSlateIcon(FCloLiveSyncEditorStyle::GetStyleName(), "CloLiveSync.Extensions.Tool"));

        ToolbarBuilder.AddToolBarButton(
            Commands.PoseToAnimCombiner,
            NAME_None,
            FText::FromString("Pose"),
            FText::FromString("Pose to Animation Combiner"),
            FSlateIcon(FCloLiveSyncEditorStyle::GetStyleName(), "CloLiveSync.LiveSync.AnimCombine.Small"));

    }

}

void FCloLiveSyncModeToolkit::OnToolPaletteChanged(FName PaletteName)
{
    FCloLiveSyncMode* EdMode = static_cast<FCloLiveSyncMode*>(GetEditorMode());
    if (EdMode)
    {
        if (EdMode->GetCurrentPaletteTool().IsValid())
        {
            auto CurrentPaletteTool = EdMode->GetCurrentPaletteTool().Pin();
            CurrentPaletteTool->SetPaletteName(PaletteName);
        }
    }

    if (LiveSyncEditor.IsValid())
    {
	    LiveSyncEditor->NotifyToolChanged();
    }
}

TSharedPtr<SCompoundWidget> FCloLiveSyncModeToolkit::GetEditorWidget(const FName& InName)
{
    if (!LiveSyncEditor.IsValid())
    {
        return nullptr;
    }

    return LiveSyncEditor->GetEditorWidget(InName);
}

void FCloLiveSyncModeToolkit::OnChangeTool(FName InToolName)
{
    FCloLiveSyncMode* EdMode = static_cast<FCloLiveSyncMode*>(GetEditorMode());
    if (EdMode != nullptr)
    {
        if (EdMode->GetCurrentPaletteTool().IsValid())
        {
            auto CurrentPaletteTool = EdMode->GetCurrentPaletteTool().Pin();
            CurrentPaletteTool->SetToolName(InToolName);
        }
    }

    if (LiveSyncEditor.IsValid())
    {
        LiveSyncEditor->NotifyToolChanged();
    }
}

bool FCloLiveSyncModeToolkit::IsToolEnabled(FName ToolName) const
{
    FCloLiveSyncMode* EdMode = static_cast<FCloLiveSyncMode*>(GetEditorMode());
    if (EdMode != nullptr)
    {
        auto CurrentPaletteTool = EdMode->GetCurrentPaletteTool().Pin();
        return CurrentPaletteTool->IsVaildToolName(ToolName);
    }

    return false;
}

bool FCloLiveSyncModeToolkit::IsToolAvailable(FName ToolName) const
{
    FCloLiveSyncMode* EdMode = static_cast<FCloLiveSyncMode*>(GetEditorMode());
    if (EdMode != nullptr)
    {
        auto CurrentPaletteTool = EdMode->GetCurrentPaletteTool().Pin();
        return CurrentPaletteTool->IsVaildToolName(ToolName);
    }

    return false;
}

bool FCloLiveSyncModeToolkit::IsToolActive(FName ToolName) const
{
    FCloLiveSyncMode* EdMode = static_cast<FCloLiveSyncMode*>(GetEditorMode());
    if (EdMode != nullptr)
    {
        if (EdMode->GetCurrentPaletteTool().IsValid())
        {
            auto CurrentPaletteTool = EdMode->GetCurrentPaletteTool().Pin();
            if (CurrentPaletteTool->GetToolName() == ToolName)
            {
                return true;
            }
        }
    }

    return false;
}


void FCloLiveSyncModeToolkit::OnUpdateButtonClick()
{
	TSharedPtr<SCloLiveSyncOptionWidget> optionWidget = StaticCastSharedPtr<SCloLiveSyncOptionWidget>(LiveSyncEditor->GetEditorWidget(LiveSyncToolNames::LiveSync));

    if(optionWidget.IsValid())
    {
        Update(optionWidget->GetUSDLiveSyncOptions());
    }
}

void FCloLiveSyncModeToolkit::OnSaveButtonClick()
{
    SaveUsdStage();
}

void FCloLiveSyncModeToolkit::OnSaveAsClothAssetButtonClick()
{
    SaveAsClothAsset();
}

void FCloLiveSyncModeToolkit::OnFilePathReceived(const FString& FilePath)
{
    FCloEditorProgressNotification::UpdateProgressInfo(FText::FromString(""), FCloEditorProgressNotification::GetCurrentProgress() + 1);
}

bool FCloLiveSyncModeToolkit::IsConnected()
{
    if (FCloLiveSyncMode* mode = static_cast<FCloLiveSyncMode*>(GetEditorMode()); mode != nullptr)
    {
        return mode->IsConnected();
    }

    return false;
}

EAppReturnType::Type FCloLiveSyncModeToolkit::OpenPickPathDlg(FString InTitleKey, FString InTitleDefault, FString& OutPackagePath)
{
    TSharedPtr<SDlgPickPath> PickAssetPathWidget = SNew(SDlgPickPath)
        .Title(FText::Format(LOCTEXT("{0}", "{1}"), FText::FromString(InTitleKey), FText::FromString(InTitleDefault)));

    if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
    {
        OutPackagePath = PickAssetPathWidget->GetPath().ToString();

        return EAppReturnType::Ok;
    }

    return EAppReturnType::Cancel;
}

void FCloLiveSyncModeToolkit::Update(const CloLiveSync::USDOptions& InUpdateOption)
{
    if (IsConnected() == false)
    {
        const FText TitleText = FText::FromString("Update failed");
    	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Unable to send the update request because it is not connected to an instance of CLO/MD: please ensure that the CLO/MD is open, and try again once the circle indicator for the LiveSync Connection Status turns green."), TitleText);
        return;
    }

    const FSuppressableWarningDialog::EResult result = ShowLiveSyncUpdateWarningDialog();
    if (result == FSuppressableWarningDialog::Confirm || result == FSuppressableWarningDialog::Suppressed)
    {
        if (FCloLiveSyncMode* mode = static_cast<FCloLiveSyncMode*>(GetEditorMode()); mode != nullptr)
        {
            FCloEditorProgressNotification::CreateProgressNotification(2, FText::FromString("Update Requested"));

            mode->Update(InUpdateOption);

            FCloEditorProgressNotification::UpdateProgressInfo(FText::FromString("Updating"), 1);
        }
    }
}

void FCloLiveSyncModeToolkit::SaveUsdStage()
{
    if (FCloLiveSyncMode* mode = static_cast<FCloLiveSyncMode*>(GetEditorMode()); mode != nullptr)
    {
        FString Path;
        if(OpenPickPathDlg("SaveUsdStagePickPath", "Choose where to place the imported USD assets", Path) == EAppReturnType::Type::Ok)
        {
            mode->SaveUsdStage(Path);
        }
    }
}

void FCloLiveSyncModeToolkit::SaveAsClothAsset()
{
    if (FCloLiveSyncMode* mode = static_cast<FCloLiveSyncMode*>(GetEditorMode()); mode != nullptr)
    {
        FString Path;
        if (OpenPickPathDlg("SaveUsdStagePickPath", "Choose where to place the imported Cloth assets", Path) == EAppReturnType::Type::Ok)
        {
            mode->SaveAsClothAsset(Path);
        }
    }
}

FSuppressableWarningDialog::EResult FCloLiveSyncModeToolkit::ShowLiveSyncUpdateWarningDialog()
{
    FSuppressableWarningDialog::FSetupInfo setupInfo{
        LOCTEXT("CLOLiveSyncUpdateWarningMessage",
                    "Make sure to turn off simulation in CLO\n"
                    "Make sure to be in either Simulation Mode or UV Editor Mode in CLO\n"
                    "Make sure that no animation is being recorded in CLO"),
        LOCTEXT("CLOLiveSyncUpdateWarningTitle", "Warning"),
        "CLOLiveSync" };
    setupInfo.ConfirmText = LOCTEXT("CLOLiveSyncUpdateWarningConfirm", "Ok");
    setupInfo.CancelText = LOCTEXT("CLOLiveSyncUpdateWarningCancel", "Cancel");
    setupInfo.CheckBoxText = LOCTEXT("CLOLiveSyncUpdateWarningCheckBox", "Don't ask me again");

    const FSuppressableWarningDialog warningDialog(setupInfo);
    return warningDialog.ShowModal();
}

#undef LOCTEXT_NAMESPACE
