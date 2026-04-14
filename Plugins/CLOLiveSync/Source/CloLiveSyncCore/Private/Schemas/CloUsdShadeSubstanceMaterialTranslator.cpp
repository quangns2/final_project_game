// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "Schemas/CloUsdShadeSubstanceMaterialTranslator.h"

#if USE_USD_SDK
#if WITH_EDITOR
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Objects/USDPrimLinkCache.h"
#include "USDAssetUserData.h"
#endif
#include "MaterialDomain.h"
#include "Runtime/RHI/Public/RHIShaderPlatform.h"

#include "CloLiveSyncCore.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Utils/CloLiveSyncCoreUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"

const FString PARENT_SUBSTANCE_MAT_PATH = TEXT("/Script/Engine.Material'/CLOLiveSync/Material/Substance/CLOSubstance_Standard_Template.CLOSubstance_Standard_Template'");
const FString SUBSTANCE_PRESET_BPL_PATH = TEXT("/CLOLiveSync/Blueprint/SubstancePresetBPLibrary.SubstancePresetBPLibrary_C");
const TArray<FName> SUBSTANCE_TEXTURE_NAMES = {
    FName(TEXT("baseColor")),
    FName(TEXT("metallic")),
    FName(TEXT("roughness")),
    FName(TEXT("emissive")),
    FName(TEXT("normal")),
    FName(TEXT("ambientOcclusion")),
    FName(TEXT("inverted_ior")),
    FName(TEXT("opacity"))
};


static bool GUseInterchangeSubstanceMaterialCLOTranslator = true;
static FAutoConsoleVariableRef CvarUseInterchangeSubstanceMaterialCLOTranslator(
	TEXT("USD.UseInterchangeCLOSubstanceMaterialTranslator"),
    GUseInterchangeSubstanceMaterialCLOTranslator,
	TEXT(""));

FName FCloUsdShadeSubstanceMaterialTranslator::CLOUsdSubstanceRenderContext = TEXT("cloSubstance");

namespace CLO
{
    namespace Private
    {
        UObject* PrepareSubstanceFactoryCDO()
        {
            const TCHAR* FactoryClassPath = TEXT("/Script/SubstanceEditor.SubstanceFactory");
            UClass* SubstanceFactoryClass = FindObject<UClass>(nullptr, FactoryClassPath);
            if (!SubstanceFactoryClass) {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("PrepareSubstanceFactoryCDO: USubstanceFactory Class not found: %s."), FactoryClassPath);
                return nullptr;
            }

