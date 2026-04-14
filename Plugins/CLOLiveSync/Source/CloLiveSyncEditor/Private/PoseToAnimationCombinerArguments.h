// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PoseToAnimationCombinerArguments.generated.h"

class UAnimationAsset;

UCLASS()
class UPoseToAnimationCombinerArguments : public UObject
{
    GENERATED_BODY()

public:
    UPoseToAnimationCombinerArguments();

    UPROPERTY(EditAnywhere, Category = "Input Animations", meta = (AllowedClasses = "/Script/Engine.PoseAsset"))
    TObjectPtr<UAnimationAsset> StartPose;

    UPROPERTY(EditAnywhere, Category = "Input Animations", meta = (AllowedClasses = "/Script/Engine.PoseAsset, /Script/Engine.AnimSequence"))
    TObjectPtr<UAnimationAsset> TargetAnimation;

    UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = "0.0"))
    float TransitionDuration;

    UPROPERTY(VisibleAnywhere, Category = "Output")
    FString OutputPath;
};