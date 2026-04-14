// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "Schemas/CloUsdShadeMaterialTranslator.h"

#if USE_USD_SDK
#if WITH_EDITOR
#include "EditorAssetLibrary.h"
#endif
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "UObject/Package.h"
#include "Engine/SubsurfaceProfile.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Runtime/RHI/Public/RHIShaderPlatform.h"
#include "UObject/StrongObjectPtr.h"
#include "USDObjectUtils.h"
#include "USDProjectSettings.h"
#include "USDShadeConversion.h"
#include "USDValueConversion.h"
#include "UsdWrappers/SdfPath.h"
#include "UsdWrappers/UsdAttribute.h"
#include "UsdWrappers/UsdPrim.h"
#include "UsdWrappers/VtValue.h"

#include "Utils/CloLiveSyncCoreUtils.h"


static bool GUseInterchangeMaterialCLOTranslator = true;
static FAutoConsoleVariableRef CvarUseInterchangeMaterialCLOTranslator(
	TEXT("USD.UseInterchangeCLOMaterialTranslator"),
    GUseInterchangeMaterialCLOTranslator,
	TEXT(""));

FName FCloUsdShadeMaterialTranslator::CLOUsdRenderContext = TEXT("clo");

void OverrideUsdImportMaterials(const TArray<FSoftObjectPath>& Materials, TArray<FSoftObjectPath>* SavedValues = nullptr)
{
    if (UUsdProjectSettings* UsdProjectSettings = GetMutableDefault<UUsdProjectSettings>())
    {
        // Check to see if we should save the existing values
        if (SavedValues)
        {
            SavedValues->Push(UsdProjectSettings->ReferencePreviewSurfaceMaterial);
            SavedValues->Push(UsdProjectSettings->ReferencePreviewSurfaceTranslucentMaterial);
            SavedValues->Push(UsdProjectSettings->ReferencePreviewSurfaceTwoSidedMaterial);
            SavedValues->Push(UsdProjectSettings->ReferencePreviewSurfaceTranslucentTwoSidedMaterial);
        }
        UsdProjectSettings->ReferencePreviewSurfaceMaterial = Materials[0];
        UsdProjectSettings->ReferencePreviewSurfaceTranslucentMaterial = Materials[1];
        UsdProjectSettings->ReferencePreviewSurfaceTwoSidedMaterial = Materials[2];
        UsdProjectSettings->ReferencePreviewSurfaceTranslucentTwoSidedMaterial = Materials[3];
    }
}

void FCloUsdShadeMaterialTranslator::CreateAssets()
{
#if WITH_EDITOR
    ReadLiveSyncParameters();

    FString MaterialType = "Matte";
    GetLiveSyncStringParameterValue(TEXT("materialType"), MaterialType);
    FString AssetName = TEXT("M_CLO_") + MaterialType;

    FString MaterialAssetPath = TEXT("/CLOLiveSync/Material/CLO");
    FString MaterialPakageName = FString::Format(TEXT("{0}/{1}"), { MaterialAssetPath, AssetName });
    if (UEditorAssetLibrary::DoesAssetExist(MaterialPakageName) == false)
    {
        AssetName = TEXT("M_CLO_Default");
        MaterialPakageName = FString::Format(TEXT("{0}/{1}"), { MaterialAssetPath, AssetName });
    }

    TArray<FSoftObjectPath> UsdMaterials;
    for (int i = 0; i < 4; ++i)
    {
        UsdMaterials.Add(FSoftObjectPath(FTopLevelAssetPath(*MaterialPakageName, *AssetName)));
    }

    TArray<FSoftObjectPath> OriginalUsdMaterials;

    OverrideUsdImportMaterials(UsdMaterials, &OriginalUsdMaterials);

    Super::CreateAssets();

    // Restore Original USD Materials
    OverrideUsdImportMaterials(OriginalUsdMaterials);

    return;
    
#else
	Super::CreateAssets();
#endif // WITH_EDITOR
}

