// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "CloLiveSyncMode.h"

#include <string>

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ChaosClothAsset/ClothAsset.h"
#include "ChaosClothAsset/USDImportNode_v2.h"
#include "Dataflow/DataflowObject.h"
#include "Dialogs/Dialogs.h"
#include "Editor/AssetGuideline.h"
#include "EditorModeManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/SlateDelegates.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Internationalization/Regex.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "PackageTools.h"
#include "UObject/NameTypes.h"
#include "USDClassesModule.h"
#include "USDObjectUtils.h"
#include "USDSchemasModule.h"
#include "USDStageActor.h"
#include "USDStageImportContext.h"
#include "USDStageImporter.h"
#include "USDStageImporterModule.h"
#include "USDStageImportOptions.h"
#include "USDStageModule.h"
#include "USDStageViewModel.h"
#include "LevelSequence.h"
#include "MovieScene.h"
#include "MovieSceneBinding.h"
#include "MovieScenePossessable.h"
#include "EngineUtils.h"
#include "GeometryCache.h" 
#include "Subsystems/AssetEditorSubsystem.h"
#include "ILevelSequenceEditorToolkit.h"
#include "ISequencer.h"
#include "SequencerUtilities.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/SRichTextBlock.h"

#include "Communication/CloLiveSyncServer.h"
#include "Communication/LiveSyncProtocol.h"
#include "Editor/CloEditorNotification.h"
#include "Mode/CloLiveSyncModeToolkit.h"
#include "Objects/USDPrimLinkCache.h"
#include "Schemas/CloUsdGroomTranslator.h"
#include "Schemas/CloUsdShadeMaterialTranslator.h"
#include "Schemas/CloUsdShadeSubstanceMaterialTranslator.h"
#include "Schemas/CloUsdSkelSkeletonTranslator.h"
#include "Utils/CloLiveSyncCoreUtils.h"
#include "Widgets/ChildWidgets/CloLiveSyncOptionWidget.h"

#define LOCTEXT_NAMESPACE "CloLiveSyncMode"

const FEditorModeID FCloLiveSyncMode::EM_CloLiveSync(TEXT("EM_CloLiveSync"));

template<typename T>
T* CreateObjectInstance(const FString& InParentPackagePath, const FString& ObjectName, const EObjectFlags Flags)
{
    // Parent package to place new mesh
    UPackage* Package = nullptr;

    // Setup package name and create one accordingly
    FString NewPackageName = InParentPackagePath + TEXT("/") + ObjectName;

    NewPackageName = UPackageTools::SanitizePackageName(NewPackageName);
    Package = CreatePackage(*NewPackageName);

    const FString SanitizedObjectName = ObjectTools::SanitizeObjectName(ObjectName);

    T* ExistingTypedObject = FindObject<T>(Package, *SanitizedObjectName);
    UObject* ExistingObject = FindObject<UObject>(Package, *SanitizedObjectName);

    if (ExistingTypedObject != nullptr)
    {
        ExistingTypedObject->PreEditChange(nullptr);
    }
    else if (ExistingObject != nullptr)
    {
        // Replacing an object.  Here we go!
        // Delete the existing object
        const bool bDeleteSucceeded = ObjectTools::DeleteSingleObject(ExistingObject);

        if (bDeleteSucceeded)
        {
            // Force GC so we can cleanly create a new asset (and not do an 'in place' replacement)
            CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

            Package = CreatePackage(*NewPackageName);
        }
        else
        {
            // failed to delete
            return nullptr;
        }
    }

    return NewObject<T>(Package, FName(*SanitizedObjectName), Flags | RF_Public);
}

template<typename T>
T* DuplicateObjectInstance(T const* SourceObject, const FString& InParentPackagePath, const FString& ObjectName)
{
    // Parent package to place new mesh
    UPackage* Package = nullptr;

    // Setup package name and create one accordingly
    FString NewPackageName = InParentPackagePath + TEXT("/") + ObjectName;

    NewPackageName = UPackageTools::SanitizePackageName(NewPackageName);
    Package = CreatePackage(*NewPackageName);

    const FString SanitizedObjectName = ObjectTools::SanitizeObjectName(ObjectName);

    T* ExistingTypedObject = FindObject<T>(Package, *SanitizedObjectName);
    UObject* ExistingObject = FindObject<UObject>(Package, *SanitizedObjectName);

    if (ExistingTypedObject != nullptr)
    {
        ExistingTypedObject->PreEditChange(nullptr);
    }
    else if (ExistingObject != nullptr)
    {
        // Replacing an object.  Here we go!
        // Delete the existing object
        const bool bDeleteSucceeded = ObjectTools::DeleteSingleObject(ExistingObject);

        if (bDeleteSucceeded)
        {
            // Force GC so we can cleanly create a new asset (and not do an 'in place' replacement)
            CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

            Package = CreatePackage(*NewPackageName);
        }
        else
        {
            // failed to delete
            return nullptr;
        }
    }

    return DuplicateObject<T>(SourceObject, Package, FName(*SanitizedObjectName));
}

FCloLiveSyncMode::FCloLiveSyncMode()
{
}

FCloLiveSyncMode::~FCloLiveSyncMode()
{
    if (LiveSyncServer.IsValid())
    {
        LiveSyncServer->StopServer();
        LiveSyncServer.Reset();
    }
}

bool FCloLiveSyncMode::UsesToolkits() const
{
    return true;
}

