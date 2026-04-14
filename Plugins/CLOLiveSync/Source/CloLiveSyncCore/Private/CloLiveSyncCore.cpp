// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "CloLiveSyncCore.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

#include "Utils/CloLiveSyncCoreUtils.h"

#define LOCTEXT_NAMESPACE "FCLOLiveSyncCoreModule"

DEFINE_LOG_CATEGORY(LogCloLiveSyncCore);

void FCloLiveSyncCoreModule::StartupModule()
{
    CreateLiveSyncTempDirectory();
}

void FCloLiveSyncCoreModule::ShutdownModule()
{
    DeleteLiveSyncTempDirectory();
}

bool FCloLiveSyncCoreModule::CreateLiveSyncTempDirectory()
{
    FString SubstanceTempDirectory = CloCoreUtils::GetSubstanceTempDirectoryPath();

    if (SubstanceTempDirectory.IsEmpty())
    {
        return false;
    }

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.CreateDirectoryTree(*SubstanceTempDirectory))
    {
        return false;
    }

    return true;
}

bool FCloLiveSyncCoreModule::DeleteLiveSyncTempDirectory()
{
    FString TempDirectory = CloCoreUtils::GetLiveSyncTempDirectoryPath();

    if (TempDirectory.IsEmpty())
    {
        return false;
    }

    IFileManager& FileManager = IFileManager::Get();
    if (!FileManager.DirectoryExists(*TempDirectory))
    {
        return false;
    }

    return FileManager.DeleteDirectory(*TempDirectory, false, true);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCloLiveSyncCoreModule, CloLiveSyncCore)
