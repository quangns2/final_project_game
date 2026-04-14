// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CloUsdShadeMaterialTranslator.h"

#include "Materials/MaterialInstance.h"

#if USE_USD_SDK

class CLOLIVESYNCCORE_API FCloUsdShadeSubstanceMaterialTranslator : public FCloUsdShadeMaterialTranslator
{
	using Super = FCloUsdShadeMaterialTranslator;

public:
    static FName CLOUsdSubstanceRenderContext;

public:
	using FCloUsdShadeMaterialTranslator::FCloUsdShadeMaterialTranslator;

	virtual void CreateAssets() override;

private:
	FString GetSubstanceMaterialHash(FString InSubstanceFilePath, FString InSubstancePresetXml);
	bool CreateSubstanceMaterial(const FString AbsoluteFilePath, const FString InSubstancePresetXML, const FString InParentPackageFolderPath, const FString InMaterialName, TWeakObjectPtr<UMaterialInstance>& OutMaterial);
	bool CopyFileToSubstanceTempFolder(const FString& SourceFilePath, const FString& NewFileName, FString& OutCopiedFilePath);
	UMaterialInstanceConstant* FindFirstMaterialInstanceConstant(const FString& InSearchFolderPath);
};

#endif // #if USE_USD_SDK
