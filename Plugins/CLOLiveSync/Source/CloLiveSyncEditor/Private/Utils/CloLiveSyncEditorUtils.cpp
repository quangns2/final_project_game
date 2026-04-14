// Copyright 2023-2025 CLO Virtual Fashion. All rights reserved.

#include "Utils/CloLiveSyncEditorUtils.h"

#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimTypes.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Animation/PoseAsset.h"
#include "Animation/Skeleton.h"
#include "BoneContainer.h"
#include "Containers/UnrealString.h"
#include "Editor.h"
#include "Engine/SkeletalMeshLODSettings.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Factories/AnimSequenceFactory.h"
#include "GeometryCache.h"
#include "GeometryCacheCodecV1.h"
#include "GeometryCacheMeshData.h"
#include "GeometryCacheTrackStreamable.h"
#include "MaterialDomain.h"
#include "ReferenceSkeleton.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalMergingLibrary.h"

int32 GetVertices(const USkeletalMesh* SkeletalMesh, const int32 LODIndex,
                  TArray<FVector3f>& OutPositions)
{
    check(SkeletalMesh);
    OutPositions.Reset();

    const FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetResourceForRendering();
    check(RenderData);

    if (!RenderData->LODRenderData.IsValidIndex(LODIndex))
    {
        return INDEX_NONE;
    }

    // Get LOD Data
    const FSkeletalMeshLODRenderData& LODRenderData = RenderData->LODRenderData[LODIndex];

    // Get Total Num of Vertices (for all sections)
    const int32 NumVertices = LODRenderData.GetNumVertices();
    OutPositions.SetNumUninitialized(NumVertices);

    for (int32 VertexIndex = 0; VertexIndex < NumVertices; ++VertexIndex)
    {
        OutPositions[VertexIndex] = LODRenderData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);
    };

    return NumVertices;
}

template <uint16 NumInfluences>
struct TVertexSkinWeight
{
    TStaticArray<uint16, NumInfluences> MeshBoneIndices;
    TStaticArray<uint8, NumInfluences>  BoneWeights;
};

using VertexSkinWeightMax = TVertexSkinWeight<MAX_TOTAL_INFLUENCES>;
using VertexSkinWeightFour = TVertexSkinWeight<4>;

void GetSkinWeights(const USkeletalMesh* SkeletalMesh, const int32 LODIndex,
    TArray<VertexSkinWeightMax>& OutSkinWeights)
{
    check(SkeletalMesh);
    OutSkinWeights.Reset();

    // Get Render Data
    const FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetResourceForRendering();
    check(RenderData);

    // Get LOD Data
    const FSkeletalMeshLODRenderData& LODRenderData = RenderData->LODRenderData[LODIndex];

    // Get Weights
    const FSkinWeightVertexBuffer* SkinWeightVertexBuffer = LODRenderData.GetSkinWeightVertexBuffer();
    check(SkinWeightVertexBuffer);

    // Get Weights from Buffer.
    // this is size of number of vertices.
    TArray<FSkinWeightInfo> SkinWeightsInfo;
    SkinWeightVertexBuffer->GetSkinWeights(SkinWeightsInfo);

    // Allocated SkinWeightData
    OutSkinWeights.SetNumUninitialized(SkinWeightsInfo.Num());

    // Loop thru vertices
    for (int32 VertexIndex = 0; VertexIndex < SkinWeightsInfo.Num(); VertexIndex++)
    {
        // Find Section From Global Vertex Index
        // NOTE: BoneMap is stored by Section.
        int32 OutSectionIndex;
        int32 OutSectionVertexIndex;
        LODRenderData.GetSectionFromVertexIndex(VertexIndex, OutSectionIndex, OutSectionVertexIndex);

        // Get Section for Vertex.
        const FSkelMeshRenderSection& RenderSection = LODRenderData.RenderSections[OutSectionIndex];

        // Get Vertex Weights
        const FSkinWeightInfo& SkinWeightInfo = SkinWeightsInfo[VertexIndex];

        // Store Weights
        for (int32 Index = 0; Index < MAX_TOTAL_INFLUENCES; Index++)
        {
            const uint8& BoneWeight = SkinWeightInfo.InfluenceWeights[Index];
            const uint16& BoneIndex = SkinWeightInfo.InfluenceBones[Index];
            const uint16& MeshBoneIndex = RenderSection.BoneMap[BoneIndex];

            OutSkinWeights[VertexIndex].BoneWeights[Index] = BoneWeight;
            OutSkinWeights[VertexIndex].MeshBoneIndices[Index] = MeshBoneIndex;
        }
    }
}

