// Copyright 2023-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "CloLiveSyncExportOptions.generated.h"

class UDebugSkelMeshComponent;

UCLASS()
class UCloLiveSyncExportOptionBase : public UObject
{
    GENERATED_BODY()
};

UCLASS()
class UCloSkeletalMeshLiveSyncExportOption : public UCloLiveSyncExportOptionBase
{
    GENERATED_BODY()
public:
    virtual bool CanEditChange(const FProperty* InProperty) const override
    {
        FName propertyName = InProperty->GetFName();

        if (propertyName == GET_MEMBER_NAME_CHECKED(UCloSkeletalMeshLiveSyncExportOption, baseSkeleton))
        {
            return false;
        }
        else if (propertyName == GET_MEMBER_NAME_CHECKED(UCloSkeletalMeshLiveSyncExportOption, targetComponents))
        {
            return false;
        }
        else if (propertyName == GET_MEMBER_NAME_CHECKED(UCloSkeletalMeshLiveSyncExportOption, animationAsset))
        {
            return false;
        }
        else if (propertyName == GET_MEMBER_NAME_CHECKED(UCloSkeletalMeshLiveSyncExportOption, refSkeleton))
        {
            return false;
        }
        else if (propertyName == GET_MEMBER_NAME_CHECKED(UCloSkeletalMeshLiveSyncExportOption, previewSkelMeshComponent))
        {
            return false;
        }

        return true;
    }

public:
    UPROPERTY(EditAnywhere, Category = "Skeleton")
        USkeleton* refSkeleton;

    UPROPERTY(EditAnywhere, Category = "Skeleton")
        USkeleton* baseSkeleton;

    UPROPERTY(EditAnywhere, Category = "Mesh")
        TArray<USkeletalMeshComponent*> targetComponents;

    UPROPERTY(EditAnywhere, Category = "Mesh")
        UDebugSkelMeshComponent* previewSkelMeshComponent;

    UPROPERTY(EditAnywhere, Category = "Animation")
        UAnimationAsset* animationAsset;
};