// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CloLiveSyncOptionUISettings.generated.h"

UENUM(BlueprintType)
enum class EObjectModeRadioButtonState : uint8
{
    SingleObject,
    MultipleObject
};

UENUM(BlueprintType)
enum class EThinTickRadioButtonState : uint8
{
    Thin,
    Thick
};

UENUM(BlueprintType)
enum class EAvatarAnimationRadioButtonState : uint8
{
    Joint,
    Cache
};

UENUM(BlueprintType)
enum class EAnimationRegionRadioButtonState : uint8
{
    PlayRegion,
    EntireRegion
};

UCLASS(config = EditorSettings)
class UCloLiveSyncOptionUISettings : public UObject
{
    GENERATED_BODY()

public:
    UCloLiveSyncOptionUISettings();
    void ResetToDefaults();

public:
    // Prim Path
    UPROPERTY(config)
    bool bAssignAvatar;

    UPROPERTY(config)
    FString AvatarPrimPath;

    UPROPERTY(config)
    bool bAssignGarment;

    UPROPERTY(config)
    FString GarmentPrimPath;

    UPROPERTY(config)
    bool bAssignSceneNProp;

    UPROPERTY(config)
    FString SceneNPropPrimPath;

    // Garment
    UPROPERTY(config)
    bool bIncludeGarment;

    UPROPERTY(config)
    EObjectModeRadioButtonState ObjectMode;

    UPROPERTY(config)
    EThinTickRadioButtonState ThinThickMode;

    UPROPERTY(config)
    bool bUnifiedUVCoordinates;

    UPROPERTY(config)
    bool bIncludeSimulationData;

    UPROPERTY(config)
    bool bIncludeGarmentCacheAnimation;

    UPROPERTY(config)
    bool bIncludeSeamPuckeringNormalMap;

    // Avatar
    UPROPERTY(config)
    bool bIncludeAvatar;

    UPROPERTY(config)
    bool bIncludeAvatarAnimation;

    UPROPERTY(config)
    EAvatarAnimationRadioButtonState AvatarAnimationType;

    // Scene and Props
    UPROPERTY(config)
    bool bIncludeSceneAndProps;

    // Animation
    UPROPERTY(config)
    EAnimationRegionRadioButtonState AnimationRegion;

    UPROPERTY(config)
    bool bBakeJointAnimFramesOnFPSMismatch;

    // Blend Mode
    UPROPERTY(config)
    TEnumAsByte<EBlendMode> TransparentBlendMode;

    UPROPERTY(config)
    bool bFlipNormalGreenChannel;
};