void GetSkinnedVertices(const USkeletalMeshComponent* SkeletalMeshComponent, const int32 LODIndex, TArray<FVector3f>& OutPositions)
{
    check(SkeletalMeshComponent);
    OutPositions.Reset();

    // Get SkeletalMesh
    const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
    check(SkeletalMesh);

    // Get Matrices
    TArray<FMatrix44f> RefToLocals;
    if (const USkinnedMeshComponent* LeaderComponent = SkeletalMeshComponent->LeaderPoseComponent.Get())
    {
        const TArray<int32>& LeaderBoneMap = SkeletalMeshComponent->GetLeaderBoneMap();
        const int32 NumBones = LeaderBoneMap.Num();

        if (NumBones == 0)
        {
            // This case indicates an invalid leader pose component (e.g. no skeletal mesh)
            SkeletalMeshComponent->CacheRefToLocalMatrices(RefToLocals);
        }
        else
        {
            RefToLocals.SetNumUninitialized(NumBones);
            TArray<FTransform3f> CurrTransforms;
            CurrTransforms.SetNumUninitialized(NumBones);

            const TArray<FTransform>& LeaderTransforms = LeaderComponent->GetComponentSpaceTransforms();
            for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
            {
                bool bFoundLeader = false;
                if (LeaderBoneMap.IsValidIndex(BoneIndex))
                {
                    const int32 LeaderIndex = LeaderBoneMap[BoneIndex];
                    if (LeaderIndex != INDEX_NONE && LeaderIndex < LeaderTransforms.Num())
                    {
                        bFoundLeader = true;
                        CurrTransforms[BoneIndex] = (FTransform3f)LeaderTransforms[LeaderIndex];
                    }
                }

                if (!bFoundLeader)
                {
                    const int32 ParentIndex = SkeletalMesh->GetRefSkeleton().GetParentIndex(BoneIndex);
                    FTransform3f BoneTransform = (FTransform3f)SkeletalMesh->GetRefSkeleton().GetRefBonePose()[BoneIndex];
                    if ((ParentIndex >= 0) && (ParentIndex < BoneIndex))
                    {
                        BoneTransform = BoneTransform * CurrTransforms[ParentIndex];
                    }
                    CurrTransforms[BoneIndex] = BoneTransform;
                }

                if (SkeletalMesh->GetRefBasesInvMatrix().IsValidIndex(BoneIndex))
                {
                    RefToLocals[BoneIndex] = SkeletalMesh->GetRefBasesInvMatrix()[BoneIndex] * CurrTransforms[BoneIndex].ToMatrixWithScale();
                }
                else
                {
                    RefToLocals[BoneIndex] = CurrTransforms[BoneIndex].ToMatrixWithScale();
                }
            }
        }
    }
    else
    {
        SkeletalMeshComponent->CacheRefToLocalMatrices(RefToLocals);
    }

    // Get Ref-Pose Vertices
    TArray<FVector3f> Vertices;
    const int32 NumVertices = GetVertices(SkeletalMesh, LODIndex, Vertices);
    OutPositions.SetNumUninitialized(NumVertices);

    // Get Weights
    TArray<VertexSkinWeightMax> SkinWeights;
    GetSkinWeights(SkeletalMesh, LODIndex, SkinWeights);

    for (int32 VertexIndex = 0; VertexIndex < NumVertices; VertexIndex++)
    {
        const FVector3f& Vertex = Vertices[VertexIndex];
        const VertexSkinWeightMax& Weights = SkinWeights[VertexIndex];

        FVector4f SkinnedVertex(0);
        for (int32 Index = 0; Index < MAX_TOTAL_INFLUENCES; Index++)
        {
            const uint8& BoneWeight = Weights.BoneWeights[Index];
            const uint16& MeshBoneIndex = Weights.MeshBoneIndices[Index];

            // Get Matrix
            const FMatrix44f& RefToLocal = RefToLocals[MeshBoneIndex];

            const float Weight = (float)BoneWeight / 255.f;
            SkinnedVertex += RefToLocal.TransformPosition(Vertex) * Weight;
        }

        OutPositions[VertexIndex] = SkinnedVertex;
    }

    // Morph Target
    const FMorphTargetWeightMap& ActiveMorphTargets = SkeletalMeshComponent->ActiveMorphTargets;
    const TArray<float>& MorphTargetWeights = SkeletalMeshComponent->MorphTargetWeights;

    for (const auto& MorphTargetWeightPair : ActiveMorphTargets)
    {
        const UMorphTarget* MorphTarget = MorphTargetWeightPair.Key;
        const int32 WeightIndex = MorphTargetWeightPair.Value;
        const float MorphWeight = MorphTargetWeights[WeightIndex];

        if (MorphWeight > KINDA_SMALL_NUMBER)
        {
            const FMorphTargetLODModel& MorphModel = MorphTarget->GetMorphLODModels()[LODIndex];
            if (MorphModel.Vertices.Num() > 0)
            {
                for (const FMorphTargetDelta& MorphDelta : MorphModel.Vertices)
                {
                    FVector3f& VertexPosition = OutPositions[MorphDelta.SourceIdx];
                    VertexPosition += MorphDelta.PositionDelta * MorphWeight;
                }
            }
        }
    }
}

