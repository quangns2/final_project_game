// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#include "CloLiveSyncOptionUISettings.h"

UCloLiveSyncOptionUISettings::UCloLiveSyncOptionUISettings()
{
    ResetToDefaults();
}

void UCloLiveSyncOptionUISettings::ResetToDefaults()
{
    bAssignAvatar = true;
    AvatarPrimPath = TEXT("/Avatar");
    bAssignGarment = true;
    GarmentPrimPath = TEXT("/Garment");
    bAssignSceneNProp = true;
    SceneNPropPrimPath = TEXT("/SceneAndProp");

    bIncludeGarment = true;
    ObjectMode = EObjectModeRadioButtonState::MultipleObject;
    ThinThickMode = EThinTickRadioButtonState::Thin;
    bUnifiedUVCoordinates = false;
    bIncludeSimulationData = false;
    bIncludeGarmentCacheAnimation = false;
    bIncludeSeamPuckeringNormalMap = false;

    bIncludeAvatar = true;
    bIncludeAvatarAnimation = false;
    AvatarAnimationType = EAvatarAnimationRadioButtonState::Joint;

    bIncludeSceneAndProps = false;

    AnimationRegion = EAnimationRegionRadioButtonState::PlayRegion;
    bBakeJointAnimFramesOnFPSMismatch = false;

    TransparentBlendMode = EBlendMode::BLEND_Translucent;
    bFlipNormalGreenChannel = true;
}