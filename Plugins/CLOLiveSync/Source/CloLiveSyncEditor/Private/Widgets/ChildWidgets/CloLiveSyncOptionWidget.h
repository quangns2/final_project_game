// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SCompoundWidget.h"

#include "Communication/LiveSyncProtocol.h"
#include "Settings/CloLiveSyncOptionUISettings.h"

class FCloLiveSyncModeToolkit;
class FUICommandList;
class UCloLiveSyncOptionUISettings;

class SCloLiveSyncOptionWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCloLiveSyncOptionWidget) {}
    SLATE_END_ARGS()

    /** SCompoundWidget functions */
    void Construct(const FArguments& InArgs, TSharedRef<FCloLiveSyncModeToolkit> LiveSyncModeToolkit);

    CloLiveSync::USDOptions GetUSDLiveSyncOptions() const;

private:
    void RefreshWidget();

    // Make widget
    TSharedRef<SWidget> MakeNetworkStatusWidget();
    TSharedRef<SWidget> MakeAvatarPrimPathGroupWidget();
    TSharedRef<SWidget> MakeGarmentPrimPathGroupWidget();
    TSharedRef<SWidget> MakeSceneNPropPrimPathGroupWidget();
    TSharedRef<SWidget> MakePrimPathOptionWidget();
    TSharedRef<SWidget> MakePrimPathWidgetBody();
    TSharedRef<SWidget> MakeObjectModeRadioButtonWidget();
    TSharedRef<SWidget> MakeThinTickRadioButtonWidget();
    TSharedRef<SWidget> MakeTransparentBlendModeRadioButtonWidget();
    TSharedRef<SWidget> MakeGarmentOptionWidget();
    TSharedRef<SWidget> MakeNormalTextureOptionWidget();
    TSharedRef<SWidget> MakGarmentWidgetBody();
    TSharedRef<SWidget> MakeAvatarAnimationRadioButtonWidget();
    TSharedRef<SWidget> MakeAvatarOptionWidget();
    TSharedRef<SWidget> MakeAvatarWidgetBody();
    TSharedRef<SWidget> MakeTransparentBlendModeTypeWidgetBody();
    TSharedRef<SWidget> MakeNormalTextureOptionWidgetBody();
    TSharedRef<SWidget> MakeSceneAndPropsWidgetBody();
    TSharedRef<SWidget> MakeAnimationRegionRadioButtonWidget();
    TSharedRef<SWidget> MakeAnimationOptionWidget();
    TSharedRef<SWidget> MakeAnimationWidgetBody();

    // Widget visibility
    EVisibility GetAvatarPrimPathGroupWidgetVisibility() const;
    EVisibility GetGarmentPrimPathGroupWidgetVisibility() const;
    EVisibility GetSceneNPropPrimPathGroupWidgetVisibility() const;
    EVisibility GetGarmentOptionWidgetVisibility() const;
    EVisibility GetAvatarAnimationRadioWidgetVisibility() const;
    EVisibility GetAvatarOptionWidgetVisibility() const;
    EVisibility GetIncludeSeamPuckeringNormalMapWidgetVisibility() const;

    // Text changed
    void OnAvatarPrimPathTextChanged(const FText& NewText);
    void OnGarmentPrimPathTextChanged(const FText& NewText);
    void OnSceneNPropPrimPathTextChanged(const FText& NewText);

    // CheckBox changed
    void OnAssignAvatarCheckBoxChanged(ECheckBoxState NewState);
    void OnAssignGarmentCheckBoxChanged(ECheckBoxState NewState);
    void OnAssignSceneNPropCheckBoxChanged(ECheckBoxState NewState);
    void OnIncludeGarmentCheckBoxChanged(ECheckBoxState NewState);
    void OnUnifiedUVCoordinatesCheckBoxChanged(ECheckBoxState NewState);
    void OnIncludeSeamPuckeringNormalMapCheckBoxChanged(ECheckBoxState NewState);
    void OnIncludeGarmentSimulationDataCheckBoxChanged(ECheckBoxState NewState);
    void OnIncludeCacheAnimationCheckBoxChanged(ECheckBoxState NewState);
    void OnIncludeAvatarCheckBoxChanged(ECheckBoxState NewState);
    void OnAvatarAnimationCheckBoxChanged(ECheckBoxState NewState);
    void OnIncludeSceneAndPropsCheckBoxChanged(ECheckBoxState NewState);
    void OnFlipNormalGreenChannelCheckBoxChanged(ECheckBoxState NewState);
    void OnBakeJointAnimFramesOnFPSMismatchCheckBoxChanged(ECheckBoxState NewState);

    // RadioButton changed
    void OnObjectModeRadioButtonChanged(ECheckBoxState NewState, EObjectModeRadioButtonState ButtonState);
    void OnThinThickRadioButtonChanged(ECheckBoxState NewState, EThinTickRadioButtonState ButtonState);
    void OnAvatarAnimationRadioButtonChanged(ECheckBoxState NewState, EAvatarAnimationRadioButtonState ButtonState);
    void OnTransparentBlendModeRadioButtonChanged(ECheckBoxState NewState, EBlendMode ButtonMode);
    void OnAnimationRegionRadioButtonChanged(ECheckBoxState NewState, EAnimationRegionRadioButtonState ButtonState);
    ECheckBoxState IsObjectModeRadioButtonChecked(EObjectModeRadioButtonState ButtonState) const;
    ECheckBoxState IsThinThickRadioButtonChecked(EThinTickRadioButtonState ButtonState) const;
    ECheckBoxState IsAvatarAnimationRadioButtonChecked(EAvatarAnimationRadioButtonState ButtonState) const;
    ECheckBoxState IsTransparentBlendModeRadioButtonChecked(EBlendMode ButtonMode) const;
    ECheckBoxState IsAnimationRegionRadioButtonChecked(EAnimationRegionRadioButtonState ButtonState) const;

    // Reset Button
    FReply OnResetToDefaultsButtonClicked();

    FSlateColor GetNetworkStatusImageColor() const;

private:
    TWeakPtr<FCloLiveSyncModeToolkit> LiveSyncModeToolkitWeakPtr;

    UCloLiveSyncOptionUISettings* LiveSyncSettings;
};