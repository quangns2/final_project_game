// Copyright 2024 CLO Virtual Fashion. All rights reserved.

#include "CloLiveSyncOptionWidget.h"

#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"

#include "Communication/LiveSyncProtocol.h"
#include "Mode/CloLiveSyncModeToolkit.h"
#include "Settings/CloLiveSyncOptionUISettings.h"
#include "Utils/CloLiveSyncCoreUtils.h"

#define LOCTEXT_NAMESPACE "CloLiveSyncWidget"

void SCloLiveSyncOptionWidget::Construct(const FArguments& InArgs, TSharedRef<FCloLiveSyncModeToolkit> LiveSyncModeToolkit)
{
    LiveSyncModeToolkitWeakPtr = LiveSyncModeToolkit;

    LiveSyncSettings = GetMutableDefault<UCloLiveSyncOptionUISettings>();

    FDetailsViewArgs detailsViewArgs;
    detailsViewArgs.bHideSelectionTip = true;
    detailsViewArgs.bAllowSearch = false;

    RefreshWidget();
}

CloLiveSync::USDOptions SCloLiveSyncOptionWidget::GetUSDLiveSyncOptions() const
{
    CloLiveSync::USDOptions Options;

    Options.bAssignAvatar = LiveSyncSettings->bAssignAvatar;
    Options.avatarPrimPath = TCHAR_TO_UTF8(*(LiveSyncSettings->AvatarPrimPath));
    Options.bAssignGarment = LiveSyncSettings->bAssignGarment;
    Options.garmentPrimPath = TCHAR_TO_UTF8(*(LiveSyncSettings->GarmentPrimPath));
    Options.bAssignSceneNProp = LiveSyncSettings->bAssignSceneNProp;
    Options.sceneNPropPrimPath = TCHAR_TO_UTF8(*(LiveSyncSettings->SceneNPropPrimPath));

    Options.bIncludeGarment = LiveSyncSettings->bIncludeGarment;
    Options.bSingleObject = (LiveSyncSettings->ObjectMode == EObjectModeRadioButtonState::SingleObject);
    Options.bThin = (LiveSyncSettings->ThinThickMode == EThinTickRadioButtonState::Thin);
    Options.bUnifiedUVCoordinates = LiveSyncSettings->bUnifiedUVCoordinates;
    Options.bIncludeSimulationData = LiveSyncSettings->bIncludeSimulationData;
    Options.bIncludeGarmentCacheAnimation = LiveSyncSettings->bIncludeGarmentCacheAnimation;
    Options.bIncludeSeamPuckeringNormalMap = LiveSyncSettings->bIncludeSeamPuckeringNormalMap;
    Options.bIncludeAvatar = LiveSyncSettings->bIncludeAvatar;
    Options.bIncludeAvatarAnimation = LiveSyncSettings->bIncludeAvatarAnimation;
    Options.avatarAnimationType = (LiveSyncSettings->AvatarAnimationType == EAvatarAnimationRadioButtonState::Joint) ? CloLiveSync::AvatarAnimationType::Joint : CloLiveSync::AvatarAnimationType::Cache;
    Options.bIncludeSceneAndProps = LiveSyncSettings->bIncludeSceneAndProps;
    Options.animationRegion = (LiveSyncSettings->AnimationRegion == EAnimationRegionRadioButtonState::PlayRegion) ? CloLiveSync::AnimationRegion::PlayRegion : CloLiveSync::AnimationRegion::EntireRegion;
    Options.bBakeJointAnimFramesOnFPSMismatch = LiveSyncSettings->bBakeJointAnimFramesOnFPSMismatch;

    return Options;
}

