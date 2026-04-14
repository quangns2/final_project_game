// Copyright 2023-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IAnimationEditorModule.h"

#include "CloLiveSyncToolbarExtensionHelper.h"


class CLOLIVESYNCEDITOR_API FCloAnimationToolbarExtensionHelper : ICloLiveSyncToolbarExtensionHelper
{
public:
    virtual ~FCloAnimationToolbarExtensionHelper() {}

    void SubscribeExtender() override;
    void UnsubscribeExtender() override;

    TSharedRef<FExtender> GetToolbarExtender(const TSharedRef<FUICommandList> CommandList, TSharedRef<IAnimationEditor> InAnimationEditor) override;

private:
    void HandleAddLiveSyncExtenderToToolbar(FToolBarBuilder& ParentToolbarBuilder);
    TSharedRef<SWidget> GenerateAnimationMenu();

    TArray<USkeletalMeshComponent*> GetPreviewMeshComponents();

    void ExportSkeletalMeshAsLiveSync(bool AttachToScene);
    void ExportGeometryCacheAsLiveSync(bool AttachToScene);

    void ProcessExportInternal(UObject* TargetObject);

    void DisplayMeshInScene(USceneComponent* SceneComponent, FTransform Transform);

private:
    TWeakPtr<IAnimationEditor> AnimationEditor;
    TArray<USceneComponent*> DisplayedComponents;

    bool Debug;
};