            UObject* FactoryCDO_Object = SubstanceFactoryClass->GetDefaultObject();
            if (!FactoryCDO_Object)
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("PrepareSubstanceFactoryCDO: Could not get CDO for USubstanceFactory."));
                return nullptr;
            }

            FName SuppressFuncName = FName("SuppressImportOverwriteDialog");
            UFunction* SuppressFunc = FactoryCDO_Object->FindFunction(SuppressFuncName);
            if (SuppressFunc)
            {
                FactoryCDO_Object->ProcessEvent(SuppressFunc, nullptr);
                UE_LOG(LogCloLiveSyncCore, Log, TEXT("PrepareSubstanceFactoryCDO: Called SuppressImportOverwriteDialog on Factory CDO."));
            }
            else
            {
                UE_LOG(LogCloLiveSyncCore, Warning, TEXT("PrepareSubstanceFactoryCDO: Could not find '%s' on Factory CDO."), *SuppressFuncName.ToString());
            }
            return FactoryCDO_Object;
        }

        UObject* CallImportArchive(UObject* FactoryCDO, const FString& DestinationPath, const FString& SourceFilename)
        {
            if (!FactoryCDO) return nullptr;
            UObject* ResultObject = nullptr;
            FName FuncName = FName("ImportArchive");
            UFunction* Func = FactoryCDO->FindFunction(FuncName);
            if (Func)
            {
                FStructOnScope FuncParams(Func);
                uint8* ParamsMemory = FuncParams.GetStructMemory();
                FStrProperty* DestPathProp = FindFProperty<FStrProperty>(Func, FName("destinationPpath"));
                FStrProperty* FilenameProp = FindFProperty<FStrProperty>(Func, FName("filename"));
                FObjectPropertyBase* ReturnProp = CastField<FObjectPropertyBase>(Func->GetReturnProperty());

                if (DestPathProp && FilenameProp && ReturnProp)
                {
                    DestPathProp->SetPropertyValue_InContainer(ParamsMemory, DestinationPath);
                    FilenameProp->SetPropertyValue_InContainer(ParamsMemory, SourceFilename);
                    ReturnProp->InitializeValue_InContainer(ParamsMemory);
                    FactoryCDO->ProcessEvent(Func, ParamsMemory);
                    ResultObject = ReturnProp->GetObjectPropertyValue_InContainer(ParamsMemory);
                }
                else
                {
                    UE_LOG(LogCloLiveSyncCore, Error, TEXT("CallImportArchive: Failed to find properties for ImportArchive function."));
                }
            }
            else
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("CallImportArchive: Could not find UFunction '%s'."), *FuncName.ToString());
            }
            return ResultObject;
        }

        bool GetFirstGraphDescData(UObject* InstanceFactory, TArray<uint8>& OutRawData)
        {
            OutRawData.Empty();
            if (!InstanceFactory)
            {
                return false;
            }

            FName FuncName = FName("GetGraphDescs");
            UFunction* Func = InstanceFactory->FindFunction(FuncName);
            if (!Func)
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("GetFirstGraphDescData: Could not find UFunction '%s'."), *FuncName.ToString());
                return false;
            }

            FArrayProperty* ReturnProp = CastField<FArrayProperty>(Func->GetReturnProperty());
            FStructProperty* InnerProp = ReturnProp ? CastField<FStructProperty>(ReturnProp->Inner) : nullptr;
            UScriptStruct* StructType = InnerProp ? InnerProp->Struct : nullptr;
            if (!ReturnProp || !InnerProp || !StructType)
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("GetFirstGraphDescData: Failed to get return property info for GetGraphDescs."));
                return false;
            }

            FStructOnScope FuncResult(Func);
            InstanceFactory->ProcessEvent(Func, FuncResult.GetStructMemory());
            FScriptArrayHelper ArrayHelper(ReturnProp, ReturnProp->ContainerPtrToValuePtr<void>(FuncResult.GetStructMemory()));
            if (ArrayHelper.Num() > 0)
            {
                int32 StructSize = StructType->GetStructureSize();
                if (StructSize > 0)
                {
                    OutRawData.SetNumUninitialized(StructSize);
                    FMemory::Memcpy(OutRawData.GetData(), ArrayHelper.GetRawPtr(0), StructSize);
                    return true;
                }
                else
                {
                    UE_LOG(LogCloLiveSyncCore, Error, TEXT("GetFirstGraphDescData: Graph description struct size is zero."));
                }
            }
            else
            {
                UE_LOG(LogCloLiveSyncCore, Warning, TEXT("GetFirstGraphDescData: GetGraphDescs() returned an empty array."));
            }
            return false;
        }

        UObject* CallCreateGraphInstance(UObject* InstanceFactory, const TArray<uint8>& GraphDescData, const FString& PackageName)
        {
            if (!InstanceFactory || GraphDescData.IsEmpty())
            {
                return nullptr;
            }
            FName FuncName = FName("CreateGraphInstance");
            UFunction* Func = InstanceFactory->FindFunction(FuncName);
            if (!Func)
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("CallCreateGraphInstance: Could not find UFunction '%s'."), *FuncName.ToString());
                return nullptr;
            }

            FStructOnScope FuncParams(Func);
            uint8* ParamsMemory = FuncParams.GetStructMemory();
            FStructProperty* GraphDescProp = FindFProperty<FStructProperty>(Func, FName("GraphDesc"));
            FStrProperty* PackageNameProp = FindFProperty<FStrProperty>(Func, FName("PackageName"));
            FObjectPropertyBase* ReturnProp = CastField<FObjectPropertyBase>(Func->GetReturnProperty());

            if (GraphDescProp && PackageNameProp && ReturnProp && GraphDescProp->Struct)
            {
                int32 ExpectedSize = GraphDescProp->Struct->GetStructureSize();
                if (GraphDescData.Num() == ExpectedSize) {
                    void* DestGraphDescPtr = GraphDescProp->ContainerPtrToValuePtr<void>(ParamsMemory);
                    FMemory::Memcpy(DestGraphDescPtr, GraphDescData.GetData(), ExpectedSize);
                    PackageNameProp->SetPropertyValue_InContainer(ParamsMemory, PackageName);
                    ReturnProp->InitializeValue_InContainer(ParamsMemory);
                    InstanceFactory->ProcessEvent(Func, ParamsMemory);
                    return ReturnProp->GetObjectPropertyValue_InContainer(ParamsMemory);
                }
                else
                {
                    UE_LOG(LogCloLiveSyncCore, Error, TEXT("CallCreateGraphInstance: Graph description data size mismatch."));
                }
            }
            else
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("CallCreateGraphInstance: Failed to find function properties (parameters/return value)."));
            }
            return nullptr;
        }

        bool CallGraphInstanceSetupSequence(UObject* GraphInstance, const FString& MaterialPkgName, UMaterial* ParentMat, bool bForceSave)
        {
            if (!GraphInstance || !ParentMat)
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("CallGraphInstanceSetupSequence: Input object (GraphInstance or ParentMat) is null."));
                return false;
            }

            bool bSetupOk = true;
            auto CallVoidFunc = [&](FName FuncName)
                {
                    if (!bSetupOk) return;
                    UFunction* Func = GraphInstance->FindFunction(FuncName);
                    if (Func)
                    {
                        UE_LOG(LogCloLiveSyncCore, Log, TEXT("Calling %s..."), *FuncName.ToString()); GraphInstance->ProcessEvent(Func, nullptr);
                    }
                    else
                    {
                        UE_LOG(LogCloLiveSyncCore, Error, TEXT("Failed to find '%s'."), *FuncName.ToString());
                        bSetupOk = false;
                    }
                };

            // 순서대로 호출
            CallVoidFunc(FName("CreateOutputs"));
            CallVoidFunc(FName("PrepareOutputsForSave"));
            CallVoidFunc(FName("RenderSync"));

            if (bSetupOk)
            {
                FName FuncName = FName("SaveAllOutputs");
                UFunction* Func = GraphInstance->FindFunction(FuncName);
                if (Func)
                {
                    FStructOnScope SaveParams(Func);
                    uint8* SaveMemory = SaveParams.GetStructMemory();
                    FBoolProperty* ForceSaveProp = FindFProperty<FBoolProperty>(Func, FName("ForceSave"));
                    if (ForceSaveProp)
                    {
                        ForceSaveProp->SetPropertyValue_InContainer(SaveMemory, bForceSave);
                        UE_LOG(LogCloLiveSyncCore, Log, TEXT("Calling SaveAllOutputs(ForceSave=%s)..."), bForceSave ? TEXT("true") : TEXT("false"));
                        GraphInstance->ProcessEvent(Func, SaveMemory);
                    }
                    else
                    {
                        UE_LOG(LogCloLiveSyncCore, Error, TEXT("Failed to find 'ForceSave' parameter for '%s'."), *FuncName.ToString());
                        bSetupOk = false;
                    }
                }
                else
                {
                    UE_LOG(LogCloLiveSyncCore, Error, TEXT("Failed to find '%s'."), *FuncName.ToString());
                    bSetupOk = false;
                }
            }

            if (bSetupOk)
            {
                FName FuncName = FName("CreateMaterial");
                UFunction* Func = GraphInstance->FindFunction(FuncName);
                if (Func)
                {
                    FStructOnScope CreateParams(Func);
                    uint8* CreateMemory = CreateParams.GetStructMemory();
                    FStrProperty* PkgNameProp = FindFProperty<FStrProperty>(Func, FName("PackageName"));
                    FObjectProperty* ParentMatProp = FindFProperty<FObjectProperty>(Func, FName("ParentMaterial"));
                    if (PkgNameProp && ParentMatProp)
                    {
                        PkgNameProp->SetPropertyValue_InContainer(CreateMemory, MaterialPkgName);
                        ParentMatProp->SetObjectPropertyValue_InContainer(CreateMemory, ParentMat);
                        UE_LOG(LogCloLiveSyncCore, Log, TEXT("Calling CreateMaterial... Target: %s"), *MaterialPkgName);
                        GraphInstance->ProcessEvent(Func, CreateMemory);
                    }
                    else
                    {
                        UE_LOG(LogCloLiveSyncCore, Error, TEXT("Failed to find parameters for '%s'."), *FuncName.ToString());
                        bSetupOk = false;
                    }
                }
                else
                {
                    UE_LOG(LogCloLiveSyncCore, Error, TEXT("Failed to find '%s'."), *FuncName.ToString());
                    bSetupOk = false;
                }
            }

            return bSetupOk;
        }

        bool CallSetSubstancePreset(UObject* SubstanceGraphInstance, const FString& PresetXml)
        {
            UClass* BflClass = LoadObject<UClass>(nullptr, *SUBSTANCE_PRESET_BPL_PATH);

            if (!BflClass)
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("CallSetSubstancePreset: Failed to load BFL Class: %s"), *SUBSTANCE_PRESET_BPL_PATH);
                return false;
            }

            UObject* BflCdo = BflClass->GetDefaultObject();
            if (!BflCdo)
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("CallSetSubstancePreset: Failed to get CDO for BFL Class: %s"), *SUBSTANCE_PRESET_BPL_PATH);
                return false;
            }

            FName FunctionName = FName("SetSubstancePreset");
            UFunction* FunctionPtr = BflClass->FindFunctionByName(FunctionName);

            if (!FunctionPtr)
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("CallSetSubstancePreset: Failed to find function %s in BFL Class: %s"), *FunctionName.ToString(), *SUBSTANCE_PRESET_BPL_PATH);
                return false;
            }

            struct FSetSubstancePreset_Params
            {
                UObject* InSubstanceGraphInstance;
                FString InPresetXML;
            };

            FSetSubstancePreset_Params Params;
            Params.InSubstanceGraphInstance = SubstanceGraphInstance;
            Params.InPresetXML = PresetXml;

            if (!IsValid(SubstanceGraphInstance))
            {
                UE_LOG(LogCloLiveSyncCore, Error, TEXT("CallSetSubstancePreset: TargetInstance is invalid!"));
                return false;
            }

            UE_LOG(LogCloLiveSyncCore, Log, TEXT("CallSetSubstancePreset: Calling ProcessEvent for %s"), *FunctionName.ToString());
            BflCdo->ProcessEvent(FunctionPtr, &Params);

            UE_LOG(LogCloLiveSyncCore, Log, TEXT("CallSetSubstancePreset: ProcessEvent finished."));
            return true;
        }

    }
}

