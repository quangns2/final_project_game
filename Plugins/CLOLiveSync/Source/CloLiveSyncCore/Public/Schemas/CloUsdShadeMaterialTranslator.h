// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "USDShadeMaterialTranslator.h"

#if USE_USD_SDK
#include "Materials/MaterialInstance.h"
#include "UsdWrappers/UsdAttribute.h"

class CLOLIVESYNCCORE_API FCloUsdShadeMaterialTranslator : public FUsdShadeMaterialTranslator
{
	using Super = FUsdShadeMaterialTranslator;

public:
	static FName CLOUsdRenderContext;

public:
	using FUsdShadeMaterialTranslator::FUsdShadeMaterialTranslator;

	virtual void CreateAssets() override;

protected:
    virtual void PostImportMaterial(const FString& PrefixedMaterialHash, UMaterialInterface* ImportedMaterial) override;
    USubsurfaceProfile* CreateSubsurfaceProfile(UMaterialInstance* InMaterialInstance);

    bool FindUsdAttribute(FString InKey, UE::FUsdAttribute& OutAttribute);

    UE::FUsdPrim GetLiveSyncPrim() const;
    void ReadLiveSyncParameters();

    bool GetLiveSyncBoolParameterValue(const FString& InParameterName, bool& OutValue);
    bool GetLiveSyncFloatParameterValue(const FString& InParameterName, float& OutValue);
    bool GetLiveSyncStringParameterValue(const FString& InParameterName, FString& OutValue);

    bool ConvertToBool(const UE::FUsdAttribute& InAttr, bool& OutValue);
    bool ConvertToFloat(const UE::FUsdAttribute& InAttr, float& OutValue);
    bool ConvertToString(const UE::FUsdAttribute& InAttr, FString& OutValue);
    bool ConvertToArrFloat(const UE::FUsdAttribute& InAttr, TArray<float>& OutValue);

    EBlendMode GetAppropriateTransparentBlendMode();
    void SetOpacityBlendMode(UMaterialInstance* InMaterialInstance);

    void PostSetBoolParameter(UMaterialInstance* InMaterialInstance, const FString& InParameterName, const bool& InValue);
    void PostSetScalarParameter(UMaterialInstance* InMaterialInstance, const FString& InParameterName, const float& InValue);
    
    UTexture2D* ImportTextureFromPath(const FString& TexturePath, const FString& MaterialHash);
    void ProcessSeamPuckeringNormalTexture(UMaterialInstance* InMaterialInstance, const FString& MaterialHash);
    void ProcessFlipGreenChannelForNormalTextures(UMaterialInstance* InMaterialInstance);

protected:
    TMap<FString, UE::FUsdAttribute> MapLiveSyncParams;
};

#endif // #if USE_USD_SDK
