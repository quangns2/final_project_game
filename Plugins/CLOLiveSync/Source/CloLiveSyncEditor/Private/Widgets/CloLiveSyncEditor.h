// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SBorder;
class FCloLiveSyncMode;
class FCloLiveSyncModeToolkit;

/**
 * Slate widgets for the CloLiveSync Editor Mode
 */
class SCloLiveSyncEditor : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCloLiveSyncEditor) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, TSharedRef<FCloLiveSyncModeToolkit> InParentToolkit);

    void NotifyToolChanged();

    TSharedPtr<SCompoundWidget> GetEditorWidget(const FName& InName);

protected:
    FCloLiveSyncMode* GetEditorMode() const;

    void RefreshEditorWidget();

private:
    TSharedPtr<SBorder> BorderWidget;
    TWeakPtr<FCloLiveSyncModeToolkit> ParentToolkit;

    TMap<FName, TSharedPtr<SCompoundWidget>> MapEditorWidgets;
};