void SCloLiveSyncOptionWidget::RefreshWidget()
{
    ChildSlot
    [
        SNew(SVerticalBox)
        // Connection Status Section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            MakeNetworkStatusWidget()
        ]
        // PrimPath Section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SExpandableArea)
            .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
            .HeaderContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Prim Path"))
            ]
            .BodyContent()
            [
                MakePrimPathWidgetBody()
            ]
        ]

        // Garment Section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SExpandableArea)
            .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
            .HeaderContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Garment"))
            ]
            .BodyContent()
            [
                MakGarmentWidgetBody()
            ]
        ]
        // Avatar Section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SExpandableArea)
            .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
            .HeaderContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Avatar"))
            ]
            .BodyContent()
            [
                MakeAvatarWidgetBody()
            ]
        ]
        // Scene and Props Section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SExpandableArea)
            .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
            .HeaderContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Scene and Props"))
            ]
            .BodyContent()
            [
                MakeSceneAndPropsWidgetBody()
            ]
        ]
        // Animation Section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SExpandableArea)
            .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
            .HeaderContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Animation"))
            ]
            .BodyContent()
            [
                MakeAnimationWidgetBody()
            ]
        ]
        // Override Transparent BlendMode Section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SExpandableArea)
            .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
            .HeaderContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Override Transparent Blend Mode"))
            ]
            .BodyContent()
            [
                MakeTransparentBlendModeTypeWidgetBody()
            ]
        ]
        // Override Normal Texture Option Section
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SExpandableArea)
            .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
            .HeaderContent()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Override Normal Texture Option"))
            ]
            .BodyContent()
            [
                MakeNormalTextureOptionWidgetBody()
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(10.0f, 20.0f, 10.0f, 10.0f)
        .HAlign(HAlign_Right)
        [
            SNew(SButton)
            .Text(LOCTEXT("ResetToDefaults", "Reset to Defaults"))
            .ToolTipText(LOCTEXT("ResetToDefaults_Tooltip", "Resets all options in this panel to their default values."))
            .OnClicked(this, &SCloLiveSyncOptionWidget::OnResetToDefaultsButtonClicked)
        ]
    ];
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeNetworkStatusWidget()
{
    auto Widget = SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(FText::FromString("LiveSync Connection Status : "))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SImage)
                .ColorAndOpacity_Raw(this, &SCloLiveSyncOptionWidget::GetNetworkStatusImageColor)
                .Image(FAppStyle::Get().GetBrush(TEXT("Icons.FilledCircle")))
            ]
        ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeAvatarPrimPathGroupWidget()
{
    auto Widget = SNew(SBorder)
        .Visibility(this, &SCloLiveSyncOptionWidget::GetAvatarPrimPathGroupWidgetVisibility)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Prim Path : "))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBox)
                .WidthOverride(200.f)
                [
                    SNew(SEditableTextBox)
                    .Text(FText::FromString(LiveSyncSettings->AvatarPrimPath))
                    .OnTextChanged(this, &SCloLiveSyncOptionWidget::OnAvatarPrimPathTextChanged)
                ]
            ]
        ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeGarmentPrimPathGroupWidget()
{
    auto Widget = SNew(SBorder)
        .Visibility(this, &SCloLiveSyncOptionWidget::GetGarmentPrimPathGroupWidgetVisibility)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Prim Path : "))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBox)
                .WidthOverride(200.f)
                [
                    SNew(SEditableTextBox)
                    .Text(FText::FromString(LiveSyncSettings->GarmentPrimPath))
                    .OnTextChanged(this, &SCloLiveSyncOptionWidget::OnGarmentPrimPathTextChanged)
                ]
            ]
        ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeSceneNPropPrimPathGroupWidget()
{
    auto Widget = SNew(SBorder)
        .Visibility(this, &SCloLiveSyncOptionWidget::GetSceneNPropPrimPathGroupWidgetVisibility)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(STextBlock)
                .Text(FText::FromString("Prim Path : "))
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SBox)
                .WidthOverride(200.f)
                [
                    SNew(SEditableTextBox)
                    .Text(FText::FromString(LiveSyncSettings->SceneNPropPrimPath))
                    .OnTextChanged(this, &SCloLiveSyncOptionWidget::OnSceneNPropPrimPathTextChanged)
                ]
            ]
        ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakePrimPathOptionWidget()
{
    auto BodyWidget = SNew(SVerticalBox)
        // Avatar Prim Path
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SCheckBox)
            .IsChecked(LiveSyncSettings->bAssignAvatar ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnAssignAvatarCheckBoxChanged)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Avatar"))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            MakeAvatarPrimPathGroupWidget()
        ]
        // Garment Prim Path
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SCheckBox)
            .IsChecked(LiveSyncSettings->bAssignGarment ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnAssignGarmentCheckBoxChanged)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Garment"))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            MakeGarmentPrimPathGroupWidget()
        ]
        // Scene and Prop Prim Path
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SCheckBox)
            .IsChecked(LiveSyncSettings->bAssignSceneNProp ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnAssignSceneNPropCheckBoxChanged)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Scene and Props"))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            MakeSceneNPropPrimPathGroupWidget()
        ];

    return BodyWidget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakePrimPathWidgetBody()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        MakePrimPathOptionWidget()
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeObjectModeRadioButtonWidget()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        SNew(SUniformGridPanel)
        .SlotPadding(4.0f)
        .MinDesiredSlotWidth(100.0f)
        .MinDesiredSlotHeight(24.0f)

        + SUniformGridPanel::Slot(0, 0)
        [
            SNew(SCheckBox)
            .Style(&FAppStyle::GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnObjectModeRadioButtonChanged, EObjectModeRadioButtonState::SingleObject)
            .IsChecked(this, &SCloLiveSyncOptionWidget::IsObjectModeRadioButtonChecked, EObjectModeRadioButtonState::SingleObject)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Single Object"))
            ]
        ]

        + SUniformGridPanel::Slot(1, 0)
        [
            SNew(SCheckBox)
            .Style(&FAppStyle::GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnObjectModeRadioButtonChanged, EObjectModeRadioButtonState::MultipleObject)
            .IsChecked(this, &SCloLiveSyncOptionWidget::IsObjectModeRadioButtonChecked, EObjectModeRadioButtonState::MultipleObject)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Multiple Objects"))
            ]
        ]
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeThinTickRadioButtonWidget()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        SNew(SUniformGridPanel)
        .SlotPadding(4.0f)
        .MinDesiredSlotWidth(100.0f)
        .MinDesiredSlotHeight(24.0f)

        + SUniformGridPanel::Slot(0, 0)
        [
            SNew(SCheckBox)
            .Style(&FAppStyle::GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnThinThickRadioButtonChanged, EThinTickRadioButtonState::Thin)
            .IsChecked(this, &SCloLiveSyncOptionWidget::IsThinThickRadioButtonChecked, EThinTickRadioButtonState::Thin)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Thin"))
            ]
        ]

        + SUniformGridPanel::Slot(1, 0)
        [
            SNew(SCheckBox)
            .Style(&FAppStyle::GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnThinThickRadioButtonChanged, EThinTickRadioButtonState::Thick)
            .IsChecked(this, &SCloLiveSyncOptionWidget::IsThinThickRadioButtonChecked, EThinTickRadioButtonState::Thick)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Thick"))
            ]
        ]
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeTransparentBlendModeRadioButtonWidget()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        SNew(SUniformGridPanel)
        .SlotPadding(4.0f)
        .MinDesiredSlotWidth(100.0f)
        .MinDesiredSlotHeight(24.0f)

        + SUniformGridPanel::Slot(0, 0)
        [
            SNew(SCheckBox)
            .Style(&FAppStyle::GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnTransparentBlendModeRadioButtonChanged, EBlendMode::BLEND_Translucent)
            .IsChecked(this, &SCloLiveSyncOptionWidget::IsTransparentBlendModeRadioButtonChecked, EBlendMode::BLEND_Translucent)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Translucent"))
            ]
        ]

        + SUniformGridPanel::Slot(1, 0)
        [
            SNew(SCheckBox)
            .Style(&FAppStyle::GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnTransparentBlendModeRadioButtonChanged, EBlendMode::BLEND_Masked)
            .IsChecked(this, &SCloLiveSyncOptionWidget::IsTransparentBlendModeRadioButtonChecked, EBlendMode::BLEND_Masked)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Opacity Mask"))
            ]
        ]
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeGarmentOptionWidget()
{
    auto BodyWidget = SNew(SVerticalBox)
// Single/Multiple Object Option
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        MakeObjectModeRadioButtonWidget()
    ]

    // Thin/Thick Option
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        MakeThinTickRadioButtonWidget()
    ]

    // Unified UV Coordinates
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        SNew(SCheckBox)
        .IsChecked(LiveSyncSettings->bUnifiedUVCoordinates ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
        .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnUnifiedUVCoordinatesCheckBoxChanged)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Unified UV Coordinates"))
        ]
    ]

    // Include SeamPuckering NormalMap
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        SNew(SCheckBox)
        .Visibility(this, &SCloLiveSyncOptionWidget::GetIncludeSeamPuckeringNormalMapWidgetVisibility)
        .IsChecked(LiveSyncSettings->bIncludeSeamPuckeringNormalMap ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
        .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnIncludeSeamPuckeringNormalMapCheckBoxChanged)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Include SeamPuckering NormalMap"))
        ]
    ]


    // Include Garment Simulation Data
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        SNew(SCheckBox)
        .IsChecked(LiveSyncSettings->bIncludeSimulationData ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
        .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnIncludeGarmentSimulationDataCheckBoxChanged)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Include Garment Simulation Data"))
        ]
    ]
    // Include Cache Animation
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        SNew(SCheckBox)
        .IsChecked(LiveSyncSettings->bIncludeGarmentCacheAnimation ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
        .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnIncludeCacheAnimationCheckBoxChanged)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Include Cache Animation"))
        ]
    ];

    return BodyWidget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeNormalTextureOptionWidget()
{
	auto BodyWidget = SNew(SVerticalBox)
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        SNew(SCheckBox)
        .IsChecked(LiveSyncSettings->bFlipNormalGreenChannel ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
        .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnFlipNormalGreenChannelCheckBoxChanged)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Flip Green Channel"))
        ]
    ];

    return BodyWidget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakGarmentWidgetBody()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SCheckBox)
            .IsChecked(LiveSyncSettings->bIncludeGarment ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnIncludeGarmentCheckBoxChanged)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Include Garment"))
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(4.0f)
        [
            SNew(SBorder)
            .Visibility(this, &SCloLiveSyncOptionWidget::GetGarmentOptionWidgetVisibility)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(4.0f)
            [
                MakeGarmentOptionWidget()
            ]
        ]
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeAvatarAnimationRadioButtonWidget()
{
    auto Widget = SNew(SBorder)
    .Visibility(this, &SCloLiveSyncOptionWidget::GetAvatarAnimationRadioWidgetVisibility)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        SNew(SUniformGridPanel)
        .SlotPadding(4.0f)
        .MinDesiredSlotWidth(100.0f)
        .MinDesiredSlotHeight(24.0f)

        + SUniformGridPanel::Slot(0, 0)
        [
            SNew(SCheckBox)
            .Style(&FAppStyle::GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnAvatarAnimationRadioButtonChanged, EAvatarAnimationRadioButtonState::Joint)
            .IsChecked(this, &SCloLiveSyncOptionWidget::IsAvatarAnimationRadioButtonChecked, EAvatarAnimationRadioButtonState::Joint)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Joint"))
            ]
        ]

        + SUniformGridPanel::Slot(1, 0)
        [
            SNew(SCheckBox)
            .Style(&FAppStyle::GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnAvatarAnimationRadioButtonChanged, EAvatarAnimationRadioButtonState::Cache)
            .IsChecked(this, &SCloLiveSyncOptionWidget::IsAvatarAnimationRadioButtonChecked, EAvatarAnimationRadioButtonState::Cache)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Cache"))
            ]
        ]
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeAvatarOptionWidget()
{
    auto BodyWidget = SNew(SVerticalBox)
    +SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        SNew(SCheckBox)
        .IsChecked(LiveSyncSettings->bIncludeAvatarAnimation ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
        .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnAvatarAnimationCheckBoxChanged)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Avatar Animation"))
        ]
    ]
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        MakeAvatarAnimationRadioButtonWidget()
    ];

    return BodyWidget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeAvatarWidgetBody()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        SNew(SVerticalBox)
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        SNew(SCheckBox)
        .IsChecked(LiveSyncSettings->bIncludeAvatar ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
        .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnIncludeAvatarCheckBoxChanged)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Include Avatar"))
        ]
    ]
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        SNew(SBorder)
        .Visibility(this, &SCloLiveSyncOptionWidget::GetAvatarOptionWidgetVisibility)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            MakeAvatarOptionWidget()
        ]
    ]
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeTransparentBlendModeTypeWidgetBody()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        MakeTransparentBlendModeRadioButtonWidget()
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeNormalTextureOptionWidgetBody()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        MakeNormalTextureOptionWidget()
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeSceneAndPropsWidgetBody()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        SNew(SVerticalBox)
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        SNew(SCheckBox)
        .IsChecked(LiveSyncSettings->bIncludeSceneAndProps ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
        .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnIncludeSceneAndPropsCheckBoxChanged)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Include Scene and Props"))
        ]
    ]
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeAnimationRegionRadioButtonWidget()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        SNew(SUniformGridPanel)
        .SlotPadding(4.0f)
        .MinDesiredSlotWidth(100.0f)
        .MinDesiredSlotHeight(24.0f)

        + SUniformGridPanel::Slot(0, 0)
        [
            SNew(SCheckBox)
            .Style(&FAppStyle::GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnAnimationRegionRadioButtonChanged, EAnimationRegionRadioButtonState::PlayRegion)
            .IsChecked(this, &SCloLiveSyncOptionWidget::IsAnimationRegionRadioButtonChecked, EAnimationRegionRadioButtonState::PlayRegion)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Play Region"))
            ]
        ]

        + SUniformGridPanel::Slot(1, 0)
        [
            SNew(SCheckBox)
            .Style(&FAppStyle::GetWidgetStyle<FCheckBoxStyle>("RadioButton"))
            .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnAnimationRegionRadioButtonChanged, EAnimationRegionRadioButtonState::EntireRegion)
            .IsChecked(this, &SCloLiveSyncOptionWidget::IsAnimationRegionRadioButtonChecked, EAnimationRegionRadioButtonState::EntireRegion)
            [
                SNew(STextBlock)
                .Text(FText::FromString("Entire Region"))
            ]
        ]
    ];

    return Widget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeAnimationOptionWidget()
{
    auto BodyWidget = SNew(SVerticalBox)
    // Animation Region Radio Buttons
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        MakeAnimationRegionRadioButtonWidget()
    ]
    // Bake Joint Animation Frames On FPS Mismatch CheckBox
    + SVerticalBox::Slot()
    .AutoHeight()
    .Padding(4.0f)
    [
        SNew(SCheckBox)
        .IsChecked(LiveSyncSettings->bBakeJointAnimFramesOnFPSMismatch ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
        .OnCheckStateChanged(this, &SCloLiveSyncOptionWidget::OnBakeJointAnimFramesOnFPSMismatchCheckBoxChanged)
        [
            SNew(STextBlock)
            .Text(FText::FromString("Bake Joint Animation Frames On FPS Mismatch"))
        ]
    ];

    return BodyWidget;
}

