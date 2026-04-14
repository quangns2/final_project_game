// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#include "Schemas/CloUsdSkelSkeletonTranslator.h"

#if USE_USD_SDK

#include "USDAssetCache3.h"
#include "USDLog.h"
#include "Objects/USDPrimLinkCache.h"
#include "USDObjectUtils.h"
#include "USDSkeletalDataConversion.h"
#include "USDSkelSkeletonTranslator.h"
#include "USDTypesConversion.h"
#include "UsdWrappers/SdfPath.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/Package.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "ObjectTools.h"
#endif

#include "Utils/CloLiveSyncCoreUtils.h" 

#if USE_USD_SDK
#include "USDIncludesStart.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/topology.h"
#include "pxr/usd/usdSkel/utils.h"
#include "USDIncludesEnd.h"
#endif // USE_USD_SDK

USceneComponent* FCloUsdSkelSkeletonTranslator::CreateComponents()
{
	USceneComponent* SceneComponent = FUsdSkelSkeletonTranslator::CreateComponents();

	USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(SceneComponent);
	if (!SkeletalMeshComponent || !SkeletalMeshComponent->GetSkeletalMeshAsset())
	{
		return SceneComponent;
	}

	if (SkeletalMeshComponent->GetAnimationMode() == EAnimationMode::AnimationBlueprint || SkeletalMeshComponent->AnimationData.AnimToPlay != nullptr)
	{
		return SceneComponent;
	}

	const pxr::UsdPrim& SkeletonPrim = GetPrim();
	const UE::FSdfPath SkeletonPrimPath{ SkeletonPrim.GetPath() };
	USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();

	// Find the parent SkelRoot prim by walking up the hierarchy
	pxr::UsdPrim ClosestParentSkelRootPrim;
	pxr::UsdPrim CurrentPrim = SkeletonPrim.GetParent();
	while (CurrentPrim)
	{
		if (CurrentPrim.IsA<pxr::UsdSkelRoot>())
		{
			ClosestParentSkelRootPrim = CurrentPrim;
			break;
		}
		CurrentPrim = CurrentPrim.GetParent();
	}

	// If no explicit SkelRoot found, try using the direct parent as SkelRoot
	if (!ClosestParentSkelRootPrim)
	{
		ClosestParentSkelRootPrim = SkeletonPrim.GetParent();
	}

	if (!ClosestParentSkelRootPrim)
	{
		UE_LOG(LogUsd, Warning,
			TEXT("Ignoring skeleton '%s' when creating rest pose - cannot find parent prim"),
			*PrimPath.GetString()
		);
		return SceneComponent;
	}

	pxr::UsdSkelRoot SkelRoot{ ClosestParentSkelRootPrim };
	if (!SkelRoot)
	{
		UE_LOG(LogUsd, Warning,
			TEXT("Skeleton '%s' parent prim '%s' is not a valid SkelRoot, trying anyway"),
			*PrimPath.GetString(),
			*UsdToUnreal::ConvertPath(ClosestParentSkelRootPrim.GetPath())
		);
		// Continue anyway - the parent might still work even if it's not explicitly typed as SkelRoot
	}

	pxr::UsdSkelCache SkelCache;
	SkelCache.Populate(SkelRoot, pxr::UsdTraverseInstanceProxies());
	pxr::UsdSkelSkeletonQuery SkeletonQuery = SkelCache.GetSkelQuery(pxr::UsdSkelSkeleton(SkeletonPrim));
	if (!SkeletonQuery)
	{
		return SceneComponent;
	}

	pxr::VtArray<pxr::GfMatrix4d> JointLocalBindTransforms;
	pxr::VtArray<pxr::GfMatrix4d> JointLocalRestTransforms;
	const bool bAtRest = true;

	const pxr::UsdSkelTopology& SkelTopology = SkeletonQuery.GetTopology();
	pxr::VtArray<pxr::GfMatrix4d> WorldBindTransforms;
	SkeletonQuery.GetJointWorldBindTransforms(&WorldBindTransforms);
	pxr::UsdSkelComputeJointLocalTransforms(SkelTopology, WorldBindTransforms, &JointLocalBindTransforms);
	SkeletonQuery.ComputeJointLocalTransforms(&JointLocalRestTransforms, pxr::UsdTimeCode::Default(), bAtRest);

	if (JointLocalBindTransforms.size() == 0 || JointLocalBindTransforms.size() != JointLocalRestTransforms.size())
	{
		return SceneComponent;
	}

	bool bPosesAreDifferent = false;
	for (size_t i = 0; i < JointLocalBindTransforms.size(); ++i)
	{
		if (!pxr::GfIsClose(JointLocalBindTransforms[i], JointLocalRestTransforms[i], 0.0001f))
		{
			bPosesAreDifferent = true;
			break;
		}
	}

	if (!bPosesAreDifferent)
	{
		return SceneComponent;
	}


	const FString DesiredName = SkeletalMesh->GetName() + TEXT("_RestPose");
	FString PackageBasePath = FPackageName::GetLongPackagePath(SkeletalMesh->GetOutermost()->GetName());
	const FString OutputContentFolderPath = CloCoreUtils::GetUsdOutputContentFolderPath();
	if (!OutputContentFolderPath.Equals(CloCoreUtils::GetUsdAssetCacheFolderPath()))
	{
		const FString RootPath = Context->Stage.GetRootLayer().GetRealPath();
		FString FileName = UsdUnreal::ObjectUtils::SanitizeObjectName(FPaths::GetBaseFilename(RootPath));
		FileName.ReplaceInline(TEXT(" "), TEXT("_"));

		PackageBasePath = OutputContentFolderPath + TEXT("/") + FileName + TEXT("/SkeletalMeshes");
	}

	UPackage* Package = FindPackage(nullptr, *PackageBasePath);
	if (Package)
	{
		UObject* ExistingObject = FindObject<UObject>(Package, *DesiredName);
		if (ExistingObject)
		{
			const bool bDeleteSucceeded = ObjectTools::DeleteSingleObject(ExistingObject);
			if (bDeleteSucceeded)
			{
				CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			}
			else
			{
				UE_LOG(LogUsd, Error, TEXT("Failed to delete existing asset '%s' to overwrite it."), *ExistingObject->GetPathName());
				return SceneComponent;
			}
		}
	}

	Package = CreatePackage(*PackageBasePath);
	UAnimSequence* RestPoseAnim = NewObject<UAnimSequence>(Package, FName(*DesiredName), RF_Transient);
	if (RestPoseAnim)
	{
		RestPoseAnim->SetSkeleton(SkeletalMesh->GetSkeleton());

		IAnimationDataController& Controller = RestPoseAnim->GetController();
		const bool bShouldTransact = false;
		Controller.OpenBracket(FText::FromString("Creating Rest Pose Animation"), bShouldTransact);
		Controller.InitializeModel();
		Controller.ResetModel(bShouldTransact);

		const FReferenceSkeleton& RefSkeleton = SkeletalMesh->GetRefSkeleton();
		const TArray<FMeshBoneInfo>& BoneInfo = RefSkeleton.GetRawRefBoneInfo();
		const int32 NumBones = BoneInfo.Num();

		for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
		{
			Controller.AddBoneCurve(BoneInfo[BoneIndex].Name, bShouldTransact);

			FTransform RestTransform = FTransform::Identity;

			if (BoneIndex < static_cast<int32>(JointLocalRestTransforms.size()))
			{
				const FUsdStageInfo StageInfo(Context->Stage);

				FMatrix RestMatrix = UsdToUnreal::ConvertMatrix(JointLocalRestTransforms[BoneIndex]);
				FTransform RawRestTransform(RestMatrix);

				RestTransform = UsdUtils::ConvertTransformToUESpace(StageInfo, RawRestTransform);
			}

			Controller.SetBoneTrackKeys(
				BoneInfo[BoneIndex].Name,
				{ (FVector3f)RestTransform.GetTranslation() },
				{ (FQuat4f)RestTransform.GetRotation() },
				{ (FVector3f)RestTransform.GetScale3D() },
				bShouldTransact
			);
		}

		const FFrameRate FrameRate(30, 1);
		Controller.SetFrameRate(FrameRate, bShouldTransact);
		Controller.SetNumberOfFrames(FFrameNumber(1), bShouldTransact);

		Controller.NotifyPopulated();
		Controller.CloseBracket(bShouldTransact);

		RestPoseAnim->PostEditChange();
		RestPoseAnim->MarkPackageDirty();
	}

	if (RestPoseAnim)
	{
		SkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		SkeletalMeshComponent->AnimationData.AnimToPlay = RestPoseAnim;
		SkeletalMeshComponent->SetAnimation(RestPoseAnim);

		SkeletalMeshComponent->Play(false);
		SkeletalMeshComponent->SetPosition(0.0f);
		SkeletalMeshComponent->Stop();
	}

	return SceneComponent;
}

#endif