void FCloLiveSyncMode::Initialize()
{
    TMap<FName, TArray<FName>> ValidPaletteTools;
    ValidPaletteTools.Add(CloLiveSyncModePaletteNames::LiveSync, { LiveSyncToolNames::LiveSync });
    ValidPaletteTools.Add(CloLiveSyncModePaletteNames::Extensions, { ExtensionToolNames::MeshMaterialCombiner, ExtensionToolNames::PoseToAnimationCombiner });

	CurrentPaletteTool = MakeShared<FLiveSyncPaletteTool>(ValidPaletteTools);
    LiveSyncServer = MakeUnique<CloLiveSyncServer>();
    bIsConnected = false;

    bool SubstanceInstalled = false;
    bool SubstanceEnabled = false;
    auto SubstancePlugin = IPluginManager::Get().FindPlugin("Substance");
    if (SubstancePlugin.IsValid())
    {
        SubstanceInstalled = true;
        if (FModuleManager::Get().IsModuleLoaded("SubstanceCore"))
        {
            SubstanceEnabled = true;
        }
    }
    CloCoreUtils::UpdateSubstanceStatus(SubstanceInstalled, SubstanceEnabled);
    PostUsdImportDelegateHandle.Reset();
}

void FCloLiveSyncMode::Enter()
{
    FEdMode::Enter();

    if (!Toolkit.IsValid())
    {
        Toolkit = MakeShareable(new FCloLiveSyncModeToolkit);
        Toolkit->Init(Owner->GetToolkitHost());
    }

    if (LiveSyncServer.IsValid())
    {
        LiveSyncServer->SetFilePathReceivedCallback(std::bind(&FCloLiveSyncMode::OnFilePathReceived, this, std::placeholders::_1));
        LiveSyncServer->SetClientConnectionCountChangedCallback(std::bind(&FCloLiveSyncMode::OnClientConnectionCountChanged, this, std::placeholders::_1));
        LiveSyncServer->StartServer();
    }

    PostUsdImportDelegateHandle = FUsdDelegates::OnPostUsdImport.AddLambda(
	    [this](FString FilePath) 
	    {
	    }
    );

    OpenUpdateInfoDialog();
    OpenSubstanceInfoDialog();
}

void FCloLiveSyncMode::Exit()
{
    if (LiveSyncServer.IsValid())
    {
        LiveSyncServer->StopServer();
        LiveSyncServer->SetFilePathReceivedCallback(nullptr);
        LiveSyncServer->SetClientConnectionCountChangedCallback(nullptr);
    }
    bIsConnected = false;

    FUsdDelegates::OnPostUsdImport.Remove(PostUsdImportDelegateHandle);
    PostUsdImportDelegateHandle.Reset();

    FEdMode::Exit();
}

TWeakPtr<FLiveSyncPaletteTool> FCloLiveSyncMode::GetCurrentPaletteTool() const
{
    return CurrentPaletteTool;
}

void FCloLiveSyncMode::Update(const CloLiveSync::USDOptions& InUpdateOption)
{
    LiveSyncServer->SendUpdateSceneRequest(InUpdateOption);
}

void FCloLiveSyncMode::SendFilePath(const FString& InFilePath)
{
    LiveSyncServer->SendFilePath(InFilePath);
}

void FCloLiveSyncMode::SaveUsdStage(FString& OutFolderPath)
{
#if USE_USD_SDK
    AUsdStageActor* StageActor = GetStageActor();
    if (!StageActor)
    {
        return;
    }

    const UE::FUsdStage UsdStage = StageActor->GetOrOpenUsdStage();
    if (!UsdStage)
    {
        return;
    }

    const FString RootPath = UsdStage.GetRootLayer().GetRealPath();
    FString StageName = UsdUnreal::ObjectUtils::SanitizeObjectName(FPaths::GetBaseFilename(RootPath));

    if (DoesPathContainAssets(OutFolderPath, StageName))
    {
        return;
    }

    RegisterLiveSyncUsdTranslator();
    SetUsdOutputFolderPath(OutFolderPath);

    TMap<FString, FString> RestPoseToSkelMeshMap;
    PrepareRestPoseAnimationsForSave(StageActor, OutFolderPath, RestPoseToSkelMeshMap);

    // Import directly from stage
    {
        FUsdStageImportContext ImportContext;

        // Preload some settings according to USDStage options. These will overwrite whatever is loaded from config
        ImportContext.ImportOptions->bImportActors = true;
        ImportContext.ImportOptions->bImportGeometry = true;
        ImportContext.ImportOptions->bImportSkeletalAnimations = true;
        ImportContext.ImportOptions->bImportMaterials = true;
        ImportContext.ImportOptions->bImportGroomAssets = true;
        ImportContext.ImportOptions->bImportLevelSequences = true;
        ImportContext.ImportOptions->ExistingAssetCache = StageActor->AssetCache;
        ImportContext.ImportOptions->bUseExistingAssetCache = true;
        ImportContext.ImportOptions->PurposesToImport = StageActor->PurposesToLoad;
        ImportContext.ImportOptions->RenderContextToImport = StageActor->RenderContext;
        ImportContext.ImportOptions->MaterialPurpose = StageActor->MaterialPurpose;
        ImportContext.ImportOptions->StageOptions.MetersPerUnit = UsdUtils::GetUsdStageMetersPerUnit(UsdStage);
        ImportContext.ImportOptions->StageOptions.UpAxis = UsdUtils::GetUsdStageUpAxisAsEnum(UsdStage);
        ImportContext.ImportOptions->ImportTimeCode = StageActor->GetTime();
        ImportContext.ImportOptions->NaniteTriangleThreshold = StageActor->NaniteTriangleThreshold;
        ImportContext.ImportOptions->RootMotionHandling = StageActor->RootMotionHandling;
        ImportContext.ImportOptions->SubdivisionLevel = StageActor->SubdivisionLevel;
        ImportContext.ImportOptions->MetadataOptions = StageActor->MetadataOptions;
        ImportContext.ImportOptions->KindsToCollapse = StageActor->KindsToCollapse;
        ImportContext.ImportOptions->bUsePrimKindsForCollapsing = StageActor->bUsePrimKindsForCollapsing;
        ImportContext.ImportOptions->bMergeIdenticalMaterialSlots = StageActor->bMergeIdenticalMaterialSlots;
        ImportContext.ImportOptions->bShareAssetsForIdenticalPrims = true;

        ImportContext.bReadFromStageCache = true;	 // So that we import whatever the user has open right now, even if the file has changes

        // Provide a StageName when importing transient stages as this is used for the content folder name and actor label
        if (UsdStage.GetRootLayer().IsAnonymous() && RootPath.IsEmpty())
        {
            StageName = TEXT("TransientStage");
        }

        // Pass the stage directly too in case we're importing a transient stage with no filepath
        ImportContext.Stage = UsdStage;

        if (ImportContext.Init(StageName, RootPath, TEXT("/Game/"), RF_Public | RF_Transactional | RF_Standalone, true))
        {
            FScopedTransaction Transaction(FText::Format(LOCTEXT("ImportTransaction", "Import USD stage '{0}'"), FText::FromString(StageName)));

            // Apply same conversion that FUsdStageImportContext::Init does on our received path
            ImportContext.PackagePath = FString::Printf(TEXT("%s/%s/"), *OutFolderPath, *StageName);

            // Pick the asset cache that the user potentially changed on the import options dialog. The USDStageImporter will sort itself out
            // if that happens to be nullptr/invalid
            ImportContext.UsdAssetCache = ImportContext.ImportOptions->bUseExistingAssetCache
                ? Cast<UUsdAssetCache3>(ImportContext.ImportOptions->ExistingAssetCache.TryLoad())
                : nullptr;
            ImportContext.BBoxCache = StageActor->GetBBoxCache();

            ImportContext.TargetSceneActorAttachParent = StageActor->GetRootComponent()->GetAttachParent();
            ImportContext.TargetSceneActorTargetTransform = StageActor->GetActorTransform();

            UUsdStageImporter* USDImporter = IUsdStageImporterModule::Get().GetImporter();
            USDImporter->ImportFromFile(ImportContext);

            // Convert LevelSequence possessables to spawnables
            ConvertLevelSequencesToSpawnable(OutFolderPath, StageName);

            // Adjust LevelSequence frame ranges to match USD timeSamples start frame
            AdjustLevelSequenceGCFrameRanges(OutFolderPath, StageName, UsdStage);

            // Delete the root actor created by USD import (named after the stage) and its children
            DeleteUSDStageRootActor(StageName);

            // Note that our ImportContext can't keep strong references to the assets in AssetsCache, and when
            // we CloseStage(), the stage actor will stop referencing them. The only thing keeping them alive at this point is
            // the transaction buffer, but it should be enough at least until this import is complete
            StageActor->Reset();
        }
    }

    FinalizeRestPoseAnimationLinks(RestPoseToSkelMeshMap);

    RenameAllAssetsInFolder(OutFolderPath, StageName);
#endif	  // #if USE_USD_SDK

    UnregisterLiveSyncUsdTranslator();
    ResetUsdOutputFolderPath();
    ClearUnreferencedUSDStageAssets();
}

