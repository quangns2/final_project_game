// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "Utils/CloLiveSyncCoreUtils.h"

#include "Misc/Paths.h"
#include "HAL/FileManager.h"

EBlendMode CloCoreUtils::DefaultTransparentBlendMode = EBlendMode::BLEND_Translucent;
bool CloCoreUtils::bFlipNormalGreenChannel = true;
bool CloCoreUtils::SubstanceInstalled = false;
bool CloCoreUtils::SubstanceEnabled = false;
FString CloCoreUtils::UsdOutputContentFolderPath = TEXT("/Game/UsdAssets");
FString CloCoreUtils::UsdAssetCacheFolderPath = TEXT("/Game/UsdAssets");

void CloCoreUtils::SetDefaultTransparentBlendMode(const EBlendMode BlendMode)
{
    DefaultTransparentBlendMode = BlendMode;
}

const EBlendMode CloCoreUtils::GetDefaultTransparentBlendMode()
{
    return DefaultTransparentBlendMode;
}

void CloCoreUtils::SetFlipNormalGreenChannel(bool ShouldFlip)
{
    bFlipNormalGreenChannel = ShouldFlip;
}

const bool CloCoreUtils::ShouldFlipNormalGreenChannel()
{
    return bFlipNormalGreenChannel;
}

void CloCoreUtils::UpdateSubstanceStatus(bool Installed, bool Enabled)
{
    SubstanceInstalled = Installed;
    SubstanceEnabled = Enabled;
}

bool CloCoreUtils::IsSubstanceInstalled()
{
    return SubstanceInstalled;
}

bool CloCoreUtils::IsSubstanceEnabled()
{
    return SubstanceEnabled;
}

void CloCoreUtils::SetUsdOutputContentFolderPath(const FString& OutputPath)
{
    UsdOutputContentFolderPath = OutputPath;
}

const FString CloCoreUtils::GetUsdOutputContentFolderPath()
{
    return UsdOutputContentFolderPath;
}

void CloCoreUtils::SetUsdAssetCacheFolderPath(const FString& OutputPath)
{
    UsdAssetCacheFolderPath = OutputPath;
}

const FString CloCoreUtils::GetUsdAssetCacheFolderPath()
{
    return UsdAssetCacheFolderPath;
}

FString CloCoreUtils::GetLiveSyncTempDirectoryPath()
{
    const FString ProjectSavedDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::ProjectSavedDir());
    return FPaths::Combine(ProjectSavedDir, TEXT("LiveSync"));
}

FString CloCoreUtils::GetSubstanceTempDirectoryPath()
{
    return FPaths::Combine(GetLiveSyncTempDirectoryPath(), TEXT("Substance"));
}
