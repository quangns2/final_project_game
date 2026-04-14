// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#include "PoseToAnimationCombinerArgumentsDetails.h"
#include "PoseToAnimationCombinerArguments.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCategoryBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "Dialogs/DlgPickPath.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"

#define LOCTEXT_NAMESPACE "AnimationSequenceCombinerDetails"

TSharedRef<IDetailCustomization> FPoseToAnimationCombinerArgumentsDetails::MakeInstance()
{
    return MakeShareable(new FPoseToAnimationCombinerArgumentsDetails);
}

void FPoseToAnimationCombinerArgumentsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TSharedPtr<IPropertyHandle> OutputPathHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPoseToAnimationCombinerArguments, OutputPath));

    DetailBuilder.HideProperty(OutputPathHandle);

    IDetailCategoryBuilder& OutputCategory = DetailBuilder.EditCategory("Output");

    OutputCategory.AddCustomRow(LOCTEXT("OutputPathRow", "Output Path"))
        .NameContent()
        [
            OutputPathHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        .MinDesiredWidth(250.f)
        [
            SNew(STextBlock)
            .Text_Lambda([OutputPathHandle]() -> FText
            {
	            FString PathValue;
	            OutputPathHandle->GetValue(PathValue);
				return FText::FromString(PathValue);
            })
        ]
        .ExtensionContent()
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .OnClicked_Lambda([OutputPathHandle]() -> FReply
            {
	            TSharedRef<SDlgPickPath> PickPathDialog = SNew(SDlgPickPath)
	            .Title(LOCTEXT("PickPathDialogTitle", "Choose Animation Save Location"));

	            if (PickPathDialog->ShowModal() == EAppReturnType::Ok)
	            {
	                OutputPathHandle->SetValue(PickPathDialog->GetPath().ToString());
	            }
	            return FReply::Handled();
            })
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush("Icons.FolderOpen"))
                .ColorAndOpacity(FSlateColor::UseForeground())
            ]
        ]
        .ResetToDefaultContent()
        [
            SNew(SButton)
            .OnClicked_Lambda([OutputPathHandle]() -> FReply
            {
	            if (OutputPathHandle.IsValid())
	            {
	                if (const UPoseToAnimationCombinerArguments* DefaultObject = GetDefault<UPoseToAnimationCombinerArguments>())
	                {
	                    OutputPathHandle->SetValue(DefaultObject->OutputPath);
	                }
	            }
	            return FReply::Handled();
            })
            .Visibility_Lambda([OutputPathHandle]() -> EVisibility
            {
	            if (OutputPathHandle.IsValid() && OutputPathHandle->DiffersFromDefault())
	            {
	                return EVisibility::Visible;
	            }
	            return EVisibility::Collapsed;
            })
            .ContentPadding(0)
            .ToolTipText(NSLOCTEXT("PropertyEditor", "ResetToDefaultPropertyValueToolTip", "Reset this property to its default value."))
            .ButtonStyle(FAppStyle::Get(), "NoBorder")
            [
                SNew(SImage)
                .Image(FAppStyle::GetBrush("PropertyWindow.DiffersFromDefault"))
            ]
        ];
}

#undef LOCTEXT_NAMESPACE