void GenerateGCMeshDataFromSkeletalMeshComponents(const TArray<USkeletalMeshComponent*>& InSkeletalMeshComponents, FGeometryCacheMeshData& OutMeshData)
{
    // Reset previous mesh data
    OutMeshData.Positions.Reset();
    OutMeshData.Indices.Reset();
    OutMeshData.TextureCoordinates.Reset();
    OutMeshData.TangentsX.Reset();
    OutMeshData.TangentsZ.Reset();
    OutMeshData.Colors.Reset();
    OutMeshData.BatchesInfo.Reset();

    // Set vertex info
    OutMeshData.VertexInfo.bHasColor0 = true;
    OutMeshData.VertexInfo.bHasTangentX = true;
    OutMeshData.VertexInfo.bHasTangentZ = true;
    OutMeshData.VertexInfo.bHasUV0 = true;

    // Initialize Vertex and Index counters to keep track of offsets
    int32 VertexOffset = 0;
    int32 IndexOffset = 0;
    int32 TotalTriangles = 0;

    for (auto com : InSkeletalMeshComponents)
    {
        TArray<FVector3f> Positions;
        GetSkinnedVertices(com, 0, Positions);

        FSkeletalMeshLODRenderData& RenderData = com->GetSkeletalMeshAsset()->GetResourceForRendering()->LODRenderData[0];

        const FStaticMeshVertexBuffers& VertexBuffers = RenderData.StaticVertexBuffers;
        const FStaticMeshVertexBuffer& StaticMeshVertexBuffer = VertexBuffers.StaticMeshVertexBuffer;
        const FRawStaticIndexBuffer16or32Interface* IndexBuffer = RenderData.MultiSizeIndexContainer.GetIndexBuffer();

        if (!IndexBuffer)
        {
            continue;
        }

        const int32 NumVertices = Positions.Num();
        const int32 NumTexCoords = StaticMeshVertexBuffer.GetNumTexCoords();

        // Extract vertex positions and other information
        for (int32 VertexIndex = 0; VertexIndex < NumVertices; ++VertexIndex)
        {
            const FVector3f Position = Positions[VertexIndex];
            OutMeshData.Positions.Add(Position);

            // Set vertex color to white
            OutMeshData.Colors.Add(FColor::White);

            // Extract UVs
            if (NumTexCoords > 0)
            {
                FVector2f UV = StaticMeshVertexBuffer.GetVertexUV(VertexIndex, 0);  // Assuming the first UV set
                OutMeshData.TextureCoordinates.Add(UV);
            }

            // Extract tangents and normals
            const FVector3f TangentX = StaticMeshVertexBuffer.VertexTangentX(VertexIndex);
            const FVector3f TangentZ = StaticMeshVertexBuffer.VertexTangentZ(VertexIndex); // Normal

            OutMeshData.TangentsX.Add(TangentX);
            OutMeshData.TangentsZ.Add(TangentZ);
        }

        // Extract indices and adjust with the current vertex offset
        for (int32 SectionIndex = 0; SectionIndex < RenderData.RenderSections.Num(); ++SectionIndex)
        {
            const FSkelMeshRenderSection& RenderSection = RenderData.RenderSections[SectionIndex];

            // Calculate the total number of triangles
            TotalTriangles += RenderSection.NumTriangles;

            for (uint32 TriangleIndex = 0; TriangleIndex < RenderSection.NumTriangles * 3; ++TriangleIndex)
            {
                const uint32 IndexValue = IndexBuffer->Get(RenderSection.BaseIndex + TriangleIndex) + VertexOffset;
                OutMeshData.Indices.Add(IndexValue);
            }

            // Update the Index offset for the next section
            IndexOffset += RenderSection.NumTriangles * 3;
        }

        // Update Vertex offset for the next LOD
        VertexOffset += NumVertices;
    }

    // Create a single GeometryCacheMeshBatchInfo instance
    if (TotalTriangles > 0)
    {
        FGeometryCacheMeshBatchInfo BatchInfo;
        BatchInfo.StartIndex = 0;
        BatchInfo.NumTriangles = TotalTriangles;
        OutMeshData.BatchesInfo.Add(BatchInfo);
    }

    // Recalculate the bounding box
    OutMeshData.BoundingBox.Init();
    for (const FVector3f& Position : OutMeshData.Positions)
    {
        OutMeshData.BoundingBox += Position;
    }
}

