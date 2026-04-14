// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CloLiveSyncDialog.h"

#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "CloLiveSyncExportDialog"


class CLOLIVESYNCEDITOR_API CloLiveSyncExportDialog : public CloLiveSyncDialog
{
public:
    SLATE_BEGIN_ARGS(CloLiveSyncExportDialog)
    {}
    
    SLATE_ARGUMENT(FText, Title)
    SLATE_END_ARGS()

    CloLiveSyncExportDialog() : DialogReturnType(EAppReturnType::Type::Cancel) {}

    void Construct(const FArguments& InArgs)
    {
        CloLiveSyncDialog::FArguments args;
        args._Title = InArgs._Title;
        CloLiveSyncDialog::Construct(args);

        MainWidget->AddSlot()
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                [
                    SAssignNew(ContentWidget, SVerticalBox)
                ]
            ];
        MainWidget->AddSlot()
            .AutoHeight()
            .HAlign(HAlign_Right)
            .VAlign(VAlign_Bottom)
            [
                SNew(SUniformGridPanel)
                .MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
                .MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"))
                .SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))

                + SUniformGridPanel::Slot(0, 0)
                [
                    SAssignNew(ExportButton, SButton)
                    .Text(LOCTEXT("Export", "Export"))
                    .ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
                    .OnClicked(this, &CloLiveSyncExportDialog::OnButtonClick, EAppReturnType::Ok)
                ]
                + SUniformGridPanel::Slot(1, 0)
                [
                    SAssignNew(CancelButton, SButton)
                    .Text(LOCTEXT("Cancel", "Cancel"))
                    .ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
                    .OnClicked(this, &CloLiveSyncExportDialog::OnButtonClick, EAppReturnType::Cancel)
                ]
            ];
    }

protected:
    virtual bool ValidatePackage() = 0;
    virtual FReply OnButtonClick(EAppReturnType::Type ButtonID) = 0;

    TSharedPtr<SVerticalBox> ContentWidget;

    TSharedPtr<SButton> ExportButton;
    TSharedPtr<SButton> CancelButton;

    EAppReturnType::Type DialogReturnType;
};

#undef LOCTEXT_NAMESPACE