void FCloUsdShadeSubstanceMaterialTranslator::CreateAssets()
{
#if WITH_EDITOR
    ReadLiveSyncParameters();

    FString substanceFilePath;
    GetLiveSyncStringParameterValue(TEXT("substanceFilePath"), substanceFilePath);

    FString substancePresetXml;
    GetLiveSyncStringParameterValue(TEXT("substancePresetXml"), substancePresetXml);

    const bool bSubstanceUnavailable = !CloCoreUtils::IsSubstanceInstalled() ||
                                       !CloCoreUtils::IsSubstanceEnabled() ||
                                       substanceFilePath.IsEmpty() ||
                                       !FPlatformFileManager::Get().GetPlatformFile().FileExists(*substanceFilePath);

    if (bSubstanceUnavailable)
    {
        Super::CreateAssets();
        return;
    }

    FString CurrentBrowserPath;
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    if (ContentBrowserModule.Get().GetCurrentPath().HasInternalPath())
    {
        CurrentBrowserPath = ContentBrowserModule.Get().GetCurrentPath().GetInternalPathString();
    }

    const FString AssetCacheFolderPath = CloCoreUtils::GetUsdAssetCacheFolderPath();
    const FString MaterialName = PrimPath.GetName();
    bool bCreated = false;
    TWeakObjectPtr<UMaterialInstance> WeakMaterial = nullptr;
    FString MaterialHash = GetSubstanceMaterialHash(substanceFilePath, substancePresetXml);
    WeakMaterial = Context->UsdAssetCache->GetOrCreateCustomCachedAsset<UMaterialInstance>(
        MaterialHash,
        MaterialName,
        RF_Standalone | RF_Public | RF_Transactional,
        [this, substanceFilePath, substancePresetXml, AssetCacheFolderPath, MaterialName](UPackage* Outer, FName SanitizedName, EObjectFlags DesiredFlags)
        {
            TWeakObjectPtr<UMaterialInstance> material = nullptr;
            CreateSubstanceMaterial(substanceFilePath, substancePresetXml, AssetCacheFolderPath, MaterialName, material);
            return material.Get();
        },
        &bCreated
    );

    if (bCreated && WeakMaterial.IsValid())
    {
        for (const auto& TextureName : SUBSTANCE_TEXTURE_NAMES)
        {
            UTexture* SubstanceTexture = nullptr;
            FMaterialParameterInfo ParameterInfo(TextureName);
            WeakMaterial->GetTextureParameterValue(ParameterInfo, SubstanceTexture, true);
            if (SubstanceTexture != nullptr)
            {
                FString TextureHash = MaterialHash + TextureName.ToString();

                Context->UsdAssetCache->GetOrCreateCustomCachedAsset<UTexture>(
                    TextureHash,
                    TextureName.ToString(),
                    RF_Standalone | RF_Public | RF_Transactional,
                    [&SubstanceTexture](UPackage* Outer, FName SanitizedName, EObjectFlags DesiredFlags)
                    {
                        return SubstanceTexture;
                    }
                );
            }
        }
    }

    if (!WeakMaterial.IsValid())
    {
        WeakMaterial = UMaterialInstanceDynamic::Create(UMaterial::GetDefaultMaterial(MD_Surface), GetTransientPackage());
    }

    // Add UserData to pass through ensure()
    UUsdMaterialAssetUserData* UserData = NewObject<UUsdMaterialAssetUserData>(WeakMaterial.Get());
    UserData->PrimPaths = { PrimPath.GetString() };
    WeakMaterial.Get()->AddAssetUserData(UserData);

	Context->PrimLinkCache->LinkAssetToPrim(PrimPath, WeakMaterial.Get());

    // Restore to the path of the ContentBrowser
    if (!CurrentBrowserPath.IsEmpty())
    {
        ContentBrowserModule.Get().SyncBrowserToFolders({ CurrentBrowserPath }, true);
    }

#else
    Super::CreateAssets();
#endif // WITH_EDITOR
}

