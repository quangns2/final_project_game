// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "SkeletalMeshLiveSyncExportDialog.h"

#include "EditorModeManager.h"
#include "Widgets/ExportDialogs/Common/SkeletalMeshLiveSyncExportUIDetails.h"

#include "Mode/CloLiveSyncMode.h"

#define LOCTEXT_NAMESPACE "SkeletalMeshLiveSyncExportDialog"


void SkeletalMeshLiveSyncExportDialog::Construct(const FArguments& InArgs)
{
    FDetailsViewArgs detailsViewArgs;
    detailsViewArgs.bHideSelectionTip = true;
    detailsViewArgs.bAllowSearch = false;

    TargetObject = NewObject<UCloSkeletalMeshLiveSyncExportOption>();
    TargetObject->AddToRoot();
    
    TargetObject->targetComponents = InArgs._TargetComponents;
    TargetObject->animationAsset = InArgs._Animation.Get();
    TargetObject->baseSkeleton = InArgs._BaseSkeleton.Get();

    FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyEditorModule.RegisterCustomClassLayout("CLOSkeletalMeshExportOption", FOnGetDetailCustomizationInstance::CreateStatic(&SkeletalMeshLiveSyncExportUIDetails::MakeInstance));
    TSharedPtr<IDetailsView> detailView = PropertyEditorModule.CreateDetailView(detailsViewArgs);
    detailView->SetObject(TargetObject);

    CloLiveSyncExportDialog::FArguments args;
    args._Title = InArgs._Title;
    CloLiveSyncExportDialog::Construct(args);

    ContentWidget->AddSlot()
        .AutoHeight()
        [
            detailView.ToSharedRef()
        ];

    this->SetSizingRule(ESizingRule::Autosized);
}

FReply SkeletalMeshLiveSyncExportDialog::OnButtonClick(EAppReturnType::Type ButtonID)
{
    DialogReturnType = ButtonID;

    FCloLiveSyncMode* Mode = static_cast<FCloLiveSyncMode*>(GLevelEditorModeTools().GetActiveMode(FCloLiveSyncMode::EM_CloLiveSync));
    FString ErrorMessage;
    if (Mode == nullptr || ValidatePackage() == false)
    {
        DialogReturnType = EAppReturnType::Type::Cancel;
        ErrorMessage = Mode == nullptr ? "Unable to export. Please open LiveSyncMode." : "Unable to export. Please set a valid export assets.";
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ErrorMessage));
    }

    RequestDestroyWindow();
    return FReply::Handled();
}

bool SkeletalMeshLiveSyncExportDialog::ValidatePackage()
{
    if (TargetObject->animationAsset == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Unable to export. Source Animation is Null."));
        return false;
    }

    if (TargetObject->baseSkeleton == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Unable to export. Source BaseSkeleton is Null."));
        return false;
    }

    return true;
}

EAppReturnType::Type SkeletalMeshLiveSyncExportDialog::ShowModal()
{
    GEditor->EditorAddModalWindow(SharedThis(this));
    return DialogReturnType;
}

#undef LOCTEXT_NAMESPACE
