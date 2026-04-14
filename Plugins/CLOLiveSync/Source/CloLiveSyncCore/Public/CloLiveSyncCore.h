// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCloLiveSyncCore, Log, All);

class CLOLIVESYNCCORE_API FCloLiveSyncCoreModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    bool CreateLiveSyncTempDirectory();
    bool DeleteLiveSyncTempDirectory();

};
