// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "MeshMaterialCombinerArguments.h"

#include "Misc/Paths.h"
#include "Templates/Casts.h"
#include "UObject/Class.h"
#include "UObject/PropertyPortFlags.h"
#include "UObject/UnrealType.h"

UMeshMaterialCombinerArguments::UMeshMaterialCombinerArguments(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), bDeleteAllOtherMaterials(false)
{

}