TSharedRef<SWidget> SCloLiveSyncOptionWidget::MakeAnimationWidgetBody()
{
    auto Widget = SNew(SBorder)
    .BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
    .Padding(4.0f)
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
        .Padding(4.0f)
        [
            MakeAnimationOptionWidget()
        ]
    ];

    return Widget;
}

EVisibility SCloLiveSyncOptionWidget::GetAvatarPrimPathGroupWidgetVisibility() const
{
    return LiveSyncSettings->bAssignAvatar ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCloLiveSyncOptionWidget::GetGarmentPrimPathGroupWidgetVisibility() const
{
    return LiveSyncSettings->bAssignGarment ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCloLiveSyncOptionWidget::GetSceneNPropPrimPathGroupWidgetVisibility() const
{
    return LiveSyncSettings->bAssignSceneNProp ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCloLiveSyncOptionWidget::GetGarmentOptionWidgetVisibility() const
{
    return LiveSyncSettings->bIncludeGarment ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCloLiveSyncOptionWidget::GetAvatarAnimationRadioWidgetVisibility() const
{
    return LiveSyncSettings->bIncludeAvatarAnimation ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCloLiveSyncOptionWidget::GetAvatarOptionWidgetVisibility() const
{
    return LiveSyncSettings->bIncludeAvatar ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SCloLiveSyncOptionWidget::GetIncludeSeamPuckeringNormalMapWidgetVisibility() const
{
    return !LiveSyncSettings->bUnifiedUVCoordinates ? EVisibility::Visible : EVisibility::Collapsed;
}

void SCloLiveSyncOptionWidget::OnAvatarPrimPathTextChanged(const FText& NewText)
{
    LiveSyncSettings->AvatarPrimPath = NewText.ToString();
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnGarmentPrimPathTextChanged(const FText& NewText)
{
    LiveSyncSettings->GarmentPrimPath = NewText.ToString();
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnSceneNPropPrimPathTextChanged(const FText& NewText)
{
    LiveSyncSettings->SceneNPropPrimPath = NewText.ToString();
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnAssignAvatarCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bAssignAvatar = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnAssignGarmentCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bAssignGarment = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnAssignSceneNPropCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bAssignSceneNProp = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnIncludeGarmentCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bIncludeGarment = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnUnifiedUVCoordinatesCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bUnifiedUVCoordinates = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnIncludeSeamPuckeringNormalMapCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bIncludeSeamPuckeringNormalMap = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnIncludeGarmentSimulationDataCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bIncludeSimulationData = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnIncludeCacheAnimationCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bIncludeGarmentCacheAnimation = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnIncludeAvatarCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bIncludeAvatar = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnAvatarAnimationCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bIncludeAvatarAnimation = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnIncludeSceneAndPropsCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bIncludeSceneAndProps = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnFlipNormalGreenChannelCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bFlipNormalGreenChannel = (NewState == ECheckBoxState::Checked);
    CloCoreUtils::SetFlipNormalGreenChannel(LiveSyncSettings->bFlipNormalGreenChannel);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnBakeJointAnimFramesOnFPSMismatchCheckBoxChanged(ECheckBoxState NewState)
{
    LiveSyncSettings->bBakeJointAnimFramesOnFPSMismatch = (NewState == ECheckBoxState::Checked);
    LiveSyncSettings->SaveConfig();
}

void SCloLiveSyncOptionWidget::OnObjectModeRadioButtonChanged(ECheckBoxState NewState, EObjectModeRadioButtonState ButtonState)
{
    if (NewState == ECheckBoxState::Checked)
    {
        LiveSyncSettings->ObjectMode = ButtonState;
        LiveSyncSettings->SaveConfig();
    }
}

void SCloLiveSyncOptionWidget::OnThinThickRadioButtonChanged(ECheckBoxState NewState, EThinTickRadioButtonState ButtonState)
{
    if (NewState == ECheckBoxState::Checked)
    {
        LiveSyncSettings->ThinThickMode = ButtonState;
        LiveSyncSettings->SaveConfig();
    }
}

void SCloLiveSyncOptionWidget::OnAvatarAnimationRadioButtonChanged(ECheckBoxState NewState, EAvatarAnimationRadioButtonState ButtonState)
{
    if (NewState == ECheckBoxState::Checked)
    {
        LiveSyncSettings->AvatarAnimationType = ButtonState;
        LiveSyncSettings->SaveConfig();
    }
}

void SCloLiveSyncOptionWidget::OnTransparentBlendModeRadioButtonChanged(ECheckBoxState NewState, EBlendMode ButtonMode)
{
    if (NewState == ECheckBoxState::Checked)
    {
        CloCoreUtils::SetDefaultTransparentBlendMode(ButtonMode);
        LiveSyncSettings->TransparentBlendMode = ButtonMode;
        LiveSyncSettings->SaveConfig();
    }
}

void SCloLiveSyncOptionWidget::OnAnimationRegionRadioButtonChanged(ECheckBoxState NewState, EAnimationRegionRadioButtonState ButtonState)
{
    if (NewState == ECheckBoxState::Checked)
    {
        LiveSyncSettings->AnimationRegion = ButtonState;
        LiveSyncSettings->SaveConfig();
    }
}

ECheckBoxState SCloLiveSyncOptionWidget::IsThinThickRadioButtonChecked(EThinTickRadioButtonState ButtonState) const
{
    return (LiveSyncSettings->ThinThickMode == ButtonState) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SCloLiveSyncOptionWidget::IsObjectModeRadioButtonChecked(EObjectModeRadioButtonState ButtonState) const
{
    return (LiveSyncSettings->ObjectMode == ButtonState) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SCloLiveSyncOptionWidget::IsAvatarAnimationRadioButtonChecked(EAvatarAnimationRadioButtonState ButtonState) const
{
    return (LiveSyncSettings->AvatarAnimationType == ButtonState) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SCloLiveSyncOptionWidget::IsTransparentBlendModeRadioButtonChecked(EBlendMode ButtonMode) const
{
    return (LiveSyncSettings->TransparentBlendMode == ButtonMode) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

ECheckBoxState SCloLiveSyncOptionWidget::IsAnimationRegionRadioButtonChecked(EAnimationRegionRadioButtonState ButtonState) const
{
    return (LiveSyncSettings->AnimationRegion == ButtonState) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FReply SCloLiveSyncOptionWidget::OnResetToDefaultsButtonClicked()
{
    LiveSyncSettings->ResetToDefaults();
    LiveSyncSettings->SaveConfig();

    RefreshWidget();

    return FReply::Handled();
}

FSlateColor SCloLiveSyncOptionWidget::GetNetworkStatusImageColor() const
{
    FSlateColor resultColor = FLinearColor::Red;

    if (LiveSyncModeToolkitWeakPtr.IsValid())
    {
        if (LiveSyncModeToolkitWeakPtr.Pin()->IsConnected())
        {
            resultColor = FLinearColor::Green;
        }
        else
        {
            resultColor = FLinearColor::Red;
        }
    }

    return resultColor;
}

#undef LOCTEXT_NAMESPACE