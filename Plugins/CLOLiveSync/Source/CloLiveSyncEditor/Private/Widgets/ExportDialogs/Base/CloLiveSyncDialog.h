// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#define LOCTEXT_NAMESPACE "CloLiveSyncDialog"


class CLOLIVESYNCEDITOR_API CloLiveSyncDialog : public SWindow
{
public:
    SLATE_BEGIN_ARGS(CloLiveSyncDialog)
    {}

    SLATE_ARGUMENT(FText, Title)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs)
    {
        SWindow::Construct(SWindow::FArguments()
            .Title(InArgs._Title)
            .SupportsMinimize(false)
            .SupportsMaximize(false)
            .ClientSize(FVector2D(450, 450))
            [
                SAssignNew(MainWidget, SVerticalBox)
            ]
        );
    }

    virtual EAppReturnType::Type ShowModal() = 0;

protected:
    TSharedPtr<SVerticalBox> MainWidget;
};

#undef LOCTEXT_NAMESPACE