UGeometryCache* CloEditorUtils::ConvertAnimationAssetToGeometryCache(UAnimationAsset* InAnimationAsset, TArray<USkeletalMeshComponent*> InSkeletalMeshes)
{
    if (!InAnimationAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid InAnimationAsset"));
        return nullptr;
    }

    if (InSkeletalMeshes.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("There is no exportable SkeletalMeshComponent."));
        return nullptr;
    }

    UAnimSequence* AnimSequence = Cast<UAnimSequence>(InAnimationAsset);
    if (!AnimSequence)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid AnimSequence"));
        return nullptr;
    }

    // Create a GeometryCache instance 
    UGeometryCache* GeometryCache = NewObject<UGeometryCache>(); // CreateObjectInstance<UGeometryCache>(TEXT("/Game"), TEXT("TestGC"), RF_Public | RF_Standalone);
    UGeometryCacheCodecV1* Codec = NewObject<UGeometryCacheCodecV1>(GeometryCache, FName(*FString(TEXT("Flattened_Codec"))), RF_Public);

    /** Precision used for compressing vertex positions (lower = better result but less compression, higher = more lossy compression but smaller size)  ClampMin = "0.000001", ClampMax = "1000", UIMin = "0.01", UIMax = "100" */
    float CompressedPositionPrecision = 0.01f;
    /** Bit-precision used for compressing texture coordinates (hight = better result but less compression, lower = more lossy compression but smaller size) ClampMin = "1", ClampMax = "31", UIMin = "4", UIMax = "16" */
    int32 CompressedTextureCoordinatesNumberOfBits = 16;

    Codec->InitializeEncoder(CompressedPositionPrecision, CompressedTextureCoordinatesNumberOfBits);
    UGeometryCacheTrackStreamable* Track = NewObject<UGeometryCacheTrackStreamable>(GeometryCache, FName(*FString(TEXT("Flattened_Track"))), RF_Public);
    Track->BeginCoding(Codec, true, false, false);

    // Sample animation and fill GeometryCacheTrack
    const int NumberOfFrames = AnimSequence->GetNumberOfSampledKeys();
    const float AnimationLength = AnimSequence->GetPlayLength();

    for (int i = 0; i < NumberOfFrames; ++i)
    {
        const float TimeInSeconds = AnimSequence->GetSamplingFrameRate().AsSeconds(FFrameTime(i));
        
        for (auto SkelComp : InSkeletalMeshes)
        {
            SkelComp->SetPosition(TimeInSeconds);
            SkelComp->TickAnimation(0.f, false);
            SkelComp->RefreshBoneTransforms(nullptr);
        }

        // Create GeometryCacheMeshData
        FGeometryCacheMeshData MeshData;
        GenerateGCMeshDataFromSkeletalMeshComponents(InSkeletalMeshes, MeshData);

        // Add the frame to the track
        Track->AddMeshSample(MeshData, TimeInSeconds, true);
    }

    UMaterialInterface* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
    GeometryCache->Materials.Add(DefaultMaterial);

    TArray<FMatrix> Mats;
    Mats.Add(FMatrix::Identity);
    Mats.Add(FMatrix::Identity);
    TArray<float> MatTimes;
    MatTimes.Add(0.0f);
    MatTimes.Add(AnimationLength);
    Track->SetMatrixSamples(Mats, MatTimes);

    Track->EndCoding();
    GeometryCache->AddTrack(Track);
    GeometryCache->SetFrameStartEnd(0, NumberOfFrames);

    return GeometryCache;
}

