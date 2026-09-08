/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomChannelLevelMatrixCustomization.cpp
 *
 ****************************************************************************/

#include "AtomChannelLevelMatrixCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailPropertyRow.h"
#include "IDetailGroup.h"
#include "DetailCategoryBuilder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SNumericEntryBox.h"

#include "CriWare.h"
#include "Atom/AtomRuntime.h"
#include "Atom/Effects/AtomBusEffectMatrix.h"

#define LOCTEXT_NAMESPACE	"AtomChannelLevelMatrixDetailsInspector"

FAtomChannelLevelMatrix* GetLevelMatrixFromPropertyHandle(TSharedPtr<IPropertyHandle> Handle)
{
	TArray<void*> RawData;
	Handle->AccessRawData(RawData);
	return RawData.Num() == 1 ? static_cast<FAtomChannelLevelMatrix*>(RawData[0]) : nullptr;
}

TSharedRef<IPropertyTypeCustomization> FAtomChannelLevelMatrixCustomization::MakeInstance()
{
	return MakeShareable(new FAtomChannelLevelMatrixCustomization);
}

void FAtomChannelLevelMatrixCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	];
}

void FAtomChannelLevelMatrixCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& DetailChildren, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	ProxyProperty = StructPropertyHandle;
	if (ProxyProperty.IsValid())
	{
		uint32 c;
		ProxyProperty->GetNumChildren(c);

		LevelMatrixProperty = ProxyProperty->GetChildHandle(TEXT("LevelMatrix"))->AsArray();

		// setup category
		//IDetailCategoryBuilder& LevelMixerCategory = DetailChildren.EditCategory("Level Mixer", LOCTEXT("AtomLevelMixerCategoryLabel", "Level Mixer"));

		// customize
		DetailChildren.AddCustomRow(FText::GetEmpty())
			.NameContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AtomLevelMatrixPropertyLabel", "Channel Level Matrix"))
				.ToolTipText(LOCTEXT("AtomLevelMatrixPropertyTooltip", "The levels to apply for each input channels to output channels."))
				.Font(StructCustomizationUtils.GetRegularFont())
			]
			.ValueContent()
			[
				GenerateLevelMatrixWidget()
			];
	}
}

TSharedRef<SWidget> FAtomChannelLevelMatrixCustomization::GenerateLevelMatrixWidget()
{
	auto LevelMatrix = GetLevelMatrixFromPropertyHandle(ProxyProperty);

	if (LevelMatrix)
	{
		int NumRow = LevelMatrix->GetNumInputChannels();
		int NumColumn = LevelMatrix->GetNumOutputChannels();

		auto GridPanel = SNew(SUniformGridPanel)
			.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
			.MinDesiredSlotWidth(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotWidth"))
			.MinDesiredSlotHeight(FAppStyle::GetFloat("StandardDialog.MinDesiredSlotHeight"));

		// column headers
		for (int Column = 0; Column < NumColumn; ++Column)
		{
			EAtomSpeaker OutCh = (EAtomSpeaker)Column;

			GridPanel->AddSlot(Column + 1, 0) // x, y
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					//.Font(DetailFontInfo)
					.Text(FText::FromString(ToAbbreviatedString(OutCh)))
				];
		}

		for (int Row = 0; Row < NumRow; ++Row)
		{ 
			EAtomSpeaker InCh = (EAtomSpeaker)Row;

			// row header
			GridPanel->AddSlot(0, Row + 1) // x, y
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					//.Font(DetailFontInfo)
					.Text(FText::FromString(ToAbbreviatedString(InCh)))
				];

			// Row values
			for (int Column = 0; Column < NumColumn; ++Column)
			{
				EAtomSpeaker OutCh = (EAtomSpeaker)Column;

				GridPanel->AddSlot(Column + 1, Row + 1) // x, y
					[
						SNew(SNumericEntryBox<float>)
						.Value(this, &FAtomChannelLevelMatrixCustomization::GetAudioIntensity, InCh, OutCh)
						.AllowSpin(true)
						.MinValue(0.0f)
						.MaxValue(1.0f)
						.MinSliderValue(0.0f)
						.MaxSliderValue(1.0f)
						.OnValueCommitted(this, &FAtomChannelLevelMatrixCustomization::OnSetAudioIntensity, InCh, OutCh)
						.OnValueChanged(this, &FAtomChannelLevelMatrixCustomization::OnSetAudioIntensity, ETextCommit::Default, InCh, OutCh)
					];
			}
		}

		return GridPanel;
	}

	return SNullWidget::NullWidget;
}

