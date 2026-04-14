// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#include "PoseToAnimationCombinerWidget.h"
#include "PoseToAnimationCombinerArguments.h"
#include "PoseToAnimationCombinerArgumentsDetails.h"
#include "Utils/CloLiveSyncEditorUtils.h"

#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "Animation/PoseAsset.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Dialogs/DlgPickPath.h"
#include "Factories/AnimSequenceFactory.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/MemStack.h"

#define LOCTEXT_NAMESPACE "AnimationSequenceCombiner"

void SPoseToAnimationCombinerWidget::Construct(const FArguments& InArgs)
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

    PropertyEditorModule.RegisterCustomClassLayout(
        UPoseToAnimationCombinerArguments::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FPoseToAnimationCombinerArgumentsDetails::MakeInstance)
    );
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsViewArgs.bAllowSearch = false;

    Arguments = TStrongObjectPtr<UPoseToAnimationCombinerArguments>(NewObject<UPoseToAnimationCombinerArguments>());

    DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    DetailsView->SetObject(Arguments.Get());

    ChildSlot
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .Padding(5.0f)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    DetailsView->AsShared()
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign(HAlign_Right)
            .Padding(5.0f)
            [
                SNew(SButton)
                .ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
                .ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
                .ForegroundColor(FLinearColor::White)
                .OnClicked(this, &SPoseToAnimationCombinerWidget::OnCombineClicked)
                .Text(LOCTEXT("CombineButton", "Combine"))
            ]
        ];
}

SPoseToAnimationCombinerWidget::~SPoseToAnimationCombinerWidget()
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyEditorModule.UnregisterCustomClassLayout(UPoseToAnimationCombinerArguments::StaticClass()->GetFName());
}

FReply SPoseToAnimationCombinerWidget::OnCombineClicked()
{
    if (!Arguments->StartPose || !Arguments->TargetAnimation)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("InputError", "Please assign both First and Second animations."));
        return FReply::Handled();
    }
    if (Arguments->StartPose->GetSkeleton() != Arguments->TargetAnimation->GetSkeleton())
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SkeletonError", "Skeletons of both animations must match."));
        return FReply::Handled();
    }

    FString NewAssetName = Arguments->StartPose->GetName() + TEXT("_") + Arguments->TargetAnimation->GetName() + TEXT("_Combined");

    UAnimSequence* MergedAnim = CombineAnimations(Arguments->StartPose.Get(), Arguments->TargetAnimation.Get(), Arguments->TransitionDuration, Arguments->OutputPath, NewAssetName);

    if (MergedAnim)
    {
        FMessageDialog::Open(EAppMsgCategory::Success, EAppMsgType::Ok, FText::Format(LOCTEXT("SuccessMessage", "Successfully created combined animation: {0}"), FText::FromString(MergedAnim->GetPathName())));
    }
    else
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("FailureMessage", "Failed to create combined animation. Check the Output Log for details."));
    }

    return FReply::Handled();
}

float SPoseToAnimationCombinerWidget::GetAnimationAssetPlayLength(UAnimationAsset* AnimAsset)
{
    if (UAnimSequence* Sequence = Cast<UAnimSequence>(AnimAsset))
    {
        return Sequence->GetPlayLength();
    }
    return 0.0f;
}

