// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "Modules/ModuleManager.h"

class CLOBLUEPRINTXMLPARSER_API FCloBlueprintXmlParserModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};