FString FCloUsdShadeSubstanceMaterialTranslator::GetSubstanceMaterialHash(FString InSubstanceFilePath, FString InSubstancePresetXml)
{
    FString result;
    auto prefix = UsdUtils::GetAssetHashPrefix(GetPrim(), Context->bShareAssetsForIdenticalPrims);

    FMD5 MD5;
    MD5.Update(reinterpret_cast<const uint8*>(*InSubstanceFilePath), static_cast<uint64>(InSubstanceFilePath.Len()) * sizeof(TCHAR));
    MD5.Update(reinterpret_cast<const uint8*>(&InSubstancePresetXml), static_cast<uint64>(InSubstancePresetXml.Len()) * sizeof(TCHAR));

    FMD5Hash Hash;
    Hash.Set(MD5);
    return prefix + LexToString(Hash);
}

bool FCloUsdShadeSubstanceMaterialTranslator::CreateSubstanceMaterial(const FString AbsoluteFilePath, const FString InSubstancePresetXML, const FString InParentPackageFolderPath, const FString InMaterialName, TWeakObjectPtr<UMaterialInstance>& OutMaterial)
{
    FString SubstanceFilePath;
    FString ValidFileName = FPaths::MakeValidFileName(GetPrim().GetName().ToString());
    FString GraphInstName = ValidFileName + TEXT("_Inst");
    if (!CopyFileToSubstanceTempFolder(AbsoluteFilePath, ValidFileName, SubstanceFilePath))
    {
        UE_LOG(LogCloLiveSyncCore, Error, TEXT("CreateSubstanceMaterial: Failed to copy Substance file to temp folder."));
        return false;
    }

    FString SubstanceMaterialFolderPath = FPaths::Combine(InParentPackageFolderPath, ValidFileName);
    FString SubstanceInstanceFactoryRefPath = SubstanceMaterialFolderPath / ValidFileName + TEXT(".") + ValidFileName;
    FString SubstanceGraphInstanceRefPath = SubstanceMaterialFolderPath / GraphInstName + TEXT(".") + GraphInstName;

    UObject* InstanceFactory = LoadObject<UObject>(nullptr, *SubstanceInstanceFactoryRefPath);
    if (!IsValid(InstanceFactory))
    {
        UE_LOG(LogCloLiveSyncCore, Log, TEXT("CreateSubstanceMaterial: InstanceFactory not found or failed to load. Creating new one..."));
        UObject* FactoryCDO = CLO::Private::PrepareSubstanceFactoryCDO();
        if (!FactoryCDO)
        {
            UE_LOG(LogCloLiveSyncCore, Error, TEXT("CreateSubstanceMaterial: Failed to prepare Factory CDO for creation."));
            return false;
        }
        InstanceFactory = CLO::Private::CallImportArchive(FactoryCDO, SubstanceMaterialFolderPath, SubstanceFilePath);
        if (!IsValid(InstanceFactory))
        {
            UE_LOG(LogCloLiveSyncCore, Error, TEXT("CreateSubstanceMaterial: Failed to create InstanceFactory asset."));
            return false;
        }
        UE_LOG(LogCloLiveSyncCore, Log, TEXT("CreateSubstanceMaterial: Successfully created new InstanceFactory."));
    }
    else
    {
        UE_LOG(LogCloLiveSyncCore, Log, TEXT("CreateSubstanceMaterial: Successfully loaded existing InstanceFactory."));
    }

    UObject* GraphInstance = LoadObject<UObject>(nullptr, *SubstanceGraphInstanceRefPath);
    if (!IsValid(GraphInstance))
    {
        UE_LOG(LogCloLiveSyncCore, Log, TEXT("CreateSubstanceMaterial: GraphInstance not found or failed to load. Creating new one using InstanceFactory..."));
        TArray<uint8> GraphData;
        if (!CLO::Private::GetFirstGraphDescData(InstanceFactory, GraphData))
        {
            UE_LOG(LogCloLiveSyncCore, Error, TEXT("CreateSubstanceMaterial: Failed to get GraphDescData for creating GraphInstance."));
            return false;
        }
        GraphInstance = CLO::Private::CallCreateGraphInstance(InstanceFactory, GraphData, SubstanceGraphInstanceRefPath);
        if (!IsValid(GraphInstance))
        {
            UE_LOG(LogCloLiveSyncCore, Error, TEXT("CreateSubstanceMaterial: Failed to create GraphInstance asset."));
            return false;
        }
        UE_LOG(LogCloLiveSyncCore, Log, TEXT("CreateSubstanceMaterial: Successfully created new GraphInstance."));
    }
    else
    {
        UE_LOG(LogCloLiveSyncCore, Log, TEXT("CreateSubstanceMaterial: Successfully loaded existing GraphInstance."));
    }


    if (!InSubstancePresetXML.IsEmpty())
    {
        if (!CLO::Private::CallSetSubstancePreset(GraphInstance, InSubstancePresetXML))
        {
            UE_LOG(LogCloLiveSyncCore, Warning, TEXT("CreateSubstanceMaterial: CallSetSubstancePreset failed, but continuing material creation."));
        }
    }

    UMaterial* ParentMat = Cast<UMaterial>(StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *PARENT_SUBSTANCE_MAT_PATH));
    if (!ParentMat)
    {
        UE_LOG(LogCloLiveSyncCore, Error, TEXT("CreateSubstanceMaterial: Failed to load Parent Material: %s"), *PARENT_SUBSTANCE_MAT_PATH);
        return false;
    }

    FString MaterialPackageName = SubstanceMaterialFolderPath / (ValidFileName + TEXT("_Mat"));
    if (!CLO::Private::CallGraphInstanceSetupSequence(GraphInstance, MaterialPackageName, ParentMat, false))
    {
        UE_LOG(LogCloLiveSyncCore, Error, TEXT("CreateSubstanceMaterial: Graph Instance setup sequence failed."));
        return false;
    }

    OutMaterial = FindFirstMaterialInstanceConstant(SubstanceMaterialFolderPath);
    return OutMaterial != nullptr;
}