UAnimSequence* SPoseToAnimationCombinerWidget::CombineAnimations(UAnimationAsset* StartAnim, UAnimationAsset* EndAnim, float InTransitionDuration, const FString& InAssetPath, const FString& InAssetName)
{
    if (!StartAnim || !EndAnim || StartAnim->GetSkeleton() != EndAnim->GetSkeleton())
    {
        return nullptr;
    }

    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    const FString PackagePath = InAssetPath.EndsWith(TEXT("/")) ? InAssetPath + InAssetName : InAssetPath + TEXT("/") + InAssetName;

    FString UniqueAssetName;
    FString UniquePackageName;
    AssetTools.CreateUniqueAssetName(PackagePath, TEXT(""), UniquePackageName, UniqueAssetName);

    UAnimSequenceFactory* Factory = NewObject<UAnimSequenceFactory>();
    Factory->TargetSkeleton = StartAnim->GetSkeleton();

    UObject* NewAsset = AssetTools.CreateAsset(UniqueAssetName, FPaths::GetPath(UniquePackageName), UAnimSequence::StaticClass(), Factory);
    UAnimSequence* NewAnimSequence = Cast<UAnimSequence>(NewAsset);
    if (!NewAnimSequence) return nullptr;

    NewAnimSequence->bEnableRootMotion = true;
    if (UAnimSequence* EndSequence = Cast<UAnimSequence>(EndAnim))
    {
        NewAnimSequence->RootMotionRootLock = EndSequence->RootMotionRootLock;
    }

    IAnimationDataController& Controller = NewAnimSequence->GetController();
    Controller.OpenBracket(LOCTEXT("CombineBracket", "Combining Pose and Animation Sequence"));

    const float StartLength = GetAnimationAssetPlayLength(StartAnim);
    const float EndLength = GetAnimationAssetPlayLength(EndAnim);
    const float TotalLength = StartLength + InTransitionDuration + EndLength;

    FFrameRate FrameRate(30, 1);
    if (UAnimSequenceBase* EndSequence = Cast<UAnimSequenceBase>(EndAnim))
    {
        FrameRate = EndSequence->GetSamplingFrameRate();
    }
    else if (UAnimSequenceBase* StartSequence = Cast<UAnimSequenceBase>(StartAnim))
    {
        FrameRate = StartSequence->GetSamplingFrameRate();
    }

    const FFrameNumber TotalFrames = FrameRate.AsFrameNumber(TotalLength);

    Controller.SetFrameRate(FrameRate);
    Controller.SetNumberOfFrames(TotalFrames);

    USkeleton* Skeleton = StartAnim->GetSkeleton();
    const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();

    FBoneContainer BoneContainer;
    TArray<FBoneIndexType> RequiredBoneIndices;
    RequiredBoneIndices.SetNum(RefSkeleton.GetNum());
    for (int32 Index = 0; Index < RefSkeleton.GetNum(); ++Index)
    {
        RequiredBoneIndices[Index] = static_cast<FBoneIndexType>(Index);
    }
    UE::Anim::FCurveFilterSettings CurveFilterSettings(UE::Anim::ECurveFilterMode::None);
    BoneContainer.InitializeTo(RequiredBoneIndices, CurveFilterSettings, *Skeleton);

    const int32 NumBones = BoneContainer.GetCompactPoseNumBones();
    TArray<TArray<FVector3f>> AllPositionKeys;
    TArray<TArray<FQuat4f>> AllRotationKeys;
    TArray<TArray<FVector3f>> AllScaleKeys;
    AllPositionKeys.SetNum(NumBones);
    AllRotationKeys.SetNum(NumBones);
    AllScaleKeys.SetNum(NumBones);
    TMap<FName, TArray<FRichCurveKey>> AllCurveKeys;
    TArray<FName> SkeletonCurveNames;
    Skeleton->GetCurveMetaDataNames(SkeletonCurveNames);

    for (int32 FrameIndex = 0; FrameIndex <= TotalFrames.Value; ++FrameIndex)
    {
        FMemMark Mark(FMemStack::Get());

        FCompactPose FinalBlendedPose;
        FinalBlendedPose.SetBoneContainer(&BoneContainer);
        FBlendedCurve FinalBlendedPoseCurve;
        FinalBlendedPoseCurve.InitFrom(BoneContainer);
        UE::Anim::FStackAttributeContainer AttributesFinal;
        FAnimationPoseData FinalPoseData(FinalBlendedPose, FinalBlendedPoseCurve, AttributesFinal);

        const double CurrentTime = FrameRate.AsSeconds(FFrameNumber(FrameIndex));

        if (CurrentTime < StartLength)
        {
            CloEditorUtils::GetPoseAtTime(StartAnim, CurrentTime, FinalPoseData);
        }
        else if (CurrentTime <= StartLength + InTransitionDuration)
        {
            FCompactPose PoseA, PoseB;
            PoseA.SetBoneContainer(&BoneContainer);
            PoseB.SetBoneContainer(&BoneContainer);
            FBlendedCurve CurveA, CurveB;
            CurveA.InitFrom(BoneContainer);
            CurveB.InitFrom(BoneContainer);
            UE::Anim::FStackAttributeContainer AttributesA, AttributesB;
            FAnimationPoseData PoseDataA(PoseA, CurveA, AttributesA);
            FAnimationPoseData PoseDataB(PoseB, CurveB, AttributesB);

            CloEditorUtils::GetPoseAtTime(StartAnim, StartLength, PoseDataA);
            CloEditorUtils::GetPoseAtTime(EndAnim, 0.0, PoseDataB);

            const float BlendAlpha = (InTransitionDuration > 0.f) ? ((CurrentTime - StartLength) / InTransitionDuration) : 1.f;

            FAnimationRuntime::BlendTwoPosesTogether(PoseDataA, PoseDataB, 1.f - BlendAlpha, FinalPoseData);
        }
        else
        {
            const double TimeInEndAnim = CurrentTime - StartLength - InTransitionDuration;
            CloEditorUtils::GetPoseAtTime(EndAnim, TimeInEndAnim, FinalPoseData);
        }

        for (FCompactPoseBoneIndex BoneIndex : FinalBlendedPose.ForEachBoneIndex())
        {
            const FTransform& BoneTransform = FinalBlendedPose[BoneIndex];
            AllPositionKeys[BoneIndex.GetInt()].Add(static_cast<FVector3f>(BoneTransform.GetLocation()));
            AllRotationKeys[BoneIndex.GetInt()].Add(static_cast<FQuat4f>(BoneTransform.GetRotation()));
            AllScaleKeys[BoneIndex.GetInt()].Add(static_cast<FVector3f>(BoneTransform.GetScale3D()));
        }

        for (const FName& CurveName : SkeletonCurveNames)
        {
            const float CurveValue = FinalBlendedPoseCurve.Get(CurveName);
            if (!FMath::IsNearlyZero(CurveValue))
            {
                AllCurveKeys.FindOrAdd(CurveName).Emplace(CurrentTime, CurveValue);
            }
        }
    }

    for (FCompactPoseBoneIndex BoneIndex : BoneContainer.ForEachCompactPoseBoneIndex())
    {
        const int32 SkeletonBoneIndex = BoneContainer.GetSkeletonIndex(BoneIndex);
        if (SkeletonBoneIndex != INDEX_NONE)
        {
            const FName BoneName = RefSkeleton.GetBoneName(SkeletonBoneIndex);
            Controller.AddBoneCurve(BoneName);
            Controller.SetBoneTrackKeys(BoneName, AllPositionKeys[BoneIndex.GetInt()], AllRotationKeys[BoneIndex.GetInt()], AllScaleKeys[BoneIndex.GetInt()]);
        }
    }

    const TScriptInterface<const IAnimationDataModel> Model = Controller.GetModelInterface();
    for (const TPair<FName, TArray<FRichCurveKey>>& CurvePair : AllCurveKeys)
    {
        const FAnimationCurveIdentifier CurveId(CurvePair.Key, ERawCurveTrackTypes::RCT_Float);

        if (Model->FindFloatCurve(CurveId) == nullptr)
        {
            Controller.AddCurve(CurveId, EAnimAssetCurveFlags::AACF_Editable);
        }

        Controller.SetCurveKeys(CurveId, CurvePair.Value);
    }

    Controller.NotifyPopulated();
    Controller.CloseBracket();

    NewAnimSequence->PostEditChange();
    NewAnimSequence->MarkPackageDirty();

    FAssetRegistryModule::GetRegistry().AssetCreated(NewAnimSequence);

    return NewAnimSequence;
}

#undef LOCTEXT_NAMESPACE