// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "Extensions/CloAnimationToolbarExtensionHelper.h"

#include "AnimPreviewAttacheInstance.h"
#include "Animation/PoseAsset.h"
#include "IPersonaPreviewScene.h"
#include "IPersonaToolkit.h"
#include "ISkeletonEditorModule.h"

#include "AssetExportTask.h"
#include "Exporters/Exporter.h"
#include "Misc/ScopedSlowTask.h"
#include "UObject/GCObjectScopeGuard.h"
#include "AnimSequenceExporterUSDOptions.h"
#include "GeometryCacheExporterUSDOptions.h"
#include "USDStageOptions.h"
#include "GeometryCache.h"

#include "Style/CloLiveSyncEditorStyle.h"
#include "Utils/CloLiveSyncCoreUtils.h"
#include "Utils/CloLiveSyncEditorUtils.h"
#include "Widgets/ExportDialogs/Derived/GeometryCacheLiveSyncExportDialog.h"
#include "Widgets/ExportDialogs/Derived/SkeletalMeshLiveSyncExportDialog.h"

#include "Animation/DebugSkelMeshComponent.h"

#include "Mode/CloLiveSyncMode.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "FCloAnimationToolbarExtensionHelper"

void FCloAnimationToolbarExtensionHelper::SubscribeExtender()
{
    IAnimationEditorModule& animationEditorModule = FModuleManager::Get().LoadModuleChecked<IAnimationEditorModule>("AnimationEditor");
    auto& toolbarExtenders = animationEditorModule.GetAllAnimationEditorToolbarExtenders();
    toolbarExtenders.Add(IAnimationEditorModule::FAnimationEditorToolbarExtender::CreateRaw(this, &FCloAnimationToolbarExtensionHelper::GetToolbarExtender));
}

void FCloAnimationToolbarExtensionHelper::UnsubscribeExtender()
{
    IAnimationEditorModule& animationEditorModule = FModuleManager::Get().LoadModuleChecked<IAnimationEditorModule>("AnimationEditor");
    animationEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(Extender);
}

TSharedRef<FExtender> FCloAnimationToolbarExtensionHelper::GetToolbarExtender(const TSharedRef<FUICommandList> CommandList, TSharedRef<IAnimationEditor> InAnimationEditor)
{
    Extender = MakeShareable(new FExtender);
    AnimationEditor = InAnimationEditor;
    
    Extender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        CommandList,
        FToolBarExtensionDelegate::CreateRaw(this, &FCloAnimationToolbarExtensionHelper::HandleAddLiveSyncExtenderToToolbar)
    );

    return Extender.ToSharedRef();
}

void FCloAnimationToolbarExtensionHelper::HandleAddLiveSyncExtenderToToolbar(FToolBarBuilder& ParentToolbarBuilder)
{
    ParentToolbarBuilder.AddComboButton(
        FUIAction(),
        FOnGetContent::CreateRaw(this, &FCloAnimationToolbarExtensionHelper::GenerateAnimationMenu),
        LOCTEXT("CLOLiveSyncExportPose", "LiveSync"),
        LOCTEXT("CLOLiveSyncExportPoseTooltip", "..."),
        FSlateIcon(FCloLiveSyncEditorStyle::GetStyleName(), "CloLiveSyncIcon.Medium")
    );
}

