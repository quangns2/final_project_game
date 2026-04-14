// Copyright 2024 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "USDGroomTranslator.h"

#if USE_USD_SDK
#include "UsdWrappers/UsdAttribute.h"

#include "USDIncludesStart.h"
	#include "pxr/pxr.h"
#include "USDIncludesEnd.h"

class CLOLIVESYNCCORE_API FCloUsdGroomTranslator : public FUsdGroomTranslator
{
	using Super = FUsdGroomTranslator;

public:
	static FName CLOUsdRenderContext;

public:
	using FUsdGroomTranslator::FUsdGroomTranslator;

	virtual void CreateAssets() override;

	virtual USceneComponent* CreateComponents() override;
	virtual void UpdateComponents(USceneComponent* SceneComponent) override;
};

#endif // #if USE_USD_SDK
