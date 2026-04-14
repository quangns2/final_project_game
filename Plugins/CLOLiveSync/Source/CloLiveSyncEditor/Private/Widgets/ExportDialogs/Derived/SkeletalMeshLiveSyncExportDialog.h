// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "Widgets/ExportDialogs/Base/CloLiveSyncExportDialog.h"

#include "Widgets/ExportDialogs/Common/CloLiveSyncExportOptions.h"

#define LOCTEXT_NAMESPACE "SkeletalMeshLiveSyncExportDialog"

class CLOLIVESYNCEDITOR_API SkeletalMeshLiveSyncExportDialog : public CloLiveSyncExportDialog
{
public:
    SLATE_BEGIN_ARGS(SkeletalMeshLiveSyncExportDialog)
        : _AllowReadOnlyFolders(true)
    {}
    
    SLATE_ARGUMENT(FText, Title)
    SLATE_ARGUMENT(FText, DefaultAssetPath)
    SLATE_ARGUMENT(bool, AllowReadOnlyFolders)
    SLATE_ARGUMENT(TWeakObjectPtr<USkeleton>, BaseSkeleton)
    SLATE_ARGUMENT(TWeakObjectPtr<UAnimationAsset>, Animation)
    SLATE_ARGUMENT(TArray<USkeletalMeshComponent*>, TargetComponents)
    SLATE_END_ARGS()

    SkeletalMeshLiveSyncExportDialog() {}

    void Construct(const FArguments& InArgs);
    EAppReturnType::Type ShowModal() override;

protected:
    bool ValidatePackage() override;
    FReply OnButtonClick(EAppReturnType::Type ButtonID) override;

    UCloSkeletalMeshLiveSyncExportOption* TargetObject;
};

#undef LOCTEXT_NAMESPACE
