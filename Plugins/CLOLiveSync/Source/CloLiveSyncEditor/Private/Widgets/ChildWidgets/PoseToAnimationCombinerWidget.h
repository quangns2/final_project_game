// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "PoseToAnimationCombinerArguments.h"
#include "Input/Reply.h"
#include "UObject/StrongObjectPtr.h"

class IDetailsView;
class UAnimSequence;
class FPoseToAnimationCombinerArgumentsDetails;

class SPoseToAnimationCombinerWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPoseToAnimationCombinerWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SPoseToAnimationCombinerWidget() override;

private:
    FReply OnCombineClicked();

    float GetAnimationAssetPlayLength(UAnimationAsset* AnimAsset);
    UAnimSequence* CombineAnimations(UAnimationAsset* StartPose, UAnimationAsset* TargetAnimation, float InTransitionDuration, const FString& InAssetPath, const FString& InAssetName);

private:
    TSharedPtr<IDetailsView> DetailsView;
    TStrongObjectPtr<UPoseToAnimationCombinerArguments> Arguments;
};