// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UAnimSequence;
class UGeometryCache;
class USkeletalMesh;

class CLOLIVESYNCEDITOR_API CloEditorUtils
{
public:
    static UGeometryCache* ConvertAnimationAssetToGeometryCache(UAnimationAsset* InAnimationAsset, TArray<USkeletalMeshComponent*> InSkeletalMeshes);
    static USkeletalMesh* ConvertSkeletalMeshesToSingleSkeletalMesh(TArray<USkeletalMeshComponent*> SkeletalMeshComponents);
    static void TransferRenderDataToImportedModel(USkeletalMesh* InSkeletalMesh);
    static void GetPoseAtTime(UAnimationAsset* InAnimAsset, double Time, FAnimationPoseData& OutPoseData);
    static UAnimSequence* CreateBakedAnimSequenceWithRootMotion(UAnimSequence* OriginalAnimSequence, USkeletalMesh* TargetSkeletalMesh);
};