void FCloLiveSyncMode::SaveAsClothAsset(FString& OutFolderPath)
{
    if (LatestFilePath.IsEmpty() || (FPaths::FileExists(LatestFilePath) == false))
    {
        return;
    }

    RegisterLiveSyncUsdTranslator();
    SetUsdOutputFolderPath(OutFolderPath);

    FString clothAssetName = TEXT("Cloth");

    UDataflow* const Template = LoadObject<UDataflow>(nullptr, TEXT("/ChaosClothAssetEditor/ClothAssetTemplate.ClothAssetTemplate"));
    UDataflow* const NewDataFlow = DuplicateObjectInstance<UDataflow>(Template, OutFolderPath, TEXT("DF_") + clothAssetName);
    NewDataFlow->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(NewDataFlow);

    UChaosClothAsset* ClothAsset = CreateObjectInstance<UChaosClothAsset>(OutFolderPath, TEXT("CA_") + clothAssetName, RF_Transactional | RF_Public | RF_Standalone);
    ClothAsset->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(ClothAsset);

    ClothAsset->SetDataflow(NewDataFlow);

    auto Nodes = NewDataFlow->GetDataflow()->GetNodes();
    auto terminalNodes = NewDataFlow->GetDataflow()->GetFilteredNodes(FDataflowTerminalNode::StaticType());
    for (auto node : Nodes)
    {
        if (node->GetName().ToString().Contains(TEXT("USDImport")))
        {
            auto USDImportNode = StaticCastSharedPtr<FChaosClothAssetUSDImportNode_v2>(node);
            USDImportNode->UsdFile.FilePath = LatestFilePath;
            UE::Dataflow::FContextThreaded EmptyContext;
            USDImportNode->UsdFile.Execute(EmptyContext);

            GetStageActor()->Reset();
            if (GetStageActor()->AssetCache != nullptr)
            {
                GetStageActor()->AssetCache.Get()->DeleteUnreferencedAssets(false);
                GetStageActor()->AssetCache.Get()->RescanAssetDirectory();
            }
            GetStageActor()->Destroy();
        }
    }

    UnregisterLiveSyncUsdTranslator();
    ResetUsdOutputFolderPath();
    ClearUnreferencedUSDStageAssets();
}

bool FCloLiveSyncMode::IsConnected() const
{
    return bIsConnected;
}

void FCloLiveSyncMode::OnFilePathReceived(const std::string& Utf8FilePath)
{
    FString path = FString(UTF8_TO_TCHAR(Utf8FilePath.c_str()));
    FPaths::NormalizeFilename(path);
    LatestFilePath = path;

    if (OnFilePathReceivedDelegate.IsBound())
    {
        OnFilePathReceivedDelegate.Broadcast(path);
    }

    AsyncTask(ENamedThreads::GameThread, [this]()
    {
        RegisterLiveSyncUsdTranslator();

        AUsdStageActor* StageActor = GetStageActor();

        // Mark all current assets as potentially stale before reload
        if (StageActor->AssetCache != nullptr)
        {
            StageActor->AssetCache.Get()->MarkAssetsAsStale();
        }

        // Open USD file and reload the stage with latest file content
        FUsdStageViewModel ViewModel;
        ViewModel.UsdStageActor = StageActor;
        ViewModel.OpenStage(*LatestFilePath);
        ViewModel.ReloadStage();

        ClearUnreferencedUSDStageAssets();
        UnregisterLiveSyncUsdTranslator();
        FocusOn(GetStageActor());
    });
}

