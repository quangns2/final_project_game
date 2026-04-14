// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "MeshMaterialCombinerWidget.h"

#include "CloLiveSyncEditorModule.h"
#include "CoreGlobals.h"
#include "Delegates/Delegate.h"
#include "DetailsViewArgs.h"
#include "EditorAssetLibrary.h"
#include "Framework/SlateDelegates.h"
#include "GeometryCache.h"
#include "HAL/Platform.h"
#include "IDetailsView.h"
#include "Internationalization/Internationalization.h"
#include "Layout/BasicLayoutWidgetSlot.h"
#include "Layout/Children.h"
#include "Layout/Margin.h"
#include "Math/Color.h"
#include "MeshMaterialCombinerArguments.h"
#include "Misc/Attribute.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "SlotBase.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateColor.h"
#include "Templates/Less.h"
#include "Types/SlateEnums.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "MeshMaterialCombiner"

void SMeshMaterialCombinerWidget::Construct(const FArguments& Args)
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsViewArgs.bAllowSearch = false;
    DetailsViewArgs.NotifyHook = this;
    DetailsViewArgs.bShowOptions = false;
    DetailsViewArgs.bShowModifiedPropertiesOption = true;

    Arguments = TStrongObjectPtr<UMeshMaterialCombinerArguments>(NewObject<UMeshMaterialCombinerArguments>());

    MeshMaterialCombinerArgumentsDetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    MeshMaterialCombinerArgumentsDetailsView->SetObject(Arguments.Get());

    const FMargin PaddingAmount(5.0f);

    this->ChildSlot
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .Padding(PaddingAmount)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                .Padding(PaddingAmount)
                [
                    MeshMaterialCombinerArgumentsDetailsView->AsShared()
                ]
                
            ]
            + SVerticalBox::Slot()
            .Padding(PaddingAmount)
            .AutoHeight()
            .HAlign(HAlign_Right)
            [
                SNew(SButton)
                .ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
                .ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
                .ForegroundColor(FLinearColor::White)
                .OnClicked(FOnClicked::CreateSP(this, &SMeshMaterialCombinerWidget::OnCombineClicked))
                .ToolTipText(LOCTEXT("CombineMeshMaterialTooltipLoc", "Combine material."))
                .Text(LOCTEXT("CombineMeshMaterialLoc", "Combine"))
            ]
        ];
}