USubsurfaceProfile* FCloUsdShadeMaterialTranslator::CreateSubsurfaceProfile(UMaterialInstance* InMaterialInstance)
{
    USubsurfaceProfile* SubsurfaceProfile = nullptr;

#if WITH_EDITOR
    // import SubsurfaceProfile.
    FString SSPName = "SSP_" + InMaterialInstance->GetName();
    
    FString PackageBasePath = FPackageName::GetLongPackagePath(InMaterialInstance->GetOutermost()->GetName());
    const FString OutputContentFolderPath = CloCoreUtils::GetUsdOutputContentFolderPath();
    if (!OutputContentFolderPath.Equals(CloCoreUtils::GetUsdAssetCacheFolderPath()))
    {
        const FString RootPath = Context->Stage.GetRootLayer().GetRealPath();
        FString FileName = UsdUnreal::ObjectUtils::SanitizeObjectName(FPaths::GetBaseFilename(RootPath));
        FileName.ReplaceInline(TEXT(" "), TEXT("_"));

        PackageBasePath = OutputContentFolderPath + TEXT("/") + FileName + TEXT("/Materials");
    }

    FString PackagePath = PackageBasePath + TEXT("/") + SSPName;
    UPackage* NewPackage = CreatePackage(*PackagePath);
    NewPackage->MarkPackageDirty();

    SubsurfaceProfile = NewObject<USubsurfaceProfile>(NewPackage, *SSPName, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
    SubsurfaceProfile->Settings.bEnableBurley = true;
    SubsurfaceProfile->Settings.bEnableMeanFreePath = true;
    SubsurfaceProfile->Settings.MeanFreePathDistance = 0.8f; // MagicNumber. You can change this at any time via SubsurfaceProfile Asset.

    UE::FUsdAttribute UsdAttribute;
    if (FindUsdAttribute("SSSMix", UsdAttribute))
    {
        float Mix = 0.0f;
        if (ConvertToFloat(UsdAttribute, Mix))
            SubsurfaceProfile->Settings.LobeMix = Mix;
    }
    if (FindUsdAttribute("SSSDensityScale", UsdAttribute))
    {
        float DensityScale = 0.0f;
        if (ConvertToFloat(UsdAttribute, DensityScale))
            SubsurfaceProfile->Settings.ExtinctionScale = DensityScale;
    }
    if (FindUsdAttribute("SSSRadius1", UsdAttribute))
    {
        float Radius1 = 0.0f;
        if (ConvertToFloat(UsdAttribute, Radius1))
            SubsurfaceProfile->Settings.ScatterRadius = Radius1;
    }
    if (FindUsdAttribute("SSSColor1", UsdAttribute))
    {
        TArray<float> Color1;
        if (ConvertToArrFloat(UsdAttribute, Color1))
        {
            SubsurfaceProfile->Settings.SurfaceAlbedo = FLinearColor(Color1[0], Color1[1], Color1[2]);
        }
    }
    if (FindUsdAttribute("SSSColor2", UsdAttribute))
    {
        TArray<float> Color2;
        if (ConvertToArrFloat(UsdAttribute, Color2))
        {
            SubsurfaceProfile->Settings.MeanFreePathColor = FLinearColor(Color2[0], Color2[1], Color2[2]);
        }
    }
    if (FindUsdAttribute("SSSColor3", UsdAttribute))
    {
        TArray<float> Color3;
        if (ConvertToArrFloat(UsdAttribute, Color3))
        {
            SubsurfaceProfile->Settings.Tint = FLinearColor(Color3[0], Color3[1], Color3[2]);
        }
    }

    InMaterialInstance->bOverrideSubsurfaceProfile = true;
    InMaterialInstance->SubsurfaceProfile = SubsurfaceProfile;
    
#endif

    return SubsurfaceProfile;
}

void FCloUsdShadeMaterialTranslator::PostImportMaterial(const FString& PrefixedMaterialHash, UMaterialInterface* ImportedMaterial)
{
    // Perform default material translate fisrt, then apply CLO specific parameters.
    Super::PostImportMaterial(PrefixedMaterialHash, ImportedMaterial);
    
    UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(ImportedMaterial);

    if(MaterialInstance != nullptr)
    {
        for(const auto& [parameterName, attr] : MapLiveSyncParams)
        {
            if(attr.GetTypeName() == TEXT("bool"))
            {
                bool value;
                if (ConvertToBool(attr, value))
                {
                    UsdUtils::SetBoolParameterValue(*MaterialInstance, *parameterName, value);
                    PostSetBoolParameter(MaterialInstance, parameterName, value);
                }
            }
            else if(attr.GetTypeName() == TEXT("float"))
            {
                float value;
                if (ConvertToFloat(attr, value))
                {
                    UsdUtils::SetScalarParameterValue(*MaterialInstance, *parameterName, value);
                    PostSetScalarParameter(MaterialInstance, parameterName, value);
                }
            }
            else if (attr.GetTypeName() == TEXT("color3f"))
            {
                TArray<float> value;
                if (ConvertToArrFloat(attr, value))
                {
                    UsdUtils::SetVectorParameterValue(*MaterialInstance, *parameterName, FLinearColor(value[0], value[1], value[2]));
                }
            }
            else if (attr.GetTypeName() == TEXT("color4f"))
            {
                TArray<float> value;
                if (ConvertToArrFloat(attr, value))
                {
                    UsdUtils::SetVectorParameterValue(*MaterialInstance, *parameterName, FLinearColor(value[0], value[1], value[2], value[3]));
                }
            }
        }

#if WITH_EDITOR
        // Process seamPuckeringNormalFilePath texture
        ProcessSeamPuckeringNormalTexture(MaterialInstance, PrefixedMaterialHash);
        ProcessFlipGreenChannelForNormalTextures(MaterialInstance);

        // create ssp.
        FMaterialShadingModelField ShadingModel = MaterialInstance->GetShadingModels();
        if (ShadingModel.HasShadingModel(MSM_SubsurfaceProfile))
        {
            CreateSubsurfaceProfile(MaterialInstance);
        }

        // Update render resources to reflect changed material properties.
        FMaterialUpdateContext UpdateContext(FMaterialUpdateContext::EOptions::Default, GMaxRHIShaderPlatform);
        UpdateContext.AddMaterialInstance(MaterialInstance);
        MaterialInstance->MarkPackageDirty();
#endif
    }
}

bool FCloUsdShadeMaterialTranslator::FindUsdAttribute(FString InKey, UE::FUsdAttribute& OutAttribute)
{
    if (MapLiveSyncParams.Contains(InKey))
    {
        OutAttribute = MapLiveSyncParams[InKey];
        return true;
    }

    return false;
}

UE::FUsdPrim FCloUsdShadeMaterialTranslator::GetLiveSyncPrim() const
{
    return Context->Stage.GetPrimAtPath(PrimPath.AppendPath(UE::FSdfPath(TEXT("cloLiveSync"))));
}

void FCloUsdShadeMaterialTranslator::ReadLiveSyncParameters()
{
    TArray<uint32> FabricIds;
    if (const UE::FUsdPrim LiveSyncPrim = GetLiveSyncPrim())
    {
        auto Attrs = LiveSyncPrim.GetAttributes();
        for(const UE::FUsdAttribute& attr : Attrs)
        {
            if(attr.HasValue())
            {
                MapLiveSyncParams.Add(attr.GetName().ToString(), attr);
            }
        }
    }
}

bool FCloUsdShadeMaterialTranslator::GetLiveSyncBoolParameterValue(const FString& InParameterName, bool& OutValue)
{
    if (const auto* attr = MapLiveSyncParams.Find(InParameterName); attr != nullptr)
    {
        return ConvertToBool(*attr, OutValue);
    }

    return false;
}

bool FCloUsdShadeMaterialTranslator::GetLiveSyncFloatParameterValue(const FString& InParameterName, float& OutValue)
{
    if (const auto* attr = MapLiveSyncParams.Find(InParameterName); attr != nullptr)
    {
        return ConvertToFloat(*attr, OutValue);
    }

    return false;
}

bool FCloUsdShadeMaterialTranslator::GetLiveSyncStringParameterValue(const FString& InParameterName, FString& OutValue)
{
    if(const auto* attr = MapLiveSyncParams.Find(InParameterName); attr!= nullptr)
    {
        return ConvertToString(*attr, OutValue);
    }

    return false;
}

bool FCloUsdShadeMaterialTranslator::ConvertToBool(const UE::FUsdAttribute& InAttr, bool& OutValue)
{
    if (InAttr.GetTypeName() == TEXT("bool"))
    {
        UE::FVtValue vtValue;
        InAttr.Get(vtValue);
        UsdUtils::FConvertedVtValue convertedVtValue;
        if (UsdToUnreal::ConvertValue(vtValue, convertedVtValue))
        {
            if (convertedVtValue.Entries.Num() == 1 && convertedVtValue.Entries[0].Num() == 1)
            {
                OutValue = convertedVtValue.Entries[0][0].Get<bool>();
                return true;
            }
        }
    }
    
    return false;
}

bool FCloUsdShadeMaterialTranslator::ConvertToFloat(const UE::FUsdAttribute& InAttr, float& OutValue)
{
    if (InAttr.GetTypeName() == TEXT("float"))
    {
        UE::FVtValue vtValue;
        InAttr.Get(vtValue);
        UsdUtils::FConvertedVtValue convertedVtValue;
        if (UsdToUnreal::ConvertValue(vtValue, convertedVtValue))
        {
            if (convertedVtValue.Entries.Num() == 1 && convertedVtValue.Entries[0].Num() == 1)
            {
                OutValue = convertedVtValue.Entries[0][0].Get<float>();
                return true;
            }
        }
    }

    return false;
}

bool FCloUsdShadeMaterialTranslator::ConvertToString(const UE::FUsdAttribute& InAttr, FString& OutValue)
{
    if (InAttr.GetTypeName() == TEXT("string"))
    {
        UE::FVtValue vtValue;
        InAttr.Get(vtValue);
        UsdUtils::FConvertedVtValue convertedVtValue;
        if (UsdToUnreal::ConvertValue(vtValue, convertedVtValue))
        {
            FString* value = convertedVtValue.Entries[0][0].TryGet<FString>();
            OutValue = *value;
            return true;
        }
    }

    return false;

}

bool FCloUsdShadeMaterialTranslator::ConvertToArrFloat(const UE::FUsdAttribute& InAttr, TArray<float>& OutValue)
{
    if (InAttr.GetTypeName() == TEXT("color3f") 
        || InAttr.GetTypeName() == TEXT("color4f")
        || InAttr.GetTypeName() == TEXT("float2")
        || InAttr.GetTypeName() == TEXT("float3")
        || InAttr.GetTypeName() == TEXT("float4"))
    {
        UE::FVtValue vtValue;
        InAttr.Get(vtValue);
        UsdUtils::FConvertedVtValue convertedVtValue;
        if (UsdToUnreal::ConvertValue(vtValue, convertedVtValue))
        {
            OutValue.Empty();

            for (int i = 0; i < convertedVtValue.Entries[0].Num(); ++i)
            {
                OutValue.Add(convertedVtValue.Entries[0][i].Get<float>());
            }

            return true;   
        }
    }

    return false;
}

EBlendMode FCloUsdShadeMaterialTranslator::GetAppropriateTransparentBlendMode()
{
    EBlendMode blendMode = CloCoreUtils::GetDefaultTransparentBlendMode();

    bool bRequireTranslucentBlending = false;
    GetLiveSyncBoolParameterValue(TEXT("Opacity_RequireTranslucentBlending"), bRequireTranslucentBlending);
    if (bRequireTranslucentBlending)
    {
        blendMode = EBlendMode::BLEND_Translucent;
    }

    return blendMode;
}

void FCloUsdShadeMaterialTranslator::SetOpacityBlendMode(UMaterialInstance* InMaterialInstance)
{
    if (InMaterialInstance == nullptr)
        return;

    auto BaseMaterial = InMaterialInstance->GetBaseMaterial();
    // For the Hair material, do not override the blend mode.
    if (BaseMaterial == nullptr || BaseMaterial->bUsedWithHairStrands)
        return;

    const auto blendMode = GetAppropriateTransparentBlendMode();

    InMaterialInstance->BasePropertyOverrides.bOverride_BlendMode = true;
    InMaterialInstance->BasePropertyOverrides.BlendMode = blendMode;
    if(blendMode == BLEND_Translucent)
    {
        InMaterialInstance->BasePropertyOverrides.bOverride_CastDynamicShadowAsMasked = true;
        InMaterialInstance->BasePropertyOverrides.bCastDynamicShadowAsMasked = true;
    }

    InMaterialInstance->UpdateOverridableBaseProperties();
}

void FCloUsdShadeMaterialTranslator::PostSetBoolParameter(UMaterialInstance* InMaterialInstance, const FString& InParameterName, const bool& InValue)
{
    if (InMaterialInstance == nullptr)
    {
        return;
    }

    if (InParameterName == TEXT("Opacity_IsTransparent"))
    {
        if (InValue)
        {
            SetOpacityBlendMode(InMaterialInstance);
        }
    }
}

void FCloUsdShadeMaterialTranslator::PostSetScalarParameter(UMaterialInstance* InMaterialInstance, const FString& InParameterName, const float& InValue)
{
    if (InMaterialInstance == nullptr)
    {
        return;
    }
}

UTexture2D* FCloUsdShadeMaterialTranslator::ImportTextureFromPath(const FString& TexturePath, const FString& MaterialHash)
{
#if WITH_EDITOR
    if (TexturePath.IsEmpty() || !FPaths::FileExists(TexturePath))
    {
        return nullptr;
    }

    FString TextureName = FPaths::GetBaseFilename(TexturePath);
    FString TextureHash = MaterialHash + TEXT("_SPNormal_") + TextureName;
    
    // Use Context->UsdAssetCache like other translators
    UTexture2D* CachedTexture = Context->UsdAssetCache->GetOrCreateCustomCachedAsset<UTexture2D>(
        TextureHash,
        TextureName,
        RF_Standalone | RF_Public | RF_Transactional,
        [this, TexturePath, TextureName](UPackage* Outer, FName SanitizedName, EObjectFlags DesiredFlags)
        {
            // Determine the correct package path for the texture
            FString PackageBasePath = FPackageName::GetLongPackagePath(Outer->GetName());
            const FString OutputContentFolderPath = CloCoreUtils::GetUsdOutputContentFolderPath();
            if (!OutputContentFolderPath.Equals(CloCoreUtils::GetUsdAssetCacheFolderPath()))
            {
                const FString RootPath = Context->Stage.GetRootLayer().GetRealPath();
                FString FileName = UsdUnreal::ObjectUtils::SanitizeObjectName(FPaths::GetBaseFilename(RootPath));
                FileName.ReplaceInline(TEXT(" "), TEXT("_"));
                PackageBasePath = OutputContentFolderPath + TEXT("/") + FileName + TEXT("/Textures");
            }

            FString TexturePackagePath = PackageBasePath + TEXT("/") + SanitizedName.ToString();
            UPackage* TexturePackage = CreatePackage(*TexturePackagePath);
            TexturePackage->MarkPackageDirty();

            UTexture2D* NewTexture = NewObject<UTexture2D>(TexturePackage, SanitizedName, DesiredFlags);
            if (!NewTexture)
            {
                return static_cast<UTexture2D*>(nullptr);
            }

            TArray<uint8> RawFileData;
            if (!FFileHelper::LoadFileToArray(RawFileData, *TexturePath))
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to load texture file: %s"), *TexturePath);
                return static_cast<UTexture2D*>(nullptr);
            }

            IImageWrapperModule& ImageWrapperModule = FModuleManager::Get().LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
            EImageFormat ImageFormat = ImageWrapperModule.DetectImageFormat(RawFileData.GetData(), RawFileData.Num());
            
            if (ImageFormat == EImageFormat::Invalid)
            {
                UE_LOG(LogTemp, Warning, TEXT("Invalid image format for texture: %s"), *TexturePath);
                return static_cast<UTexture2D*>(nullptr);
            }

            TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(ImageFormat);
            if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to create image wrapper for texture: %s"), *TexturePath);
                return static_cast<UTexture2D*>(nullptr);
            }

            TArray<uint8> UncompressedRGBA;
            if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedRGBA))
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to get raw image data for texture: %s"), *TexturePath);
                return static_cast<UTexture2D*>(nullptr);
            }

            const int32 Width = ImageWrapper->GetWidth();
            const int32 Height = ImageWrapper->GetHeight();
            
            NewTexture->Source.Init(Width, Height, 1, 1, TSF_BGRA8, UncompressedRGBA.GetData());
            NewTexture->LODGroup = TextureGroup::TEXTUREGROUP_WorldNormalMap;
            NewTexture->CompressionSettings = TextureCompressionSettings::TC_Normalmap;
            NewTexture->SRGB = false;
            NewTexture->UpdateResource();
            NewTexture->MarkPackageDirty();

            return NewTexture;
        }
    );

    return CachedTexture;