USkeletalMesh* CloEditorUtils::ConvertSkeletalMeshesToSingleSkeletalMesh(TArray<USkeletalMeshComponent*> SkeletalMeshComponents)
{
    if (SkeletalMeshComponents.IsEmpty())
    {
        return nullptr;
    }

    TArray<USkeletalMesh*> MeshesToMerge;
    USkeleton* MasterSkeleton = nullptr;

    for (USkeletalMeshComponent* Component : SkeletalMeshComponents)
    {
        if (Component && Component->GetSkeletalMeshAsset())
        {
            MeshesToMerge.Add(Component->GetSkeletalMeshAsset());

            if (!MasterSkeleton && Component->GetSkeletalMeshAsset()->GetSkeleton())
            {
                MasterSkeleton = Component->GetSkeletalMeshAsset()->GetSkeleton();
            }
        }
    }

    if (MeshesToMerge.Num() == 0 || !MasterSkeleton)
    {
        return nullptr;
    }

    USkeletalMesh* BaseMesh = nullptr;
    if (MeshesToMerge.Num() == 1)
    {
        BaseMesh = DuplicateObject<USkeletalMesh>(MeshesToMerge[0], GetTransientPackage());
        if (BaseMesh)
        {
            BaseMesh->SetSkeleton(MasterSkeleton);
        }
    }
    else
    {
        FSkeletalMeshMergeParams SkeletalMeshMergeParams;
        SkeletalMeshMergeParams.MeshesToMerge = MeshesToMerge;
        SkeletalMeshMergeParams.Skeleton = MasterSkeleton;
        SkeletalMeshMergeParams.bSkeletonBefore = true;
        SkeletalMeshMergeParams.StripTopLODS = 0;

        BaseMesh = USkeletalMergingLibrary::MergeMeshes(SkeletalMeshMergeParams);
    }

    if (BaseMesh)
    {
        // TODO: Export AnimBlueprints for all SkeletalMeshComponents in Persona.
        CloEditorUtils::TransferRenderDataToImportedModel(BaseMesh);
    }

    return BaseMesh;
}

void CalculateBonesToRemove(const FSkeletalMeshLODRenderData& LODResource, const FReferenceSkeleton& RefSkeleton, TArray<FBoneReference>& OutBonesToRemove)
{
    const int32 NumBones = RefSkeleton.GetNum();
    OutBonesToRemove.Empty(NumBones);

    TArray<bool> RemovedBones;
    RemovedBones.Init(true, NumBones);

    for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
    {
        if (LODResource.RequiredBones.Find((uint16)BoneIndex) != INDEX_NONE)
        {
            RemovedBones[BoneIndex] = false;
            continue;
        }

        const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
        if (RemovedBones.IsValidIndex(ParentIndex) && !RemovedBones[ParentIndex])
        {
            OutBonesToRemove.Add(RefSkeleton.GetBoneName(BoneIndex));
        }
    }
}

