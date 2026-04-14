// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"


class CLOLIVESYNCEDITOR_API ICloLiveSyncToolbarExtensionHelper
{
public:

    virtual ~ICloLiveSyncToolbarExtensionHelper() {};

    virtual void SubscribeExtender() = 0;
    virtual void UnsubscribeExtender() = 0;

    virtual TSharedRef<FExtender> GetToolbarExtender(const TSharedRef<FUICommandList> CommandList, TSharedRef<IAnimationEditor> InAnimationEditor) = 0;

protected:
    TSharedPtr<FExtender> Extender;
};