FReply SMeshMaterialCombinerWidget::OnCombineClicked()
{
    if ((Arguments->BaseMesh == nullptr)
        || (Arguments->TargetMeshes.IsEmpty()))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Please ensure that both BaseMesh and TargetMeshes are set before proceeding."));
        return FReply::Unhandled();
    }

    for (int i = 0; i < Arguments->TargetMeshes.Num(); ++i)
    {
        if (Arguments->TargetMeshes[i] == nullptr)
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("TargetMeshs at index %d is not valid. Please ensure all meshes are loaded and valid before proceeding."), i)));
            return FReply::Unhandled();
        }
    }

    if (!IsMeshObject(Arguments->BaseMesh))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("BaseMesh must be a Static Mesh, Skeletal Mesh, or Geometry Cache."));
        return FReply::Unhandled();
    }


	{
		TArray<int> invaildMeshIndex;
    	for (int i = 0; i < Arguments->TargetMeshes.Num(); ++i)
    	{
    		if (!IsMeshObject(Arguments->TargetMeshes[i]))
    		{
    			invaildMeshIndex.Add(i);
    		}
    	}

    	if (!invaildMeshIndex.IsEmpty())
    	{
    		FString ProblematicIndices = "TargetMeshes must be a Static Mesh, Skeletal Mesh, or Geometry Cache. Please check TargetMeshes : ";
    		for (int Index : invaildMeshIndex)
    		{
    			ProblematicIndices += FString::Printf(TEXT("[%d], "), Index);
    		}
    		ProblematicIndices = ProblematicIndices.LeftChop(2);

    		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ProblematicIndices));
    		return FReply::Unhandled();
    	}
	}

    TArray<TObjectPtr<UMaterialInterface>> BaseMaterials;
    GetMeshMaterials(Arguments->BaseMesh, BaseMaterials);

	{
		TArray<int> invaildMeshIndex;
    	for (int i = 0; i < Arguments->TargetMeshes.Num(); ++i)
    	{
            TArray<TObjectPtr<UMaterialInterface>> Materials;
            GetMeshMaterials(Arguments->TargetMeshes[i], Materials);

    		if (Materials.Num() != BaseMaterials.Num())
    		{
    			invaildMeshIndex.Add(i);
    		}
    	}

    	if (!invaildMeshIndex.IsEmpty())
    	{
    		FString ProblematicIndices = "Please check TargetMeshes. The following indices have mismatched material counts: ";
    		for (int Index : invaildMeshIndex)
    		{
    			ProblematicIndices += FString::Printf(TEXT("[%d], "), Index);
    		}
    		ProblematicIndices = ProblematicIndices.LeftChop(2);

    		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ProblematicIndices));
    		return FReply::Unhandled();
    	}
	}

    if (Arguments->bDeleteAllOtherMaterials)
    {
        EAppReturnType::Type DialogReturn = FMessageDialog::Open(EAppMsgCategory::Warning
            , EAppMsgType::OkCancel
            , FText::FromString(TEXT("Material Instances for TargetMeshes will be deleted, and Material Instances for BaseMesh will be applied to TargetMeshes. This process is irreversible, do you wish to continue?"))
            , FText::FromString(TEXT("Warning: reducing the number of Material Instances")));
        if (DialogReturn != EAppReturnType::Ok)
        {
            return FReply::Unhandled();
        }
    }

    TSet<TObjectPtr<UMaterialInterface>> OtherMaterials;
    for (int i = 0; i < Arguments->TargetMeshes.Num(); ++i)
    {
        TArray<TObjectPtr<UMaterialInterface>> MeshMaterials;
        GetMeshMaterials(Arguments->TargetMeshes[i], MeshMaterials);
        for (int j = 0; j < MeshMaterials.Num(); ++j)
        {
            if (MeshMaterials[j] != BaseMaterials[j])
            {
                OtherMaterials.Add(MeshMaterials[j]);
            }
        }

        SetMeshMaterials(Arguments->TargetMeshes[i], BaseMaterials);
        Arguments->TargetMeshes[i]->MarkPackageDirty();
    }

    if (Arguments->bDeleteAllOtherMaterials)
    {
        DeleteMaterialsAndTextures(OtherMaterials);
    }

    FString message = "Combination successful";
    if (Arguments->bDeleteAllOtherMaterials)
    {
        message += ": all unnecessary materials have been deleted.";
    }
    else
    {
        message += ".";
    }

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(message));

    return FReply::Handled();
}

bool SMeshMaterialCombinerWidget::IsMeshObject(const TObjectPtr<UObject>& InTargetMesh)
{
    if (!(InTargetMesh->IsA(UStaticMesh::StaticClass())
		|| InTargetMesh->IsA(USkeletalMesh::StaticClass())
        || InTargetMesh->IsA(UGeometryCache::StaticClass())))
    {
        return false;
    }

    return true;
}

bool SMeshMaterialCombinerWidget::GetMeshMaterials(const TObjectPtr<UObject>& InTargetMesh, TArray<TObjectPtr<UMaterialInterface>>& OutMaterials)
{
    if (!IsMeshObject(InTargetMesh))
    {
        return false;
    }

    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(InTargetMesh); StaticMesh != nullptr)
    {
    	for (FStaticMaterial& staticMaterial : StaticMesh->GetStaticMaterials())
    	{
            OutMaterials.Add(staticMaterial.MaterialInterface);
    	}
    }
    else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(InTargetMesh); SkeletalMesh != nullptr)
    {
        for (FSkeletalMaterial& skeletalMaterial : SkeletalMesh->GetMaterials())
        {
            OutMaterials.Add(skeletalMaterial.MaterialInterface);
        }
    }
    else if (UGeometryCache* GeometryCache = Cast<UGeometryCache>(InTargetMesh); GeometryCache != nullptr)
    {
        for (auto& material : GeometryCache->Materials)
        {
            OutMaterials.Add(material);
        }
    }

    return true;
}

