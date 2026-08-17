/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomAmbientSoundDetails.cpp
 *
 ****************************************************************************/

#include "AtomAmbientSoundDetails.h"

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Delegates/Delegate.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Text.h"
#include "Layout/Margin.h"
#include "Misc/AssertionMacros.h"
#include "Misc/Attribute.h"
#include "SlotBase.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Templates/Casts.h"
#include "Textures/SlateIcon.h"
#include "Types/SlateEnums.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ObjectPtr.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "CriWareEditor.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomAmbientSound.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomSoundWave.h"

// Forward Definitions
class UAtomSoundBase;

#define LOCTEXT_NAMESPACE "AtomAmbientSoundDetails"

TSharedRef<IDetailCustomization> FAtomAmbientSoundDetails::MakeInstance()
{
	return MakeShareable(new FAtomAmbientSoundDetails);
}

void FAtomAmbientSoundDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailBuilder.GetSelectedObjects();

	for (int32 ObjectIndex = 0; !AmbientSound.IsValid() && ObjectIndex < SelectedObjects.Num(); ++ObjectIndex)
	{
		const TWeakObjectPtr<UObject>& CurrentObject = SelectedObjects[ObjectIndex];
		if (CurrentObject.IsValid())
		{
			AmbientSound = Cast<AAtomAmbientSound>(CurrentObject.Get());
		}
	}

	DetailBuilder.EditCategory("Sound", FText::GetEmpty(), ECategoryPriority::Important)
		.AddCustomRow(FText::GetEmpty())
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(0, 2.0f, 0, 0)
			.FillHeight(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SButton)
					.VAlign(VAlign_Center)
					.OnClicked(this, &FAtomAmbientSoundDetails::OnEditSoundClicked)
					.IsEnabled(this, &FAtomAmbientSoundDetails::IsEditSoundEnabled)
					.Text(LOCTEXT("EditAsset", "Edit"))
					.ToolTipText(LOCTEXT("EditAssetToolTip", "Edit this sound"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SButton)
					.VAlign(VAlign_Center)
					.OnClicked(this, &FAtomAmbientSoundDetails::OnPlaySoundClicked)
					.Text(LOCTEXT("PlaySound", "Play"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f, 0.0f)
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				[
					SNew(SButton)
					.VAlign(VAlign_Center)
					.OnClicked(this, &FAtomAmbientSoundDetails::OnStopSoundClicked)
					.Text(LOCTEXT("StopSound", "Stop"))
				]
			]
		];

	DetailBuilder.EditCategory("Attenuation", FText::GetEmpty(), ECategoryPriority::TypeSpecific);
	DetailBuilder.EditCategory("Modulation", FText::GetEmpty(), ECategoryPriority::TypeSpecific);
}

bool FAtomAmbientSoundDetails::IsEditSoundEnabled() const
{
	if (AmbientSound.IsValid())
	{
		UAtomSoundBase* Sound = AmbientSound.Get()->GetAtomComponent()->Sound;
		return Sound != nullptr;
	}
	return false;
}

FReply FAtomAmbientSoundDetails::OnEditSoundClicked()
{
	if (AmbientSound.IsValid())
	{
		if (UAtomSoundBase* Sound = AmbientSound.Get()->GetAtomComponent()->Sound)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Sound);
		}
	}

	return FReply::Handled();
}

FReply FAtomAmbientSoundDetails::OnPlaySoundClicked()
{
	if (AmbientSound.IsValid())
	{
		if (UAtomSoundBase* Sound = AmbientSound.Get()->GetAtomComponent()->Sound)
		{
			GCriWareEditor->PlayPreviewSound(Sound);
		}
	}

	return FReply::Handled();
}

FReply FAtomAmbientSoundDetails::OnStopSoundClicked()
{
	GCriWareEditor->ResetPreviewAtomComponent();

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
