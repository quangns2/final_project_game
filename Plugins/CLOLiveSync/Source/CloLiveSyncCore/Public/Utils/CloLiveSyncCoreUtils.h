// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

class CLOLIVESYNCCORE_API CloCoreUtils
{
public:
    static void SetDefaultTransparentBlendMode(const EBlendMode BlendMode);
    static const EBlendMode GetDefaultTransparentBlendMode();

    static void SetFlipNormalGreenChannel(bool ShouldFlip);
    static const bool ShouldFlipNormalGreenChannel();

    static void UpdateSubstanceStatus(bool Installed, bool Enabled);
    static bool IsSubstanceInstalled();
    static bool IsSubstanceEnabled();

    static void SetUsdOutputContentFolderPath(const FString& OutputPath);
    static const FString GetUsdOutputContentFolderPath();

    static void SetUsdAssetCacheFolderPath(const FString& OutputPath);
    static const FString GetUsdAssetCacheFolderPath();

    static FString GetLiveSyncTempDirectoryPath();
    static FString GetSubstanceTempDirectoryPath();

private:
    static EBlendMode DefaultTransparentBlendMode;
    static bool bFlipNormalGreenChannel;
    static bool SubstanceInstalled;
    static bool SubstanceEnabled;
    static FString UsdOutputContentFolderPath;
    static FString UsdAssetCacheFolderPath;
};
