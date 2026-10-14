/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomBusDetails.cpp
 *
 ****************************************************************************/

#include "AtomBusDetails.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyCustomizationHelpers.h"

#include "Atom/Atom.h"
#include "Atom/AtomBus.h"
#include "Atom/AtomBusEffectPreset.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomConfig.h"

#define LOCTEXT_NAMESPACE	"AtomBusDetailsInspector"

TSharedRef<IDetailCustomization> FAtomBusDetails::MakeInstance()
{
	return MakeShareable(new FAtomBusDetails);
}

void FAtomBusDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Add Bus information
	IDetailCategoryBuilder& BusCategoryBuilder = DetailLayout.EditCategory(TEXT("Bus"));
	TSharedPtr<IPropertyHandle> BusNameHandle = DetailLayout.GetProperty(TEXT("BusName"));
	BusCategoryBuilder.AddProperty(BusNameHandle);

	if (GCriWare)
	{
		if (auto AtomRuntime = GCriWare->GetActiveAtomRuntime(); AtomRuntime.IsValid())
		{
			if (AtomRuntime->IsAtomMixerPluginEnabled())
			{
				TArray<TWeakObjectPtr<UObject>> Objects;
				DetailLayout.GetObjectsBeingCustomized(Objects);
				UAtomBus* AtomBus = CastChecked<UAtomBus>(Objects[0].Get());

				const FAtomBusMixerOutputInfo OutputInfo = GetBusOutputInfoFromConfig(AtomBus);

				BusCategoryBuilder.AddCustomRow(LOCTEXT("SpeakerChannelMap", "Speaker Channel Map"))
					.NameContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SpeakerChannelMapLabel", "Default Speaker Channel Map"))
						.ToolTipText(LOCTEXT("SpeakerChannelMapTooltip", "The speaker channel map assigned to this bus in the DSP bus settings."))
						.Font(DetailLayout.GetDetailFontItalic())
					]
					.ValueContent()
					[
						SNew(STextBlock)
						.Text(UEnum::GetDisplayValueAsText(OutputInfo.SpeakerChannelMap))
						.ToolTipText(LOCTEXT("SpeakerChannelMapTooltip", "The speaker channel map assigned to this bus in the DSP bus settings."))
						.Font(DetailLayout.GetDetailFontItalic())
					];

				BusCategoryBuilder.AddCustomRow(LOCTEXT("SoundRendererType", "Sound Renderer Type"))
					.NameContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SoundRendererTypeLabel", "Default Sound Renderer Type"))
						.ToolTipText(LOCTEXT("SoundRendererTypeTooltip", "The sound renderer type assigned to this bus in the DSP bus settings."))
						.Font(DetailLayout.GetDetailFontItalic())
					]
					.ValueContent()
					[
						SNew(STextBlock)
						.Text(UEnum::GetDisplayValueAsText(OutputInfo.SoundRendererType))
						.ToolTipText(LOCTEXT("SoundRendererTypeTooltip", "The sound renderer type assigned to this bus in the DSP bus settings."))
						.Font(DetailLayout.GetDetailFontItalic())
					];

				BusCategoryBuilder.AddCustomRow(LOCTEXT("SoundfieldRendererType", "Soundfield Renderer Type"))
					.NameContent()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SoundfieldRendererTypeLabel", "Default Soundfield Renderer Type"))
						.ToolTipText(LOCTEXT("SoundfieldRendererTypeTooltip", "The soundfield renderer type assigned to this bus in the DSP bus settings."))
						.Font(DetailLayout.GetDetailFontItalic())
					]
					.ValueContent()
					[
						SNew(STextBlock)
						.Text(UEnum::GetDisplayValueAsText(OutputInfo.SoundfieldRendererType))
						.ToolTipText(LOCTEXT("SoundfieldRendererTypeTooltip", "The soundfield renderer type assigned to this bus in the DSP bus settings."))
						.Font(DetailLayout.GetDetailFontItalic())
					];
			}
			else
			{
				// Adjust endpoint settings visibility according to AtomMixerPlugin status 
				TSharedPtr<IPropertyHandle> OverrideEndpointHandle = DetailLayout.GetProperty(TEXT("bOverrideEndpoint"));
				OverrideEndpointHandle->MarkHiddenByCustomization();
				TSharedPtr<IPropertyHandle> EndpointHandle = DetailLayout.GetProperty(TEXT("Endpoint"));
				EndpointHandle->MarkHiddenByCustomization();
			}
		}
	}

	// Get Effects category
	IDetailCategoryBuilder& CategoryBuilder = DetailLayout.EditCategory(TEXT("Effects"));

	// Hide effect classes
	TSharedPtr<IPropertyHandle> BusEffectClassesHandle = DetailLayout.GetProperty(TEXT("BusEffectChainClasses"));
	BusEffectClassesHandle->MarkHiddenByCustomization();

	// Cutomize effect chain array
	TSharedPtr<IPropertyHandle> BusEffectChainHandle = DetailLayout.GetProperty(TEXT("BusEffectChain"));
	TSharedRef<FDetailArrayBuilder> BusEffectChainBuilder = MakeShareable(new FDetailArrayBuilder(BusEffectChainHandle.ToSharedRef(), false));
	BusEffectChainBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateSP(this, &FAtomBusDetails::GenerateBusEffectPresetWidget));
	CategoryBuilder.AddCustomBuilder(BusEffectChainBuilder);

	
}

