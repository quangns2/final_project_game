// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "USDSkelSkeletonTranslator.h"

#if USE_USD_SDK

class CLOLIVESYNCCORE_API FCloUsdSkelSkeletonTranslator : public FUsdSkelSkeletonTranslator
{
public:
	using FUsdSkelSkeletonTranslator::FUsdSkelSkeletonTranslator;

	virtual USceneComponent* CreateComponents() override;
};

#endif