TSharedRef<SWidget> FCloAnimationToolbarExtensionHelper::GenerateAnimationMenu()
{
    FMenuBuilder MenuBuilder(true, nullptr);

    if (AnimationEditor.IsValid())
    {
        auto IsExportable = [this]() {
            TSharedRef<IAnimationEditor> AnimationEditorRef = AnimationEditor.Pin().ToSharedRef();
            TSharedRef<IPersonaToolkit> PersonaToolkit = AnimationEditorRef->GetPersonaToolkit();
            USkeleton* Skeleton = PersonaToolkit->GetSkeleton();
            UAnimationAsset* AnimationAsset = PersonaToolkit->GetAnimationAsset();
            UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent();
            
			return Skeleton && 
                PreviewMeshComponent && PreviewMeshComponent->GetSkeletalMeshAsset() &&
                AnimationAsset && AnimationAsset->IsA<UAnimSequence>();
            };

		bool bExportable = IsExportable();

        FUIAction SaveSkeletalMeshAnimationTo(
            FExecuteAction::CreateLambda([this](){
                this->ExportSkeletalMeshAsLiveSync(Debug);
                }
            ),
            FCanExecuteAction::CreateLambda([bExportable](){
                return bExportable;
                }
            )
        );

        FUIAction SaveGeometryCacheAnimationTo(
            FExecuteAction::CreateLambda([this](){
                this->ExportGeometryCacheAsLiveSync(Debug);
                }
            ),
            FCanExecuteAction::CreateLambda([bExportable](){
                return bExportable;
                }
            )
        );

#if 0
        // for visual debug.
        FUIAction ToggleDebugMode(
            FExecuteAction::CreateLambda([this](){
                Debug = !Debug;
            }),
            FCanExecuteAction(),
            FGetActionCheckState::CreateLambda([this](){
                return Debug ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
            }),
            EUIActionRepeatMode::RepeatEnabled
        );
#endif

        auto NotExportableTooltip = LOCTEXT("CLOLiveSyncUnableExport_NeedPreviewMesh", "Unable to Export. Please set PreviewMesh on the right.");

        MenuBuilder.BeginSection("Export", LOCTEXT("CLOLiveSyncSavePoseSection", "Export Pose"));
        {
            MenuBuilder.AddSubMenu(
                LOCTEXT("CLOLiveSyncExportTypeSelection", "As LiveSync"),
                LOCTEXT("CLOLiveSyncExportTypeSelectionTooltip", "Export Additional SkeletalMeshes As LiveSync"),
                FNewMenuDelegate::CreateLambda([this, SaveSkeletalMeshAnimationTo, SaveGeometryCacheAnimationTo, bExportable, NotExportableTooltip](FMenuBuilder& InSubMenuBuilder)
                {
                    InSubMenuBuilder.AddMenuEntry(
                        LOCTEXT("CLOLiveSyncExportPoseAsSkeletalMesh", "Skeletal Mesh"),
                        bExportable ? LOCTEXT("CLOLiveSyncExportPoseAsSkeletalMeshTooltip", "Export Additional SkeletalMeshes to Single SkeletalMesh.")
                         : NotExportableTooltip,
                        FSlateIcon(),
                        SaveSkeletalMeshAnimationTo);

                    InSubMenuBuilder.AddMenuEntry(
                        LOCTEXT("CLOLiveSyncExportPoseAsGeometryCache", "Geometry Cache"),
                        bExportable ? LOCTEXT("CLOLiveSyncExportPoseAsGeometryCacheTooltip", "Export Joint Animation to GeometryCache Animation.")
                         : NotExportableTooltip,
                        FSlateIcon(),
                        SaveGeometryCacheAnimationTo);
                })
            );
        }
        MenuBuilder.EndSection();
    }
    return MenuBuilder.MakeWidget();
}

TArray<USkeletalMeshComponent*> FCloAnimationToolbarExtensionHelper::GetPreviewMeshComponents()
{
    TSharedRef<IAnimationEditor> AnimationEditorRef = AnimationEditor.Pin().ToSharedRef();
    TSharedRef<IPersonaToolkit> PersonaToolkit = AnimationEditorRef->GetPersonaToolkit();
    TSharedRef<IPersonaPreviewScene> PreviewScene = PersonaToolkit->GetPreviewScene();

    AActor* PersonaActor = PreviewScene->GetActor();
    if (PersonaActor == nullptr)
    {
        return {};
    }

    TArray<USkeletalMeshComponent*> OutSkelComps;
    for (auto Component : PersonaActor->GetComponents())
    {
        if (Component->IsA(USkeletalMeshComponent::StaticClass()))
        {
            OutSkelComps.Add(Cast<USkeletalMeshComponent>(Component));
        }
    }
    
    return OutSkelComps;
}