void CloEditorUtils::TransferRenderDataToImportedModel(USkeletalMesh* InSkeletalMesh)
{
    if ((InSkeletalMesh == nullptr)
        || (InSkeletalMesh->GetResourceForRendering() == nullptr)
        || (InSkeletalMesh->GetImportedModel() == nullptr))
    {
        return;
    }

    FSkeletalMeshRenderData* RenderData = InSkeletalMesh->GetResourceForRendering();
    FSkeletalMeshModel* ImportedModel = InSkeletalMesh->GetImportedModel();

    ImportedModel->bGuidIsHash = false;
    ImportedModel->SkeletalMeshModelGUID = FGuid::NewGuid();

    ImportedModel->LODModels.Empty();

    for (int32 LodIndex = 0; LodIndex < RenderData->LODRenderData.Num(); ++LodIndex)
    {
        FSkeletalMeshLODRenderData& LODRenderData = RenderData->LODRenderData[LodIndex];
        FSkeletalMeshLODModel* LODModel = new FSkeletalMeshLODModel();

        LODModel->NumTexCoords = LODRenderData.GetNumTexCoords();
        LODModel->NumVertices = LODRenderData.GetNumVertices();
        LODModel->RequiredBones = LODRenderData.RequiredBones;

        // index buffer
        if (LODRenderData.MultiSizeIndexContainer.IsIndexBufferValid())
        {
            const int32 NumIndices = LODRenderData.MultiSizeIndexContainer.GetIndexBuffer()->Num();
            LODModel->IndexBuffer.SetNum(NumIndices);
            for (int32 Index = 0; Index < NumIndices; ++Index)
            {
                LODModel->IndexBuffer[Index] = LODRenderData.MultiSizeIndexContainer.GetIndexBuffer()->Get(Index);
            }
        }

        LODModel->Sections.SetNum(LODRenderData.RenderSections.Num());

        int32 CurrentSectionInitialVertex = 0;
        for (int32 SectionIndex = 0; SectionIndex < LODRenderData.RenderSections.Num(); ++SectionIndex)
        {
            const FSkelMeshRenderSection& RenderSection = LODRenderData.RenderSections[SectionIndex];
            FSkelMeshSection& Section = LODModel->Sections[SectionIndex];

            Section.BaseIndex = RenderSection.BaseIndex;
            Section.BaseVertexIndex = RenderSection.BaseVertexIndex;
            Section.NumTriangles = RenderSection.NumTriangles;
            Section.MaterialIndex = RenderSection.MaterialIndex;
            Section.BoneMap = RenderSection.BoneMap;
            Section.MaxBoneInfluences = RenderSection.MaxBoneInfluences;
            Section.NumVertices = RenderSection.GetNumVertices();
            Section.bUse16BitBoneIndex = LODRenderData.DoesVertexBufferUse16BitBoneIndex();
            Section.SoftVertices.SetNum(Section.NumVertices);


            Section.CorrespondClothAssetIndex = RenderSection.CorrespondClothAssetIndex;
            Section.ClothingData = RenderSection.ClothingData;

            for (int32 VertexIndex = 0; VertexIndex < Section.NumVertices; ++VertexIndex)
            {
                FSoftSkinVertex& Vertex = Section.SoftVertices[VertexIndex];

                // vertex buffer
                const FPositionVertexBuffer& PositionVertexBuffer = LODRenderData.StaticVertexBuffers.PositionVertexBuffer;
                const FStaticMeshVertexBuffer& StaticMeshVertexBuffer = LODRenderData.StaticVertexBuffers.StaticMeshVertexBuffer;
                const FSkinWeightVertexBuffer* SkinWeightVertexBuffer = LODRenderData.GetSkinWeightVertexBuffer();

                Vertex.Position = PositionVertexBuffer.VertexPosition(VertexIndex + Section.BaseVertexIndex);

                // tangent buffer
                Vertex.TangentX = StaticMeshVertexBuffer.VertexTangentX(VertexIndex + Section.BaseVertexIndex);
                Vertex.TangentY = StaticMeshVertexBuffer.VertexTangentY(VertexIndex + Section.BaseVertexIndex);
                Vertex.TangentZ = StaticMeshVertexBuffer.VertexTangentZ(VertexIndex + Section.BaseVertexIndex);

                for (uint32 UVIndex = 0; UVIndex < LODModel->NumTexCoords; ++UVIndex)
                {
                    Vertex.UVs[UVIndex] = StaticMeshVertexBuffer.GetVertexUV(VertexIndex + Section.BaseVertexIndex, UVIndex);
                }

                // skin weight
                if (SkinWeightVertexBuffer)
                {
                    const FSkinWeightInfo& SkinWeights = SkinWeightVertexBuffer->GetVertexSkinWeights(VertexIndex + Section.BaseVertexIndex);
                    for (int32 InfluenceIndex = 0; InfluenceIndex < MAX_TOTAL_INFLUENCES; ++InfluenceIndex)
                    {
                        Vertex.InfluenceBones[InfluenceIndex] = SkinWeights.InfluenceBones[InfluenceIndex];
                        Vertex.InfluenceWeights[InfluenceIndex] = SkinWeights.InfluenceWeights[InfluenceIndex];
                    }
                }

                Vertex.Color = FColor::White;
            }

            // UserSectionsData
            FSkelMeshSourceSectionUserData& SectionUserData = LODModel->UserSectionsData.FindOrAdd(Section.OriginalDataSectionIndex);
            SectionUserData.bCastShadow = RenderSection.bCastShadow;
            SectionUserData.bDisabled = RenderSection.bDisabled;

            // ClothingData
            SectionUserData.CorrespondClothAssetIndex = RenderSection.CorrespondClothAssetIndex;
            SectionUserData.ClothingData.AssetGuid = RenderSection.ClothingData.AssetGuid;
            SectionUserData.ClothingData.AssetLodIndex = RenderSection.ClothingData.AssetLodIndex;

            // Update vertex offset
            CurrentSectionInitialVertex += Section.NumVertices;
        }

        // Update ActiveBoneIndices and calculate RequiredBones
        LODModel->SyncronizeUserSectionsDataArray();
        LODModel->ActiveBoneIndices = LODRenderData.ActiveBoneIndices;

        CalculateBonesToRemove(LODRenderData, InSkeletalMesh->GetRefSkeleton(), InSkeletalMesh->GetLODInfo(LodIndex)->BonesToRemove);

        USkeletalMesh::CalculateRequiredBones(*LODModel, InSkeletalMesh->GetRefSkeleton(), nullptr);

        // DDC keys
        const USkeletalMeshLODSettings* LODSettings = InSkeletalMesh->GetLODSettings();
        const bool bValidLODSettings = LODSettings && LODSettings->GetNumberOfSettings() > LodIndex;
        const FSkeletalMeshLODGroupSettings* SkeletalMeshLODGroupSettings = bValidLODSettings ? &LODSettings->GetSettingsForLODLevel(LodIndex) : nullptr;

        FSkeletalMeshLODInfo* LODInfo = InSkeletalMesh->GetLODInfo(LodIndex);
        LODInfo->BuildGUID = LODInfo->ComputeDeriveDataCacheKey(SkeletalMeshLODGroupSettings);

        LODModel->BuildStringID = LODModel->GetLODModelDeriveDataKey();

        ImportedModel->LODModels.Add(LODModel);
    }

    InSkeletalMesh->CalculateInvRefMatrices();
    InSkeletalMesh->PostEditChange();
    InSkeletalMesh->MarkPackageDirty();
}