void FCloLiveSyncMode::OnClientConnectionCountChanged(const int clientCount)
{
    if (clientCount == 0)
	{
        bIsConnected = false;
        FCloEditorProgressNotification::DestroyProgressNotification();
	}
    else
    {
        bIsConnected = true;
    }
}

void FCloLiveSyncMode::FocusOn(AActor* Actor)
{
#if WITH_EDITOR
    if (GEditor == nullptr)
        return;
    
    if (Actor == nullptr)
        return;

    FViewport* Viewport = GEditor->GetActiveViewport();
    if (Viewport == nullptr)
        return;

    FEditorViewportClient* ViewportClient = static_cast<FEditorViewportClient*>(Viewport->GetClient());
    if (ViewportClient == nullptr)
        return;


    FBox boundingBox = Actor->GetComponentsBoundingBox();
    
    TArray<AActor*> ChildActors;
    Actor->GetAttachedActors(ChildActors);
    for (AActor* child : ChildActors)
    {
        boundingBox += child->GetComponentsBoundingBox(true, true);
    }

    ViewportClient->FocusViewportOnBox(boundingBox);
#endif
}

void FCloLiveSyncMode::RegisterLiveSyncUsdTranslator()
{
    MapCLOTranslatorHandle.Add(TEXT("UsdShadeMaterial"), FUsdSchemaTranslatorRegistry::Get().Register<FCloUsdShadeSubstanceMaterialTranslator>(TEXT("UsdShadeMaterial")));
    MapCLOTranslatorHandle.Add(TEXT("UsdGeomXformable"), FUsdSchemaTranslatorRegistry::Get().Register<FCloUsdGroomTranslator>(TEXT("UsdGeomXformable")));
    MapCLOTranslatorHandle.Add(TEXT("UsdSkelSkeleton"), FUsdSchemaTranslatorRegistry::Get().Register<FCloUsdSkelSkeletonTranslator>(TEXT("UsdSkelSkeleton")));
}

void FCloLiveSyncMode::UnregisterLiveSyncUsdTranslator()
{
    for (const auto& handle : MapCLOTranslatorHandle)
    {
        FUsdSchemaTranslatorRegistry::Get().Unregister(handle.Value);
    }
    MapCLOTranslatorHandle.Empty();
}

FString FCloLiveSyncMode::CreateFormattedUpdateMessage(const TSharedPtr<IPlugin>& Plugin, const FString& CurrentVersion) const
{
    FString DialogMessage;
    const FString ChangelogFileName = FString::Printf(TEXT("changelog_%s.txt"), *CurrentVersion);
    const FString ChangelogFilePath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources"), TEXT("Changelog"), ChangelogFileName);

    if (FPaths::FileExists(ChangelogFilePath))
    {
        FFileHelper::LoadFileToString(DialogMessage, *ChangelogFilePath);
    }
    else
    {
        DialogMessage = FString::Printf(TEXT("%s plugin has been updated to v%s!"), *Plugin->GetName(), *CurrentVersion);
    }

    const FRegexPattern RegexPattern(TEXT("(https?://[^\\s<>]+|www\\.[^\\s<>]+)"));
    FRegexMatcher RegexMatcher(RegexPattern, DialogMessage);

    FString FormattedMessage;
    int32 LastMatchEnd = 0;

    while (RegexMatcher.FindNext())
    {
        const int32 MatchStart = RegexMatcher.GetMatchBeginning();
        const int32 MatchEnd = RegexMatcher.GetMatchEnding();

        FormattedMessage.Append(DialogMessage.Mid(LastMatchEnd, MatchStart - LastMatchEnd));
        const FString MatchedUrl = RegexMatcher.GetCaptureGroup(0);
        FormattedMessage.Append(FString::Printf(TEXT("<a id=\"browser_link\" href=\"%s\">%s</>"), *MatchedUrl, *MatchedUrl));
        LastMatchEnd = MatchEnd;
    }

    FormattedMessage.Append(DialogMessage.Mid(LastMatchEnd));

    return FormattedMessage;
}