TOptional<float> FAtomChannelLevelMatrixCustomization::GetAudioIntensity(EAtomSpeaker InCh, EAtomSpeaker OutCh) const
{
	auto LevelMatrix = GetLevelMatrixFromPropertyHandle(ProxyProperty);
	if (LevelMatrix)
	{
		return LevelMatrix->GetChannelLevel(InCh, OutCh);
	}

	return TOptional<float>();
}

void FAtomChannelLevelMatrixCustomization::OnSetAudioIntensity(float NewValue, ETextCommit::Type CommitInfo, EAtomSpeaker InCh, EAtomSpeaker OutCh)
{
	auto LevelMatrix = GetLevelMatrixFromPropertyHandle(ProxyProperty);
	if (LevelMatrix && LevelMatrixProperty.IsValid())
	{
		// Set thru property handle to mark dirty and serialize changes
		const int NumInChs = LevelMatrix->GetNumInputChannels();
		const int NumOutChs = LevelMatrix->GetNumOutputChannels();

		if ((int)InCh < NumInChs && (int)OutCh < NumOutChs)
		{
			const uint32 Index = (int)InCh * NumOutChs + (int)OutCh;

			uint32 NumElements = 0;
			FPropertyAccess::Result Result = LevelMatrixProperty->GetNumElements(NumElements);
			if (Result == FPropertyAccess::Success && Index < NumElements)
			{
				LevelMatrixProperty->GetElement(Index)->SetValue(NewValue);
			}
		}

		TArray<UObject*> OuterObjects;
		ProxyProperty->GetOuterObjects(OuterObjects);

		if (OuterObjects.Num() == 1)
		{
			if (const UAtomBus* Bus = Cast<const UAtomBus>(OuterObjects[0]))
			{
				// If it's a bus, Immediatly apply level matrix modification to Atom.
				if (GCriWare)
				{
					if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime())
					{
						AtomRuntime->SetBusChannelLevelMatrix(Bus);
					}
				}
			}
			else if (UAtomBusEffectMatrixPreset* MatrixPreset = Cast<UAtomBusEffectMatrixPreset>(OuterObjects[0]))
			{
				// If it's an Effect, Immediatly apply channel level matrix modification to preset.
				MatrixPreset->SetChannelLevel(InCh, OutCh, NewValue);
			}
		}
	}
}

/*
void FAtomChannelLevelMatrixCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Make sure we actually get a valid struct before continuing
	TArray<TSharedPtr<FStructOnScope>> Structs;
	DetailLayout.GetStructsBeingCustomized(Structs);

	if (Structs.Num() == 0)
	{
		// Nothing being customized
		return;
	}

	const UStruct* Struct = Structs[0]->GetStruct();
	if (!Struct || Struct != FAtomChannelLevelMatrix::StaticStruct())
	{
		// Invalid struct
		return;
	}

	// Get ptr to our actual type
	FAtomChannelLevelMatrix* LevelMatrix = (FAtomChannelLevelMatrix*)Structs[0]->GetStructMemory();

	// setup category
	IDetailCategoryBuilder& LevelMixerCategory = DetailLayout.EditCategory("Level Mixer", LOCTEXT("AtomLevelMixerCategoryLabel", "Level Mixer"));

	// customize
	LevelMixerCategory.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("AtomLevelMatrixPropertyLabel", "Channel Level Matrix"))
			.ToolTipText(LOCTEXT("AtomLevelMatrixPropertyTooltip", "The levels to apply for each input channels to output channels."))
			.Font(StructCustomizationUtils.GetRegularFont())
		]
		.ValueContent()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(FMargin(2.0f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Left", "Left"))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(FMargin(2.0f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Right", "Right"))
				]
			]
			+ SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(FMargin(2.0f))
				[
					SNumericEntryBox<float>)
					.AllowSpin(true)
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(FMargin(2.0f))
				[
					SNumericEntryBox<float>)
					.AllowSpin(true)
				]
			]
		];

}
*/

#undef LOCTEXT_NAMESPACE