#endif
    return nullptr;
}

void FCloUsdShadeMaterialTranslator::ProcessSeamPuckeringNormalTexture(UMaterialInstance* InMaterialInstance, const FString& MaterialHash)
{
#if WITH_EDITOR
    if (InMaterialInstance == nullptr)
    {
        return;
    }

    bool bHasSPNormalMap = false;
    if (!GetLiveSyncBoolParameterValue(TEXT("SPNormal_HasMap"), bHasSPNormalMap) || !bHasSPNormalMap)
    {
        return;
    }

    FString SeamPuckeringNormalPath;
    if (!GetLiveSyncStringParameterValue(TEXT("seamPuckeringNormalFilePath"), SeamPuckeringNormalPath))
    {
        return;
    }

    UTexture2D* SPNormalTexture = ImportTextureFromPath(SeamPuckeringNormalPath, MaterialHash);
    if (SPNormalTexture)
    {
        UsdUtils::SetTextureParameterValue(*InMaterialInstance, TEXT("SPNormalTexture"), SPNormalTexture);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to import SPNormalTexture from: %s"), *SeamPuckeringNormalPath);
    }
#endif
}

void FCloUsdShadeMaterialTranslator::ProcessFlipGreenChannelForNormalTextures(UMaterialInstance* InMaterialInstance)
{
    if (!InMaterialInstance)
    {
        return;
    }

    if (!CloCoreUtils::ShouldFlipNormalGreenChannel())
    {
        return;
    }

    for (FTextureParameterValue& TextureParam : InMaterialInstance->TextureParameterValues)
    {
        if (TextureParam.IsOverride() && (TextureParam.ParameterValue != nullptr) && (TextureParam.ParameterValue->LODGroup == TextureGroup::TEXTUREGROUP_WorldNormalMap))
        {
            if (!TextureParam.ParameterValue->bFlipGreenChannel)
            {
                TextureParam.ParameterValue->bFlipGreenChannel = true;
                TextureParam.ParameterValue->UpdateResource();
                TextureParam.ParameterValue->MarkPackageDirty();
                UE_LOG(LogTemp, Display, TEXT("Set Flip Green Channel to true for texture '%s': %s"), *TextureParam.ParameterInfo.Name.ToString(), *TextureParam.ParameterValue->GetName());
            }
        }
    }
}

#undef LOCTEXT_NAMESPACE

#endif // #if USE_USD_SDK