bool FCloUsdShadeSubstanceMaterialTranslator::CopyFileToSubstanceTempFolder(const FString& SourceFilePath, const FString& NewFileName, FString& OutCopiedFilePath)
{
    FString ValidFileName = FPaths::MakeValidFileName(NewFileName);

    if (ValidFileName.IsEmpty())
    {
        ValidFileName = FPaths::GetBaseFilename(SourceFilePath);
    }
    else
    {
        ValidFileName = ValidFileName.TrimStartAndEnd();
    }

    if (FPaths::GetExtension(ValidFileName, false).IsEmpty())
    {
        ValidFileName += FPaths::GetExtension(SourceFilePath, true);
    }

    FString SubstanceTempFolderPath = CloCoreUtils::GetSubstanceTempDirectoryPath();
    IFileManager& FileManager = IFileManager::Get();

    FString DestFilePath = FPaths::Combine(SubstanceTempFolderPath, ValidFileName);
    int32 CopyResult = FileManager.Copy(*DestFilePath, *SourceFilePath, true);
    bool bSuccess = (CopyResult != -1);
    if (bSuccess)
    {
        OutCopiedFilePath = DestFilePath;
    }

    return bSuccess;
}

UMaterialInstanceConstant* FCloUsdShadeSubstanceMaterialTranslator::FindFirstMaterialInstanceConstant(const FString& InSearchFolderPath)
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FARFilter Filter;
    Filter.PackagePaths.Add(*InSearchFolderPath);
    Filter.ClassPaths.Add(UMaterialInstanceConstant::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;

    TArray<FAssetData> AssetDataList;
    AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);

    for (const FAssetData& AssetData : AssetDataList)
    {
	    if (UObject* FoundObject = AssetData.GetAsset())
        {
	        if (UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(FoundObject))
            {
                return MaterialInstance;
            }
        }
    }

    return nullptr;
}

#undef LOCTEXT_NAMESPACE

#endif // #if USE_USD_SDK
