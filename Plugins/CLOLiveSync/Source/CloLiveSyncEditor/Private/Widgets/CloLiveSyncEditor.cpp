// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "CloLiveSyncEditor.h"

#include "EditorModeManager.h"
#include "Widgets/Layout/SBorder.h"

#include "Mode/CloLiveSyncMode.h"
#include "Mode/CloLiveSyncModeToolkit.h"
#include "Widgets/ChildWidgets/CloLiveSyncOptionWidget.h"
#include "Widgets/ChildWidgets/PoseToAnimationCombinerWidget.h"
#include "Widgets/ChildWidgets/MeshMaterialCombinerWidget.h"


void SCloLiveSyncEditor::Construct(const FArguments& InArgs, TSharedRef<FCloLiveSyncModeToolkit> InParentToolkit)
{

    BorderWidget = SNew(SBorder)
			        .HAlign(HAlign_Fill)
			        .VAlign(VAlign_Fill)
					.BorderBackgroundColor(FLinearColor::Transparent)
					.Padding(FMargin(0));
    ParentToolkit = InParentToolkit;

    MapEditorWidgets.Add(LiveSyncToolNames::LiveSync, SNew(SCloLiveSyncOptionWidget, InParentToolkit));
    MapEditorWidgets.Add(ExtensionToolNames::MeshMaterialCombiner, SNew(SMeshMaterialCombinerWidget));
    MapEditorWidgets.Add(ExtensionToolNames::PoseToAnimationCombiner, SNew(SPoseToAnimationCombinerWidget));

    ChildSlot
    [
        BorderWidget.ToSharedRef()
    ];

    RefreshEditorWidget();
}

void SCloLiveSyncEditor::NotifyToolChanged()
{
    RefreshEditorWidget();
}

TSharedPtr<SCompoundWidget> SCloLiveSyncEditor::GetEditorWidget(const FName& InName)
{
    if (MapEditorWidgets.Contains(InName))
    {
        return MapEditorWidgets[InName];
    }
    else
    {
        return nullptr;
    }
}

FCloLiveSyncMode* SCloLiveSyncEditor::GetEditorMode() const
{
    return (FCloLiveSyncMode*)GLevelEditorModeTools().GetActiveMode(FCloLiveSyncMode::EM_CloLiveSync);
}

void SCloLiveSyncEditor::RefreshEditorWidget()
{
    if (auto CloLiveSyncMode = GetEditorMode(); CloLiveSyncMode != nullptr)
    {
        if (CloLiveSyncMode->GetCurrentPaletteTool().IsValid())
        {
            auto CurrentPaletteTool = CloLiveSyncMode->GetCurrentPaletteTool().Pin();

            if (auto WidgetPtr = MapEditorWidgets.Find(CurrentPaletteTool->GetToolName()); WidgetPtr != nullptr)
            {
                if (WidgetPtr->IsValid())
                {
                    BorderWidget->SetContent(WidgetPtr->ToSharedRef());
                    BorderWidget->Invalidate(EInvalidateWidget::LayoutAndVolatility);
                }
            }
            else
            {
                BorderWidget->ClearContent();
                BorderWidget->Invalidate(EInvalidateWidget::LayoutAndVolatility);
            }
        }
    }
}