void CloEditorUtils::GetPoseAtTime(UAnimationAsset* InAnimAsset, double Time, FAnimationPoseData& OutPoseData)
{
    if (UAnimSequence* Sequence = Cast<UAnimSequence>(InAnimAsset))
    {
        FAnimExtractContext Context(Time, false);
        Context.bIgnoreRootLock = true;
        Sequence->GetAnimationPose(OutPoseData, Context);

        FTransform RootMotionTransform;
        const FSkeletonPoseBoneIndex RootBoneIndex(0);
        Sequence->GetBoneTransform(RootMotionTransform, RootBoneIndex, Context, false);
        OutPoseData.GetPose()[FCompactPoseBoneIndex(0)] = RootMotionTransform;
    }
    else if (UPoseAsset* PoseAsset = Cast<UPoseAsset>(InAnimAsset))
    {
        FAnimExtractContext PoseExtractionContext;
        if (PoseAsset->GetPoseFNames().Num() > 0)
        {
            const FName PoseName = PoseAsset->GetPoseFNames()[0];
            const int32 PoseIndex = PoseAsset->GetPoseIndexByName(PoseName);
            if (PoseIndex != INDEX_NONE)
            {
                PoseExtractionContext.PoseCurves.Emplace(PoseIndex, PoseName, 1.0f);
            }
        }
        PoseAsset->GetAnimationPose(OutPoseData, PoseExtractionContext);
    }
    else
    {
        OutPoseData.GetPose().ResetToRefPose();
    }
}