void FCloLiveSyncMode::OpenUpdateInfoDialog()
{
    const FName PluginName = TEXT("CloLiveSync");
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName.ToString());
    if (!Plugin.IsValid())
    {
        return;
    }
    FString CurrentVersion = Plugin->GetDescriptor().VersionName;
    const FString SettingName = FString::Printf(TEXT("SuppressUpdateNotification_CloLiveSync_v%s"), *CurrentVersion);

    bool bSuppressNotification = false;
    GConfig->GetBool(TEXT("SuppressableDialogs"), *SettingName, bSuppressNotification, GEditorSettingsIni);
    if (bSuppressNotification)
    {
        return;
    }

    const FString FormattedMessage = CreateFormattedUpdateMessage(Plugin, CurrentVersion);

    TSharedRef<FHyperlinkDecorator> HyperlinkDecorator = FHyperlinkDecorator::Create("browser_link",
        FSlateHyperlinkRun::FOnClick::CreateLambda([](const FSlateHyperlinkRun::FMetadata& Metadata)
        {
            const FString* UrlToLaunch = Metadata.Find(TEXT("href"));
            if (UrlToLaunch && !UrlToLaunch->IsEmpty())
            {
                FPlatformProcess::LaunchURL(**UrlToLaunch, nullptr, nullptr);
            }
        })
    );

    TSharedPtr<SWindow> DialogWindow;
    TSharedPtr<SCheckBox> SuppressCheckBox;

    SAssignNew(DialogWindow, SWindow)
    .Title(FText::FromString(CurrentVersion + TEXT(" Release Note")))
    .SizingRule(ESizingRule::UserSized)
    .ClientSize(FVector2D(600, 400))
    .SupportsMaximize(false)
    .SupportsMinimize(false)
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .Padding(10)
        .FillHeight(1.0f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [
                SNew(SRichTextBlock)
                .Text(FText::FromString(FormattedMessage))
                .TextStyle(FAppStyle::Get(), "NormalText")
                .AutoWrapText(true)
                + SRichTextBlock::Decorator(HyperlinkDecorator)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10)
        [
            SAssignNew(SuppressCheckBox, SCheckBox)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Don't show this again"))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .HAlign(HAlign_Right)
        .Padding(10)
        [
            SNew(SButton)
            .Text(FText::FromString("OK"))
            .OnClicked_Lambda([&DialogWindow, &SuppressCheckBox, SettingName]() -> FReply
            {
	            if (SuppressCheckBox.IsValid() && SuppressCheckBox->IsChecked())
	            {
	                GConfig->SetBool(TEXT("SuppressableDialogs"), *SettingName, true, GEditorSettingsIni);
	                GConfig->Flush(false, GEditorSettingsIni);
	            }
	            if (DialogWindow.IsValid())
	            {
	                DialogWindow->RequestDestroyWindow();
	            }

	            return FReply::Handled();
            })
        ]
    ];

    FSlateApplication::Get().AddModalWindow(DialogWindow.ToSharedRef(), nullptr);
}

void FCloLiveSyncMode::OpenSubstanceInfoDialog()
{
    const FText TitleText = FText::FromString("Substance Required");

    if (!CloCoreUtils::IsSubstanceInstalled())
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Please install 'Substance 3D for Unreal Engine' plugin to use Substance materials applied to CLO garments, otherwise default materials will be applied to garments."), TitleText);
    }
    else if (!CloCoreUtils::IsSubstanceEnabled())
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Please enable 'Substance 3D for Unreal Engine' plugin to use Substance materials applied to CLO garments, otherwise default materials will be applied to garments."), TitleText);
    }
}

void FCloLiveSyncMode::SetUsdOutputFolderPath(const FString& OutputPath)
{
    if (const auto& Cache = GetStageActor()->AssetCache; Cache != nullptr)
    {
        CloCoreUtils::SetUsdAssetCacheFolderPath(Cache.Get()->AssetDirectory.Path);
    }
    CloCoreUtils::SetUsdOutputContentFolderPath(OutputPath);
}

void FCloLiveSyncMode::ResetUsdOutputFolderPath()
{
    if (const auto& Cache = GetStageActor()->AssetCache; Cache != nullptr)
    {
        CloCoreUtils::SetUsdAssetCacheFolderPath(Cache.Get()->AssetDirectory.Path);
    }
    CloCoreUtils::SetUsdOutputContentFolderPath(CloCoreUtils::GetUsdAssetCacheFolderPath());
}

void FCloLiveSyncMode::RenameAllAssetsInFolder(const FString& OutputPath, const FString& USDStageName)
{
    const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");

    const FString TargetPath = FString::Printf(TEXT("%s/%s"), *OutputPath, *USDStageName);

    TArray<FAssetData> AssetDataList;
    AssetRegistryModule.Get().GetAssetsByPath(FName(*TargetPath), AssetDataList, true);

    if (AssetDataList.Num() == 0) return;

    TArray<FAssetRenameData> AssetsToRename;
    const FString AbsoluteContentPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    const int MaxTotalPathLength = 260; // Windows Max Path Size
    const int ExtensionLength = 7;      // ".uasset"

    // Helper Lambda to generate safe asset name
    auto GetSafeNewName = [&](const FString& BaseName, const FString& Suffix, int32 MaxLen) -> FString
        {
            FString CandidateName = FString::Printf(TEXT("%s_%s%s"), *USDStageName, *BaseName, *Suffix);

            if (CandidateName.Len() <= MaxLen)
            {
                return CandidateName;
            }

            const uint32 Hash = FCrc::StrCrc32(*BaseName);
            const FString UID = FString::Printf(TEXT("%08X"), Hash);

            // Calculate overhead: <ProjectName>_ + <BaseName>_ + <UID> + <Suffix>
            // Two underscores: one after ProjectName, one after BaseName
            int32 OverheadLength = USDStageName.Len() + 1 + 1 + UID.Len() + Suffix.Len();

            const int32 MaxBaseNameLength = MaxLen - OverheadLength;
            FString TruncatedBaseName = BaseName.Left(FMath::Max(0, MaxBaseNameLength));

            // Result: <Project>_<TruncatedName>_<UID><Suffix>
            // Note: Suffix already contains "_" if needed (e.g., "_GC", "_SM")
            return FString::Printf(TEXT("%s_%s_%s%s"), *USDStageName, *TruncatedBaseName, *UID, *Suffix);
        };

    for (const FAssetData& AssetData : AssetDataList)
    {
        const FString RelativeAssetPath = AssetData.PackagePath.ToString().RightChop(5);
        const FString AbsoluteAssetFolderPath = FPaths::Combine(AbsoluteContentPath, RelativeAssetPath);

        // Calculate Max Name Length allowed for this specific asset
        const int MaxAssetNameLength = MaxTotalPathLength - (AbsoluteAssetFolderPath.Len() + 1 + ExtensionLength);

        if (MaxAssetNameLength <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("Path is too long for asset: %s"), *AssetData.AssetName.ToString());
            continue;
        }

        const FString CurrentName = AssetData.AssetName.ToString();
        FString NewName;

        // GeometryCache Handling
        if (AssetData.AssetClassPath == UGeometryCache::StaticClass()->GetClassPathName())
        {
            FString MeshName = CurrentName;
            // Remove existing "GC_" prefix if present to avoid duplication
            if (MeshName.StartsWith(TEXT("GC_")))
            {
                MeshName = MeshName.RightChop(3);
            }

            // Apply Rule: <Project>_<MeshName>_GC
            NewName = GetSafeNewName(MeshName, TEXT("_GC"), MaxAssetNameLength);
        }
        // LevelSequence Handling
        else if (AssetData.AssetClassPath == ULevelSequence::StaticClass()->GetClassPathName())
        {
            FString Prefix, RestOfName;
            if (!CurrentName.Split(TEXT("_"), &Prefix, &RestOfName))
            {
                Prefix = TEXT("");
                RestOfName = CurrentName;
            }

            FString SuffixString = Prefix.IsEmpty() ? TEXT("") : (TEXT("_") + Prefix);
            NewName = RestOfName + SuffixString;
        }
        // Default Handling (StaticMesh, SkeletalMesh, etc.)
        else
        {
            FString Prefix, RestOfName;
            if (!CurrentName.Split(TEXT("_"), &Prefix, &RestOfName))
            {
                Prefix = TEXT("");
                RestOfName = CurrentName;
            }

            FString SuffixString = Prefix.IsEmpty() ? TEXT("") : (TEXT("_") + Prefix);

            // Apply Rule: <Project>_<Name>_<Prefix> (Prefix is moved to end)
            NewName = GetSafeNewName(RestOfName, SuffixString, MaxAssetNameLength);
        }

        // Rename Execution
        UObject* AssetObject = AssetData.GetAsset();
        if (AssetObject)
        {
            if (!CurrentName.Equals(NewName))
            {
                AssetsToRename.Emplace(AssetObject, AssetData.PackagePath.ToString(), NewName);
            }
        }
    }

    if (AssetsToRename.Num() > 0)
    {
        if (!AssetToolsModule.Get().RenameAssets(AssetsToRename))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to rename assets. Check for naming conflicts."));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Asset renaming process finished."));
}