void FCloAnimationToolbarExtensionHelper::ExportSkeletalMeshAsLiveSync(bool AttachToScene)
{
    TSharedRef<IAnimationEditor> AnimationEditorRef = AnimationEditor.Pin().ToSharedRef();
    auto PersonaToolkit = AnimationEditorRef->GetPersonaToolkit();
    
    auto BaseSkeleton = PersonaToolkit->GetSkeleton();
    auto SkeletalMeshComponents = GetPreviewMeshComponents();
	auto AnimationAsset = PersonaToolkit->GetAnimationAsset();

    // For UI, It need to be shown in the same way as the original one.
    FString NewNameSuggestion = FString(TEXT("export params"));
    TSharedPtr<SkeletalMeshLiveSyncExportDialog> exportDialog =
        SNew(SkeletalMeshLiveSyncExportDialog)
        .Title(LOCTEXT("CLOLiveSyncExportTitle", "CLOLiveSync Exporter"))
        .DefaultAssetPath(FText::FromString(NewNameSuggestion))
        .BaseSkeleton(TWeakObjectPtr<USkeleton>(BaseSkeleton))
        .TargetComponents(SkeletalMeshComponents)
        .Animation(TWeakObjectPtr<UAnimationAsset>(AnimationAsset));

    if (exportDialog->ShowModal() == EAppReturnType::Ok)
    {
        UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationAsset);

        USkeletalMesh* ConvertedSkelMesh = CloEditorUtils::ConvertSkeletalMeshesToSingleSkeletalMesh(SkeletalMeshComponents);
        if (ConvertedSkelMesh != nullptr)
        {
            UAnimSequence* BakedAnimSequence = CloEditorUtils::CreateBakedAnimSequenceWithRootMotion(AnimSequence, ConvertedSkelMesh);
            if (BakedAnimSequence)
            {
                ProcessExportInternal(BakedAnimSequence);
            }
        }

        // recache
        if (AnimSequence)
        {
            AnimSequence->UpdateDependentStreamingAnimations();
            AnimSequence->ClearAllCachedCookedPlatformData();
            AnimSequence->BeginCacheDerivedDataForCurrentPlatform();
        }
    }
}

void FCloAnimationToolbarExtensionHelper::ExportGeometryCacheAsLiveSync(bool AttachToScene)
{
    TSharedRef<IAnimationEditor> AnimationEditorRef = AnimationEditor.Pin().ToSharedRef();
    auto PersonaToolkit = AnimationEditorRef->GetPersonaToolkit();

    auto BaseSkeleton = PersonaToolkit->GetSkeleton();
    auto SkeletalMeshComponents = GetPreviewMeshComponents();
    auto AnimationAsset = PersonaToolkit->GetAnimationAsset();

    FString newNameSuggestion = FString(TEXT("export params"));
    TSharedPtr<GeometryCacheLiveSyncExportDialog> exportDialog =
        SNew(GeometryCacheLiveSyncExportDialog)
        .Title(LOCTEXT("CLOLiveSyncGCExportTitle", "CLOLiveSync GC Exporter"))
        .DefaultAssetPath(FText::FromString(newNameSuggestion))
        .BaseSkeleton(TWeakObjectPtr<USkeleton>(BaseSkeleton))
        .TargetComponents(SkeletalMeshComponents)
        .Animation(TWeakObjectPtr<UAnimationAsset>(AnimationAsset));

    if (exportDialog->ShowModal() == EAppReturnType::Ok)
    {
        UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimationAsset);

        if (AnimSequence)
        {
            USkeletalMesh* ConvertedSkelMesh = CloEditorUtils::ConvertSkeletalMeshesToSingleSkeletalMesh(SkeletalMeshComponents);

            if (ConvertedSkelMesh)
            {
                UAnimSequence* BakedAnimSequence = CloEditorUtils::CreateBakedAnimSequenceWithRootMotion(AnimSequence, ConvertedSkelMesh);

                if (BakedAnimSequence)
                {
                    TSharedRef<IPersonaPreviewScene> PreviewScene = PersonaToolkit->GetPreviewScene();
                    UDebugSkelMeshComponent* TempMergedComponent = NewObject<UDebugSkelMeshComponent>(PreviewScene->GetActor());
                    TempMergedComponent->SetSkeletalMesh(ConvertedSkelMesh);
                    PreviewScene->AddComponent(TempMergedComponent, FTransform::Identity);
                    TempMergedComponent->SetAnimation(BakedAnimSequence);

                    TArray<USkeletalMeshComponent*> ComponentsForGC;
                    ComponentsForGC.Add(TempMergedComponent);

                    UGeometryCache* GeometryCache = CloEditorUtils::ConvertAnimationAssetToGeometryCache(BakedAnimSequence, ComponentsForGC);

                    PreviewScene->RemoveComponent(TempMergedComponent);

                    ProcessExportInternal(GeometryCache);
                }
            }
        }
    }
}