void FAtomBusDetails::GenerateBusEffectPresetWidget(TSharedRef<IPropertyHandle> PropertyHandle, int32 ArrayIndex, IDetailChildrenBuilder& ChildrenBuilder)
{
	IDetailPropertyRow& EffectPresetRow = ChildrenBuilder.AddProperty(PropertyHandle);

	FNumberFormattingOptions NoCommas;
	NoCommas.UseGrouping = false;
	UObject* BusEffectPresetObject = nullptr;
	PropertyHandle->GetValue(BusEffectPresetObject);
	if (UAtomBusEffectPreset* EffectPreset = Cast<UAtomBusEffectPreset>(BusEffectPresetObject))
	{
		const FText SlotDesc = FText::Format(LOCTEXT("BusEffectChainName", "Effect #{0} - {1}"), FText::AsNumber(ArrayIndex, &NoCommas), FText::FromName(EffectPreset->GetEffectName()));
		EffectPresetRow.DisplayName(SlotDesc);
	}
	else
	{
		const FText SlotDesc = FText::Format(LOCTEXT("BusEffectChainIndex", "Effect #{0}"), FText::AsNumber(ArrayIndex, &NoCommas));
		EffectPresetRow.DisplayName(SlotDesc);
	}

	EffectPresetRow.ShowPropertyButtons(false);

	FUIAction CopyAction;
	FUIAction PasteAction;
	PropertyHandle->CreateDefaultPropertyCopyPasteActions(CopyAction, PasteAction);

	EffectPresetRow.CustomWidget(false)
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(TOptional<float>())
		[
			PropertyHandle->CreatePropertyValueWidget()
		]
		.CopyAction(CopyAction)
		.PasteAction(PasteAction);
}

FAtomBusDetails::FAtomBusMixerOutputInfo FAtomBusDetails::GetBusOutputInfoFromConfig(const UAtomBus* InAtomBus) const
{
	check(InAtomBus);

	FAtomBusMixerOutputInfo OutputInfo = {};

	UAtomRack* Rack = Cast<UAtomRack>(InAtomBus->GetRack());
	if (Rack && Rack->DspBusSetting)
	{
		const auto& DspSetting = Rack->DspBusSetting->GetEditorDspSetting();

		// Look up for bus info then setup to AtomBus
		const auto BusInfoPtr = DspSetting.Buses.FindByPredicate([InAtomBus](const FAtomBusInfo& Info) { return InAtomBus->GetBusName() == Info.Name; });
		if (BusInfoPtr)
		{
			const FAtomBusInfo& BusInfo = *BusInfoPtr;
			OutputInfo.SpeakerChannelMap = BusInfo.SpeakerChannelMap;
			OutputInfo.SoundRendererType = BusInfo.SoundRendererType;
			OutputInfo.SoundfieldRendererType = BusInfo.SoundfieldRendererType;
		}
	}

	return MoveTemp(OutputInfo);
}

#undef LOCTEXT_NAMESPACE