bool FCloLiveSyncMode::DoesPathContainAssets(const FString& OutputPath, const FString& USDStageName)
{
    const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");


    const FString OutputAssetFolderPath = FPaths::Combine(OutputPath, USDStageName);
    TArray<FAssetData> AssetDataList;
    AssetRegistryModule.Get().GetAssetsByPath(FName(*OutputAssetFolderPath), AssetDataList, true);

    if (!AssetDataList.IsEmpty())
    {
        FText DialogText = FText::FromString(
            FString::Printf(
                TEXT("Assets already exist in '%s'.\nPlease select a new path and try again."),
                *OutputAssetFolderPath
            )
        );

        FText DialogTitle = FText::FromString(TEXT("Warning"));

        FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);

    	return true;
    }

    return false;
}

void FCloLiveSyncMode::PrepareRestPoseAnimationsForSave(AUsdStageActor* StageActor, const FString& TargetFolderPath, TMap<FString, FString>& OutRestPoseToSkelMeshMap)
{
    if (!StageActor)
    {
        return;
    }

    const UE::FUsdStage UsdStage = StageActor->GetOrOpenUsdStage();
    if (!UsdStage)
    {
        return;
    }

    if (StageActor->PrimLinkCache)
    {
        for (const TPair<UE::FSdfPath, TArray<TWeakObjectPtr<UObject>>>& PrimPathToAssets : StageActor->PrimLinkCache->GetInner().GetAllAssetPrimLinks())
        {
            const UE::FSdfPath& PrimPath = PrimPathToAssets.Key;
            if (PrimPath.GetString().IsEmpty())
            {
                continue;
            }

            UE::FUsdPrim Prim = UsdStage.GetPrimAtPath(PrimPath);
            if (!Prim.GetTypeName().IsEqual(TEXT("Skeleton")))
            {
                continue;
            }

            if (USkeletalMeshComponent* Component = Cast<USkeletalMeshComponent>(StageActor->GetGeneratedComponent(PrimPath.GetString())))
            {
                if (UAnimSequence* AnimToPlay = Cast<UAnimSequence>(Component->AnimationData.AnimToPlay))
                {
                    if (AnimToPlay->GetName().EndsWith(TEXT("_RestPose")))
                    {
                        const FString DesiredName = AnimToPlay->GetName();
                        const FString RootPath = UsdStage.GetRootLayer().GetRealPath();
                        FString StageName = UsdUnreal::ObjectUtils::SanitizeObjectName(FPaths::GetBaseFilename(RootPath));
                        StageName.ReplaceInline(TEXT(" "), TEXT("_"));

                        const FString PackageBasePath = TargetFolderPath + TEXT("/") + StageName + TEXT("/SkeletalMeshes");
                        const FString PackagePath = UPackageTools::SanitizePackageName(PackageBasePath + TEXT("/") + DesiredName);

                        UPackage* Package = FindPackage(nullptr, *PackagePath);
                        if (Package)
                        {
                            if (UObject* ExistingObject = FindObject<UObject>(Package, *DesiredName))
                            {
                                ObjectTools::DeleteSingleObject(ExistingObject);
                                CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
                            }
                        }

                        Package = CreatePackage(*PackagePath);

                        if (UAnimSequence* SavedAnim = DuplicateObject<UAnimSequence>(AnimToPlay, Package, FName(*DesiredName)))
                        {
                            SavedAnim->ClearFlags(RF_Transient);
                            SavedAnim->SetFlags(RF_Public | RF_Standalone);

                            if (USkeletalMesh* SkelMesh = Component->GetSkeletalMeshAsset())
                            {
                                OutRestPoseToSkelMeshMap.Add(SavedAnim->GetPathName(), FPaths::GetBaseFilename(SkelMesh->GetPathName()));
                            }

                            SavedAnim->MarkPackageDirty();
                            FAssetRegistryModule::AssetCreated(SavedAnim);
                        }
                    }
                }
            }
        }
    }

}

