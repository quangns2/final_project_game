// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "Engine/EngineTypes.h"
#include "HAL/PlatformMath.h"
#include "UObject/ObjectMacros.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

#include "MeshMaterialCombinerArguments.generated.h"

class UGeometryCache;
class FProperty;
class UObject;

UCLASS()
class UMeshMaterialCombinerArguments : public UObject
{
    GENERATED_UCLASS_BODY()

    /** The base directories to be considered Internal Only for the struct picker.*/
    UPROPERTY(EditAnywhere, Category = BaseMesh, meta = (Tooltip = "This is a Mesh that contains a material referenced by other Mesh assets.", AllowedClasses = "/Script/Engine.StaticMesh, /Script/Engine.SkeletalMesh, /Script/GeometryCache.GeometryCache"))
    TObjectPtr<UObject> BaseMesh;

    UPROPERTY(EditAnywhere, Category = TargetMeshes, meta = (Tooltip = "These are the Meshes in which to change the material references.", AllowedClasses = "/Script/Engine.StaticMesh, /Script/Engine.SkeletalMesh, /Script/GeometryCache.GeometryCache"))
    TArray<TObjectPtr<UObject>> TargetMeshes;

    UPROPERTY(EditAnywhere, Category = TargetMeshes, meta = (Tooltip = "This option determines whether to remove the previously used materials and textures when changing material references."))
    bool bDeleteAllOtherMaterials;
};
