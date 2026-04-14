// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "CloLiveSyncExportOptions.h"


class CloLiveSyncExportUIDetails : public IDetailCustomization
{
public:
    virtual void CustomizeDetails(IDetailLayoutBuilder& detailBuilder) override;

protected:
    UCloLiveSyncExportOptionBase* target;
};