void FCloLiveSyncMode::FinalizeRestPoseAnimationLinks(const TMap<FString, FString>& InRestPoseToSkelMeshMap)
{
    if (InRestPoseToSkelMeshMap.Num() > 0)
    {
        IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

        for (const TPair<FString, FString>& Pair : InRestPoseToSkelMeshMap)
        {
            UAnimSequence* SavedRestPoseAnim = LoadObject<UAnimSequence>(nullptr, *Pair.Key);
            if (!SavedRestPoseAnim)
            {
                continue;
            }

            FString OriginalSkelMeshName = Pair.Value;
            const FString PathToSearch = FPaths::GetPath(Pair.Key);
            TArray<FAssetData> FoundAssets;
            AssetRegistry.GetAssetsByPath(FName(*PathToSearch), FoundAssets, true);

            USkeletalMesh* NewSkelMesh = nullptr;
            for (const FAssetData& AssetData : FoundAssets)
            {
                if (AssetData.AssetClassPath == USkeletalMesh::StaticClass()->GetClassPathName() &&
                    OriginalSkelMeshName.StartsWith(AssetData.AssetName.ToString()))
                {
                    NewSkelMesh = Cast<USkeletalMesh>(AssetData.GetAsset());
                    if (NewSkelMesh)
                    {
                        break;
                    }
                }
            }

            if (NewSkelMesh && NewSkelMesh->GetSkeleton())
            {
                SavedRestPoseAnim->ReplaceSkeleton(NewSkelMesh->GetSkeleton());
                SavedRestPoseAnim->SetPreviewSkeletalMesh(NewSkelMesh);
                SavedRestPoseAnim->PostEditChange();
                SavedRestPoseAnim->MarkPackageDirty();
                SavedRestPoseAnim->BeginCacheDerivedDataForCurrentPlatform();
            }
        }
    }
}

void FCloLiveSyncMode::ConvertLevelSequencesToSpawnable(const FString& OutputPath, const FString& USDStageName)
{
    const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // Find all LevelSequence assets in the output folder
    const FString OutputAssetFolderPath = FPaths::Combine(OutputPath, USDStageName);
    TArray<FAssetData> AssetDataList;

    FARFilter Filter;
    Filter.ClassPaths.Add(ULevelSequence::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(*OutputAssetFolderPath));
    Filter.bRecursivePaths = true;
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return;
    }

    for (const FAssetData& AssetData : AssetDataList)
    {
        ULevelSequence* LevelSequence = Cast<ULevelSequence>(AssetData.GetAsset());
        if (!LevelSequence || !LevelSequence->GetMovieScene())
        {
            continue;
        }

        UMovieScene* MovieScene = LevelSequence->GetMovieScene();

        // Open the level sequence to get a sequencer instance
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(LevelSequence);

        // Find the editor toolkit for this asset
        IAssetEditorInstance* EditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(LevelSequence, false);
        ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(EditorInstance);

        if (LevelSequenceEditor)
        {
            TSharedPtr<ISequencer> Sequencer = LevelSequenceEditor->GetSequencer();

            if (Sequencer.IsValid())
            {
                // Collect all root-level possessables to convert
                TArray<FGuid> PossessablesToConvert;

                for (int32 i = 0; i < MovieScene->GetPossessableCount(); ++i)
                {
                    FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);

                    // Only process root-level possessables (those without parents)
                    if (!Possessable.GetParent().IsValid())
                    {
                        PossessablesToConvert.Add(Possessable.GetGuid());
                    }
                }

                // Convert each possessable to spawnable using the official Sequencer utility
                for (const FGuid& PossessableGuid : PossessablesToConvert)
                {
                    FSequencerUtilities::ConvertToSpawnable(Sequencer.ToSharedRef(), PossessableGuid);
                }

                // Force evaluate to update the sequencer state
                Sequencer->ForceEvaluate();
            }

            // Close the level sequence editor
            GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseAllEditorsForAsset(LevelSequence);
        }

        LevelSequence->MarkPackageDirty();
    }
}

