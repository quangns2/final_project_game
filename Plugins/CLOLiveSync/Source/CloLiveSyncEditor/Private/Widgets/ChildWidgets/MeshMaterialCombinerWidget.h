// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Input/Reply.h"
#include "MeshMaterialCombinerArguments.h"
#include "Misc/NotifyHook.h"
#include "Templates/SharedPointer.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FProperty;
class IDetailsView;
struct FPropertyChangedEvent;

class SMeshMaterialCombinerWidget : public SCompoundWidget, public FNotifyHook
{
    SLATE_BEGIN_ARGS(SMeshMaterialCombinerWidget)
    {
    }

    SLATE_END_ARGS()

    void Construct(const FArguments& Args);

private:
    FReply OnCombineClicked();

    bool IsMeshObject(const TObjectPtr<UObject>& InTargetMesh);

    bool GetMeshMaterials(const TObjectPtr<UObject>& InTargetMesh, TArray<TObjectPtr<UMaterialInterface>>& OutMaterials);
    bool SetMeshMaterials(const TObjectPtr<UObject>& InTargetMesh, const TArray<TObjectPtr<UMaterialInterface>>& InMaterials);

    void DeleteMaterialsAndTextures(TSet<TObjectPtr<UMaterialInterface>>& OtherMaterials);

    TSharedPtr<IDetailsView> MeshMaterialCombinerArgumentsDetailsView = nullptr;

    /** The current arguments. */
    TStrongObjectPtr<UMeshMaterialCombinerArguments> Arguments = nullptr;
};