bool SMeshMaterialCombinerWidget::SetMeshMaterials(const TObjectPtr<UObject>& InTargetMesh, const TArray<TObjectPtr<UMaterialInterface>>& InMaterials)
{
    if (!IsMeshObject(InTargetMesh))
    {
        return false;
    }

    if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(InTargetMesh); StaticMesh != nullptr)
    {
        if (StaticMesh->GetStaticMaterials().Num() != InMaterials.Num())
        {
            return false;
        }

        for (int i = 0; i < InMaterials.Num(); ++i)
        {
            StaticMesh->GetStaticMaterials()[i].MaterialInterface = InMaterials[i];
        }
        
    }
    else if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(InTargetMesh); SkeletalMesh != nullptr)
    {
        if (SkeletalMesh->GetMaterials().Num() != InMaterials.Num())
        {
            return false;
        }
        
        for (int i = 0; i < InMaterials.Num(); ++i)
        {
            SkeletalMesh->GetMaterials()[i].MaterialInterface = InMaterials[i];
        }
    }
    else if (UGeometryCache* GeometryCache = Cast<UGeometryCache>(InTargetMesh); GeometryCache != nullptr)
    {
        if (GeometryCache->Materials.Num() != InMaterials.Num())
        {
	        return false;
        }

        for (int i = 0; i < InMaterials.Num(); ++i)
        {
            GeometryCache->Materials[i] = InMaterials[i];
        }
    }

    return true;
}

void SMeshMaterialCombinerWidget::DeleteMaterialsAndTextures(TSet<TObjectPtr<UMaterialInterface>>& OtherMaterials)
{
    for (TObjectPtr<UMaterialInterface> Material : OtherMaterials)
    {
        UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
        if (MaterialInstance)
        {
            for (FTextureParameterValue& TextureParam : MaterialInstance->TextureParameterValues)
            {
                if (TextureParam.IsOverride() && (TextureParam.ParameterValue != nullptr))
                {
                    FString TexturePath = TextureParam.ParameterValue->GetPathName();
                    if (UEditorAssetLibrary::DoesAssetExist(TexturePath))
                    {
                        if (UEditorAssetLibrary::DeleteAsset(TexturePath))
                        {
                            UE_LOG(LogCloLiveSyncEditor, Log, TEXT("Overridden texture deleted successfully: %s"), *TexturePath);
                        }
                        else
                        {
                            UE_LOG(LogCloLiveSyncEditor, Warning, TEXT("Failed to delete overridden texture: %s"), *TexturePath);
                        }
                    }
                    else
                    {
                        UE_LOG(LogCloLiveSyncEditor, Warning, TEXT("Overridden texture not found: %s"), *TexturePath);
                    }
                }
            }
        }

        FString MaterialPath = Material->GetPathName();
        if (UEditorAssetLibrary::DoesAssetExist(MaterialPath))
        {
            if (UEditorAssetLibrary::DeleteAsset(MaterialPath))
            {
                UE_LOG(LogCloLiveSyncEditor, Log, TEXT("Material deleted successfully: %s"), *MaterialPath);
            }
            else
            {
                UE_LOG(LogCloLiveSyncEditor, Warning, TEXT("Failed to delete material: %s"), *MaterialPath);
            }
        }
        else
        {
            UE_LOG(LogCloLiveSyncEditor, Warning, TEXT("Material not found: %s"), *MaterialPath);
        }
    }

    OtherMaterials.Empty();
}

#undef LOCTEXT_NAMESPACE