void FCloLiveSyncMode::AdjustLevelSequenceGCFrameRanges(const FString& OutputPath, const FString& USDStageName, const UE::FUsdStage& UsdStage)
{
#if USE_USD_SDK
    if (!UsdStage)
    {
        return;
    }

    const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // Find all LevelSequence assets in the output folder
    const FString OutputAssetFolderPath = FPaths::Combine(OutputPath, USDStageName);
    TArray<FAssetData> AssetDataList;

    FARFilter Filter;
    Filter.ClassPaths.Add(ULevelSequence::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(*OutputAssetFolderPath));
    Filter.bRecursivePaths = true;
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

    // Get USD stage TimeCodesPerSecond (frame rate)
    double UsdTimeCodesPerSecond = UsdStage.GetTimeCodesPerSecond();

    for (const FAssetData& AssetData : AssetDataList)
    {
        ULevelSequence* LevelSequence = Cast<ULevelSequence>(AssetData.GetAsset());
        if (!LevelSequence || !LevelSequence->GetMovieScene())
        {
            continue;
        }

        const UMovieScene* MovieScene = LevelSequence->GetMovieScene();
        FFrameRate TickResolution = MovieScene->GetTickResolution();

        // Adjust GeometryCache track sections based on USD prim timeSamples
        for (const FMovieSceneBinding& Binding : MovieScene->GetBindings())
        {
            for (UMovieSceneTrack* Track : Binding.GetTracks())
            {
                if (Track && Track->GetClass()->GetName().Contains(TEXT("GeometryCache")))
                {
                    for (UMovieSceneSection* Section : Track->GetAllSections())
                    {
                        if (!Section) continue;

                        // Get GeometryCacheAsset from section using reflection
                        FProperty* ParamsProperty = Section->GetClass()->FindPropertyByName(TEXT("Params"));
                        if (!ParamsProperty) continue;

                        FStructProperty* ParamsStructProp = CastField<FStructProperty>(ParamsProperty);
                        if (!ParamsStructProp) continue;

                        void* ParamsPtr = ParamsProperty->ContainerPtrToValuePtr<void>(Section);
                        FProperty* AssetProperty = ParamsStructProp->Struct->FindPropertyByName(TEXT("GeometryCacheAsset"));
                        if (!AssetProperty) continue;

                        FObjectProperty* ObjectProp = CastField<FObjectProperty>(AssetProperty);
                        UGeometryCache* GeoCacheAsset = ObjectProp ? Cast<UGeometryCache>(ObjectProp->GetObjectPropertyValue_InContainer(ParamsPtr)) : nullptr;
                        if (!GeoCacheAsset) continue;

                        // Get USD prim path from GeometryCache UserData
                        const TArray<UAssetUserData*>* UserDataArray = GeoCacheAsset->GetAssetUserDataArray();
                        FString PrimPath;

                        if (UserDataArray)
                        {
                            for (UAssetUserData* UserData : *UserDataArray)
                            {
                                if (UserData && UserData->GetClass()->GetName().Contains(TEXT("UsdGeometryCacheAssetUserData")))
                                {
                                    FProperty* PrimPathsProperty = UserData->GetClass()->FindPropertyByName(TEXT("PrimPaths"));
                                    if (PrimPathsProperty)
                                    {
                                        FArrayProperty* ArrayProp = CastField<FArrayProperty>(PrimPathsProperty);
                                        if (ArrayProp)
                                        {
                                            FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(UserData));
                                            if (ArrayHelper.Num() > 0)
                                            {
                                                FStrProperty* StrProp = CastField<FStrProperty>(ArrayProp->Inner);
                                                if (StrProp)
                                                {
                                                    PrimPath = StrProp->GetPropertyValue(ArrayHelper.GetRawPtr(0));
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (PrimPath.IsEmpty())
                        {
                            continue;
                        }

                        // Get USD prim and timeSamples
                        UE::FUsdPrim Prim = UsdStage.GetPrimAtPath(UE::FSdfPath(*PrimPath));
                        if (!Prim)
                        {
                            continue;
                        }

                        UE::FUsdAttribute PointsAttr = Prim.GetAttribute(TEXT("points"));
                        if (!PointsAttr)
                        {
                            continue;
                        }

                        TArray<double> TimeSamples;
                        PointsAttr.GetTimeSamples(TimeSamples);

                        if (TimeSamples.Num() == 0)
                        {
                            continue;
                        }

                        // USD timeSamples are frame numbers, convert to LevelSequence ticks
                        double UsdStartFrame = TimeSamples[0];
                        double UsdEndFrame = TimeSamples.Last();

                        // Convert USD frames to seconds
                        double StartTimeSeconds = UsdStartFrame / UsdTimeCodesPerSecond;
                        double EndTimeSeconds = UsdEndFrame / UsdTimeCodesPerSecond;

                        // Convert seconds to LevelSequence ticks
                        FFrameNumber LevelSeqStartFrame = FFrameTime::FromDecimal(StartTimeSeconds * TickResolution.AsDecimal()).RoundToFrame();
                        FFrameNumber LevelSeqEndFrame = FFrameTime::FromDecimal(EndTimeSeconds * TickResolution.AsDecimal()).RoundToFrame();

                        // Set the section range to match USD timeSamples
                        Section->SetRange(TRange<FFrameNumber>(LevelSeqStartFrame, LevelSeqEndFrame));
                        Section->MarkAsChanged();
                    }
                }
            }
        }

        LevelSequence->MarkPackageDirty();
    }
#endif // USE_USD_SDK
}

void FCloLiveSyncMode::DeleteUSDStageRootActor(const FString& USDStageName)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return;
    }

    // Find and delete the root actor created by USD import (named after the stage) and its children
    for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        if (!Actor)
        {
            continue;
        }

        // Check ActorLabel for exact match only to avoid deleting unrelated actors
        FString ActorLabel = Actor->GetActorLabel();

        if (ActorLabel == USDStageName)
        {
            TArray<AActor*> AttachedActors;
            Actor->GetAttachedActors(AttachedActors);

            for (AActor* ChildActor : AttachedActors)
            {
                if (IsValid(ChildActor))
                {
                    World->DestroyActor(ChildActor);
                }
            }

            World->DestroyActor(Actor);
            break;
        }
    }
}

void FCloLiveSyncMode::ClearUnreferencedUSDStageAssets()
{
    AsyncTask(ENamedThreads::GameThread, [this]()
    {
        // Force garbage collection to update reference tracking
        CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

        // Now delete unreferenced assets after GC has updated references
        // Assets that are still referenced by the new stage will be kept
        AUsdStageActor* StageActor = GetStageActor();
        if (StageActor->AssetCache != nullptr)
        {
            StageActor->AssetCache.Get()->DeleteUnreferencedAssets(false);
        }

    });
}

AUsdStageActor* FCloLiveSyncMode::GetStageActor()
{
    AUsdStageActor* resultActor = nullptr;

    if (!WeakStageActorPtr.IsValid())
    {
        IUsdStageModule& UsdStageModule = FModuleManager::GetModuleChecked<IUsdStageModule>(TEXT("USDStage"));
        WeakStageActorPtr = &UsdStageModule.GetUsdStageActor(IUsdClassesModule::GetCurrentWorld());

        // Default settings for USD Stage Actor
        WeakStageActorPtr->SetNaniteTriangleThreshold(100000000);
    	WeakStageActorPtr->SetGeometryCacheImport(EGeometryCacheImport::OnSave);
    }

    return WeakStageActorPtr.Get();
}

#undef LOCTEXT_NAMESPACE