UAnimSequence* CloEditorUtils::CreateBakedAnimSequenceWithRootMotion(UAnimSequence* OriginalAnimSequence, USkeletalMesh* TargetSkeletalMesh)
{
    if (!OriginalAnimSequence || !TargetSkeletalMesh || !TargetSkeletalMesh->GetSkeleton())
    {
        return nullptr;
    }

    USkeleton* TargetSkeleton = TargetSkeletalMesh->GetSkeleton();
    const FReferenceSkeleton& RefSkeleton = TargetSkeleton->GetReferenceSkeleton();

    UAnimSequenceFactory* Factory = NewObject<UAnimSequenceFactory>();
    Factory->TargetSkeleton = TargetSkeleton;
    Factory->PreviewSkeletalMesh = TargetSkeletalMesh;

    UAnimSequence* BakedAnimSequence = Cast<UAnimSequence>(Factory->FactoryCreateNew(UAnimSequence::StaticClass(), GetTransientPackage(), NAME_None, RF_Public | RF_Standalone, nullptr, GWarn));

    if (!BakedAnimSequence)
    {
        return nullptr;
    }

    IAnimationDataController& Controller = BakedAnimSequence->GetController();
    const FFrameRate SamplingRate = OriginalAnimSequence->GetSamplingFrameRate();
    const FFrameNumber NumFrames = OriginalAnimSequence->GetNumberOfSampledKeys();

    Controller.SetFrameRate(SamplingRate, false);
    Controller.SetNumberOfFrames(NumFrames, false);

    FBoneContainer BoneContainer;
    TArray<FBoneIndexType> RequiredBoneIndices;
    RequiredBoneIndices.SetNum(RefSkeleton.GetNum());
    for (int32 Index = 0; Index < RefSkeleton.GetNum(); ++Index)
    {
        RequiredBoneIndices[Index] = static_cast<FBoneIndexType>(Index);
    }
    UE::Anim::FCurveFilterSettings CurveFilterSettings(UE::Anim::ECurveFilterMode::None);
    BoneContainer.InitializeTo(RequiredBoneIndices, CurveFilterSettings, *TargetSkeleton);

    TMap<FName, FRawAnimSequenceTrack> BoneTracks;
    BoneTracks.Reserve(RefSkeleton.GetNum());

    TMap<FName, TArray<FRichCurveKey>> AllCurveKeys;

    for (int32 FrameIndex = 0; FrameIndex < NumFrames.Value; ++FrameIndex)
    {
        FMemMark Mark(FMemStack::Get());
        const double CurrentTime = SamplingRate.AsSeconds(FFrameNumber(FrameIndex));

        FCompactPose Pose;
        Pose.SetBoneContainer(&BoneContainer);
        FBlendedCurve Curve;
        Curve.InitFrom(BoneContainer);
        UE::Anim::FStackAttributeContainer Attributes;
        FAnimationPoseData PoseData(Pose, Curve, Attributes);

        GetPoseAtTime(OriginalAnimSequence, CurrentTime, PoseData);

        Curve.ForEachElement([&AllCurveKeys, CurrentTime](const UE::Anim::FCurveElement& CurveElement)
        {
            AllCurveKeys.FindOrAdd(CurveElement.Name).Emplace(CurrentTime, CurveElement.Value);
        });

        TArray<FTransform> ComponentSpaceTransforms;
        ComponentSpaceTransforms.SetNum(RefSkeleton.GetNum());

        if (RefSkeleton.GetNum() > 0)
        {
            const FCompactPoseBoneIndex RootCompactIndex(0);
            ComponentSpaceTransforms[0] = Pose[RootCompactIndex];

            for (int32 BoneIndex = 1; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
            {
                const FCompactPoseBoneIndex CompactBoneIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(BoneIndex);
                if (CompactBoneIndex.IsValid())
                {
                    const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
                    ComponentSpaceTransforms[BoneIndex] = Pose[CompactBoneIndex] * ComponentSpaceTransforms[ParentIndex];
                }
            }
        }

        for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
        {
            const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
            FRawAnimSequenceTrack& Track = BoneTracks.FindOrAdd(BoneName);

            FTransform NewLocalTransform;
            const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);

            if (ParentIndex == INDEX_NONE)
            {
                NewLocalTransform = FTransform::Identity;
            }
            else if (ParentIndex == 0)
            {
                NewLocalTransform = ComponentSpaceTransforms[BoneIndex];
            }
            else
            {
                NewLocalTransform = ComponentSpaceTransforms[BoneIndex].GetRelativeTransform(ComponentSpaceTransforms[ParentIndex]);
            }

            Track.PosKeys.Add(static_cast<FVector3f>(NewLocalTransform.GetLocation()));
            Track.RotKeys.Add(static_cast<FQuat4f>(NewLocalTransform.GetRotation()));
            Track.ScaleKeys.Add(static_cast<FVector3f>(NewLocalTransform.GetScale3D()));
        }
    }

    Controller.OpenBracket(FText::FromString(TEXT("Bake and Transfer Root Motion")), false);
    for (const TPair<FName, FRawAnimSequenceTrack>& TrackPair : BoneTracks)
    {
        Controller.AddBoneCurve(TrackPair.Key, false);
        Controller.SetBoneTrackKeys(TrackPair.Key, TrackPair.Value.PosKeys, TrackPair.Value.RotKeys, TrackPair.Value.ScaleKeys, false);
    }

    const TScriptInterface<const IAnimationDataModel> Model = Controller.GetModelInterface();
    for (const TPair<FName, TArray<FRichCurveKey>>& CurvePair : AllCurveKeys)
    {
        const FAnimationCurveIdentifier CurveId(CurvePair.Key, ERawCurveTrackTypes::RCT_Float);

        int32 CurveFlags = AACF_Editable;
        if (const FFloatCurve* OriginalCurve = Model->FindFloatCurve(CurveId))
        {
            CurveFlags = OriginalCurve->GetCurveTypeFlags();
        }

        if (Model->FindFloatCurve(CurveId) == nullptr)
        {
            Controller.AddCurve(CurveId, CurveFlags, false);
        }

        Controller.SetCurveKeys(CurveId, CurvePair.Value, false);
    }

    Controller.NotifyPopulated();
    Controller.CloseBracket(false);

    BakedAnimSequence->PostEditChange();
    BakedAnimSequence->MarkPackageDirty();
    BakedAnimSequence->BeginCacheDerivedDataForCurrentPlatform();


    // Force Finalization of Animation Data:
	// The newly created BakedAnimSequence only contains raw data at this point.
	// For performance optimization, the engine does not generate the final data (e.g., compressed data) until it is actually needed.
	// The code below forces the animation to be evaluated at least once using a temporary component.
	// This ensures that all final data required for the export is generated synchronously and immediately.
	// Without this step, the first export would fail because it would execute with unprepared data.
    UDebugSkelMeshComponent* TempComponent = NewObject<UDebugSkelMeshComponent>();
    TempComponent->SetSkeletalMesh(TargetSkeletalMesh);
    TempComponent->EnablePreview(true, BakedAnimSequence);
    TempComponent->SetPosition(0.0f, false);
    TempComponent->RefreshBoneTransforms();

    return BakedAnimSequence;
}