void FCloAnimationToolbarExtensionHelper::ProcessExportInternal(UObject* TargetObject)
{
    if (TargetObject != nullptr)
    {
        FScopedSlowTask SlowTask(3, FText().FromString(TEXT("Sending Avatar via LiveSync..")));
        SlowTask.MakeDialog(false);
        SlowTask.EnterProgressFrame(1);

        const FString FileName = TEXT("UE_Avatar");
        const FString Extension = TEXT("usd");
        FString FullPath = FPaths::SetExtension(FPaths::Combine(CloCoreUtils::GetLiveSyncTempDirectoryPath(), FileName), Extension);

        UAssetExportTask* ExportTask = NewObject<UAssetExportTask>();
        FGCObjectScopeGuard ExportTaskGuard(ExportTask);
        ExportTask->Object = TargetObject;
        ExportTask->Exporter = UExporter::FindExporter(TargetObject, *Extension);
        ExportTask->Filename = FullPath;
        ExportTask->bSelected = false;
        ExportTask->bReplaceIdentical = true;
        ExportTask->bPrompt = false;
        ExportTask->bUseFileArchive = false;
        ExportTask->bWriteEmptyFiles = false;
        ExportTask->bReplaceIdentical = true;
        ExportTask->bAutomated = false;

        if (TargetObject->IsA<UAnimationAsset>())
        {
            UAnimSequenceExporterUSDOptions* Options = NewObject<UAnimSequenceExporterUSDOptions>();
            Options->StageOptions.UpAxis = EUsdUpAxis::YAxis;
            Options->bExportPreviewMesh = true;
            Options->PreviewMeshOptions.HighestMeshLOD = 0;
            Options->PreviewMeshOptions.bBakeMaterials = false;

            ExportTask->Options = Options;

        }
        else if (TargetObject->IsA<UGeometryCache>())
        {
            UGeometryCacheExporterUSDOptions* Options = NewObject<UGeometryCacheExporterUSDOptions>();
            Options->StageOptions.UpAxis = EUsdUpAxis::YAxis;
            Options->MeshAssetOptions.HighestMeshLOD = 0;

            ExportTask->Options = Options;
        }

        SlowTask.EnterProgressFrame(1);
        UExporter::RunAssetExportTask(ExportTask);

        FCloLiveSyncMode* Mode = static_cast<FCloLiveSyncMode*>(GLevelEditorModeTools().GetActiveMode(FCloLiveSyncMode::EM_CloLiveSync));
        if (Mode) 
            Mode->SendFilePath(FullPath);
    
        SlowTask.EnterProgressFrame(1);
    }
}

void FCloAnimationToolbarExtensionHelper::DisplayMeshInScene(USceneComponent* SceneComponent, FTransform Transform)
{
    TSharedRef<IAnimationEditor> AnimationEditorRef = AnimationEditor.Pin().ToSharedRef();
    auto PersonaToolkit = AnimationEditorRef->GetPersonaToolkit();
    auto previewScene = PersonaToolkit->GetPreviewScene();

    for (USceneComponent* comp : DisplayedComponents)
    {
        if (comp && comp->IsA<USkeletalMeshComponent>())
        {
            auto skel = Cast<USkeletalMeshComponent>(comp);
            UAnimInstance* animInst = skel->GetAnimInstance();
            if (animInst && animInst->IsA(UAnimPreviewAttacheInstance::StaticClass()))
            {
                animInst->Montage_Stop(0.0f);
            }
        }
        previewScene->RemoveComponent(comp);
    }

    DisplayedComponents.Empty();
    
    previewScene->AddComponent(SceneComponent, Transform);
    if (SceneComponent->IsA<USkeletalMeshComponent>())
    {
        auto skel = Cast<USkeletalMeshComponent>(SceneComponent);
        skel->SetAnimation(previewScene->GetPreviewAnimationAsset());
        skel->PlayAnimation(previewScene->GetPreviewAnimationAsset(), true);
    }

    DisplayedComponents.Add(SceneComponent);
}

#undef LOCTEXT_NAMESPACE
