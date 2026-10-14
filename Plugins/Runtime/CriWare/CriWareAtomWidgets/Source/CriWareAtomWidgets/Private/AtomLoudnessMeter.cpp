
#include "AtomLoudnessMeter.h"

#include "UObject/UObjectGlobals.h"
#include "Misc/CoreDelegates.h"

#include "Atom/AtomAudioBusSubsystem.h"
#include "Atom/AtomRuntime.h"

#include "AtomLevelMeterTypes.h"
#include "AtomLevelMeterStyle.h"
#include "Analyzers/AtomLoudnessAnalyzer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomLoudnessMeter)

#define LOCTEXT_NAMESPACE "AtomWidgets::FAtomLoudnessMeter"

namespace AtomWidgets
{
	const FAudioAnalyzerRackUnitTypeInfo FAtomLoudnessMeter::RackUnitTypeInfo
	{
		.TypeName = TEXT("FAtomLoudnessMeter"),
		.DisplayName = LOCTEXT("AtomLoudnessMeterMeterDisplayName", "Loudness Meter"),
		.OnMakeAudioAnalyzerRackUnit = FOnMakeAudioAnalyzerRackUnit::CreateStatic(&MakeRackUnit),
		.VerticalSizeCoefficient = 0.5f,
	};

	FAtomLoudnessMeter::FAtomLoudnessMeter(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, const TObjectPtr<UAtomAudioBus> InExternalAudioBus, const FAtomLevelMeterDefaultColorStyle* MeterWidgetStyle)
	{
		const FAtomLevelMeterDefaultColorStyle* WidgetStyle = MeterWidgetStyle ? MeterWidgetStyle : &AtomWidgets::FSlateStyle::Get().GetWidgetStyle<FAtomLevelMeterDefaultColorStyle>("AtomLevelMeter.DefaultColorStyle");
		Widget = SNew(SAtomLoudnessMeter)
			.BackgroundColor(FLinearColor::Transparent)
			.MetersBackgroundColor(WidgetStyle->MeterBackgroundColor)
			.MomentaryValueColor(FLinearColor(0.87f, 0.87f, 0.87f))
			.ShortTermValueColor(FLinearColor(0.0f, 0.83f, 0.83f))
			.IntegratedValueColor(FLinearColor(0.0f, 0.85f, 0.28f))
			.TruePeakValueColor(WidgetStyle->MeterValueColor)
			.MeterPeakColor(FLinearColor::Transparent)
			.MeterClippingColor(WidgetStyle->MeterClippingColor)
			.MeterScaleColor(WidgetStyle->MeterScaleColor)
			.MeterScaleLabelColor(WidgetStyle->MeterScaleLabelColor)
			.OnAtomLoudnessMeterResetMenuEntryClicked_Raw(this, &FAtomLoudnessMeter::OnReset);

		//ContextMenuExtension = Widget->AddContextMenuExtension(EExtensionHook::Before, nullptr, FMenuExtensionDelegate::CreateRaw(this, &FAtomLoudnessMeter::ExtendSpectrumPlotContextMenu));

		Init(InNumChannels, InAtomRuntimeID, InExternalAudioBus);
	}

	FAtomLoudnessMeter::FAtomLoudnessMeter(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, const TObjectPtr<UAtomRackBase> InAtomRack, const FAtomLevelMeterDefaultColorStyle* MeterWidgetStyle)
	{
		const FAtomLevelMeterDefaultColorStyle* WidgetStyle = MeterWidgetStyle ? MeterWidgetStyle : &AtomWidgets::FSlateStyle::Get().GetWidgetStyle<FAtomLevelMeterDefaultColorStyle>("AtomLevelMeter.DefaultColorStyle");
		Widget = SNew(SAtomLoudnessMeter)
			.BackgroundColor(FLinearColor::Transparent)
			.MetersBackgroundColor(WidgetStyle->MeterBackgroundColor)
			.MomentaryValueColor(FLinearColor(0.87f, 0.87f, 0.87f))
			.ShortTermValueColor(FLinearColor(0.0f, 0.83f, 0.83f))
			.IntegratedValueColor(FLinearColor(0.0f, 0.85f, 0.28f))
			.TruePeakValueColor(WidgetStyle->MeterValueColor)
			.MeterPeakColor(FLinearColor::Transparent)
			.MeterClippingColor(WidgetStyle->MeterClippingColor)
			.MeterScaleColor(WidgetStyle->MeterScaleColor)
			.MeterScaleLabelColor(WidgetStyle->MeterScaleLabelColor)
			.OnAtomLoudnessMeterResetMenuEntryClicked_Raw(this, &FAtomLoudnessMeter::OnReset);

		//ContextMenuExtension = Widget->AddContextMenuExtension(EExtensionHook::Before, nullptr, FMenuExtensionDelegate::CreateRaw(this, &FAtomLoudnessMeter::ExtendSpectrumPlotContextMenu));

		Init(InNumChannels, InAtomRuntimeID, InAtomRack);
	}

	FAtomLoudnessMeter::~FAtomLoudnessMeter()
	{
		Teardown();
	}

	UAtomAudioBus* FAtomLoudnessMeter::GetAudioBus() const
	{
		return AudioBus.Get();
	}

	UAtomRackBase* FAtomLoudnessMeter::GetRack() const
	{
		return AtomRack.Get();
	}

	TSharedRef<SAtomLoudnessMeter> FAtomLoudnessMeter::GetWidget() const
	{
		return StaticCastSharedRef<SAtomLoudnessMeter>(Widget->AsShared());
	}

	void FAtomLoudnessMeter::Init(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomRackBase> InAtomRack)
	{
		AtomRuntimeID = InAtomRuntimeID;

		check(InNumChannels > 0);

		Teardown();

		AtomRack = InAtomRack ? TStrongObjectPtr(InAtomRack.Get()) : nullptr;
		
		if (AtomRack.IsValid())
		{
			DelegateObject = TStrongObjectPtr(NewObject<UAtomLoudnessMeterDelegate>());
			DelegateObject->LoudnessMeter = this;

			LoudnessDelegateHandle.BindUFunction(DelegateObject.Get(), "OnLoudnessMeterMeasure");
			TruePeakDelegateHandle.BindUFunction(DelegateObject.Get(), "OnTruePeakMeterMeasure");

			FAtomRuntime* AtomRuntime = FAtomRuntimeManager::Get()->GetAtomRuntimeRaw(InAtomRuntimeID);
			AtomRuntime->AddLoudnessMeterDelegate(InAtomRack, LoudnessDelegateHandle);
			AtomRuntime->AddTruePeakMeterDelegate(InAtomRack, TruePeakDelegateHandle);

			FAtomLoudnessMeterSettings RackLoudnessMeterSettings;
			RackLoudnessMeterSettings.ShortTermTime = 3;
			RackLoudnessMeterSettings.IntegratedTime = 600;
			AtomRuntime->StartLoudnessMeterMeasuring(InAtomRack, RackLoudnessMeterSettings);

			FAtomTruePeakMeterSettings RackTruePeakMeterSettings;
			RackTruePeakMeterSettings.AnalysisPeriod = 0.01f;
			RackTruePeakMeterSettings.PeakHoldTime = 4000;
			RackTruePeakMeterSettings.bSampleClipping = false;
			AtomRuntime->StartTruePeakMeterMeasuring(InAtomRack, RackTruePeakMeterSettings);
		}

		constexpr float DefaultMeterValue = ATOM_MIN_VOLUME_DECIBELS;
		constexpr float DefaultPeakValue = ATOM_MIN_VOLUME_DECIBELS;
		LoudnessMeterInfo = { DefaultMeterValue, DefaultMeterValue, DefaultMeterValue, { DefaultMeterValue, DefaultPeakValue } };

		if (Widget.IsValid())
		{
			Widget->SetMeterInfo(LoudnessMeterInfo);
		}
	}

	void FAtomLoudnessMeter::Init(const int32 InNumChannels, const FAtomRuntimeId InAtomRuntimeID, TObjectPtr<UAtomAudioBus> InExternalAudioBus)
	{
		AtomRuntimeID = InAtomRuntimeID;

		check(InNumChannels > 0);

		Teardown();

		LoudnessSettings = TStrongObjectPtr(NewObject<UAtomLoudnessSettings>());
		LoudnessSettings->AnalysisPeriod = 0.1f; // each 100 ms
		LoudnessSettings->MomentarySize = 0.4f; // 400 ms
		LoudnessSettings->ShortTermSize = 3.0f; // 3 seconds
		LoudnessSettings->MaxIntegratedSize = 600.0f; // 10 minutes

		LoudnessAnalyzer = TStrongObjectPtr(NewObject<UAtomLoudnessAnalyzer>());
		LoudnessAnalyzer->Settings = LoudnessSettings.Get();

		TruePeakSettings = TStrongObjectPtr(NewObject<UAtomTruePeakSettings>());
		TruePeakSettings->AnalysisPeriod = 0.01f;
		TruePeakSettings->PeakHoldTime = 4000.0f;

		TruePeakAnalyzer = TStrongObjectPtr(NewObject<UAtomTruePeakAnalyzer>());
		TruePeakAnalyzer->Settings = TruePeakSettings.Get();

		bUseExternalAudioBus = InExternalAudioBus != nullptr;

		AudioBus = bUseExternalAudioBus ? TStrongObjectPtr(InExternalAudioBus.Get()) : TStrongObjectPtr(NewObject<UAtomAudioBus>());
		if (!bUseExternalAudioBus)
		{
			AudioBus->AudioBusChannels = AtomAudioBusUtils::ConvertIntToEAtomAudioBusChannels(InNumChannels);
		}

		LoudnessResultsDelegateHandle = LoudnessAnalyzer->OnLatestOverallLoudnessResultsNative.AddRaw(this, &FAtomLoudnessMeter::OnAudioBusLoudnessOutput);
		LoudnessAnalyzer->StartAnalyzing(InAtomRuntimeID, AudioBus.Get());

		TruePeakResultsDelegateHandle = TruePeakAnalyzer->OnLatestOverallTruePeakResultsNative.AddRaw(this, &FAtomLoudnessMeter::OnAudioBusTruePeakOutput);
		TruePeakAnalyzer->StartAnalyzing(InAtomRuntimeID, AudioBus.Get());

		constexpr float DefaultMeterValue = ATOM_MIN_VOLUME_DECIBELS;
		constexpr float DefaultPeakValue = ATOM_MIN_VOLUME_DECIBELS;
		LoudnessMeterInfo = { DefaultMeterValue, DefaultMeterValue, DefaultMeterValue, { DefaultMeterValue, DefaultPeakValue } };

		if (Widget.IsValid())
		{
			Widget->SetMeterInfo(LoudnessMeterInfo);
		}
	}

	void FAtomLoudnessMeter::OnRackLoudnessMeterMeasure(const FAtomLoudnessMeterMeasure& InMeasure)
	{
		if (Widget.IsValid())
		{
			LoudnessMeterInfo.MomentaryValue = InMeasure.Momentary;
			LoudnessMeterInfo.ShortTermValue = InMeasure.ShortTerm;
			LoudnessMeterInfo.IntegratedValue = InMeasure.Integrated;

			// Update the widget
			Widget->SetMeterInfo(LoudnessMeterInfo);
		}
	}

	void FAtomLoudnessMeter::OnRackTruePeakMeterMeasure(const TArray<FAtomTruePeakMeterMeasure>& InChannelMeasures)
	{
		if (Widget.IsValid())
		{
			float OverallLevel = -96.0f, OverallHoldLevel = -96.0f;
			for (auto& ChannelMeasure : InChannelMeasures)
			{
				OverallLevel = FMath::Max(OverallLevel, ChannelMeasure.Level);
				OverallHoldLevel = FMath::Max(OverallHoldLevel, ChannelMeasure.HoldLevel);
			}

			LoudnessMeterInfo.TruePeakMeterChannelInfo.MeterValue = OverallLevel;
			LoudnessMeterInfo.TruePeakMeterChannelInfo.PeakValue = OverallHoldLevel;
			LoudnessMeterInfo.TruePeakMeterChannelInfo.ClippingValue = -2.0f;

			// Update the widget
			Widget->SetMeterInfo(LoudnessMeterInfo);
		}
	}

	void FAtomLoudnessMeter::OnAudioBusLoudnessOutput(UAtomLoudnessAnalyzer* InLoudnessAnalyzer, const FAtomLoudnessResults& InResults)
	{
		if (InLoudnessAnalyzer == LoudnessAnalyzer.Get())
		{
			LoudnessMeterInfo.MomentaryValue = InResults.Momentary;
			LoudnessMeterInfo.ShortTermValue = InResults.ShortTerm;
			LoudnessMeterInfo.IntegratedValue = InResults.Integrated;

			// Update the widget
			if (Widget.IsValid())
			{
				Widget->SetMeterInfo(LoudnessMeterInfo);
			}
		}
	}

	void FAtomLoudnessMeter::OnAudioBusTruePeakOutput(UAtomTruePeakAnalyzer* InTruePeakAnalyzer, const FAtomTruePeakResults& InResults)
	{
		if (InTruePeakAnalyzer == TruePeakAnalyzer.Get())
		{
			LoudnessMeterInfo.TruePeakMeterChannelInfo.MeterValue = InResults.PeakValue;
			LoudnessMeterInfo.TruePeakMeterChannelInfo.PeakValue = InResults.PeakHoldValue;
			LoudnessMeterInfo.TruePeakMeterChannelInfo.ClippingValue = -2.0f;

			// Update the widget
			if (Widget.IsValid())
			{
				Widget->SetMeterInfo(LoudnessMeterInfo);
			}
		}
	}

	FReply FAtomLoudnessMeter::OnReset()
	{
		if (AtomRack)
		{
			TStrongObjectPtr<UAtomRackBase> AtomRackPtr = AtomRack;
			Init(1, AtomRuntimeID, AtomRack.Get());
		}
		else if (AudioBus)
		{
			TStrongObjectPtr<UAtomAudioBus> AudioBusPtr = AudioBus;
			Init(AudioBus->GetNumChannels(), AtomRuntimeID, AudioBusPtr.Get());
		}

		return FReply::Handled();
	}

	void FAtomLoudnessMeter::Teardown()
	{
		if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
		{
			FAtomRuntimeHandle AtomRuntime = RuntimeManager->GetAtomRuntime(AtomRuntimeID);
			if (AtomRuntime.IsValid() && AtomRack.IsValid())
			{
				AtomRuntime->StopLoudnessMeterMeasuring(AtomRack.Get());
				AtomRuntime->RemoveLoudnessMeterDelegate(AtomRack.Get(), LoudnessDelegateHandle);
				AtomRuntime->StopTruePeakMeterMeasuring(AtomRack.Get());
				AtomRuntime->RemoveTruePeakMeterDelegate(AtomRack.Get(), TruePeakDelegateHandle);
			}
		}

		if (LoudnessAnalyzer.IsValid() && LoudnessAnalyzer->IsValidLowLevel())
		{
			LoudnessAnalyzer->StopAnalyzing();
			if (LoudnessResultsDelegateHandle.IsValid())
			{
				LoudnessAnalyzer->OnLatestOverallLoudnessResultsNative.Remove(LoudnessResultsDelegateHandle);
			}

			LoudnessAnalyzer.Reset();
		}

		if (TruePeakAnalyzer.IsValid() && TruePeakAnalyzer->IsValidLowLevel())
		{
			TruePeakAnalyzer->StopAnalyzing();
			if (TruePeakResultsDelegateHandle.IsValid())
			{
				TruePeakAnalyzer->OnLatestOverallTruePeakResultsNative.Remove(TruePeakResultsDelegateHandle);
			}

			TruePeakAnalyzer.Reset();
		}

		LoudnessResultsDelegateHandle.Reset();
		TruePeakResultsDelegateHandle.Reset();

		LoudnessDelegateHandle.Clear();
		TruePeakDelegateHandle.Clear();
		DelegateObject.Reset();

		AtomRack.Reset();
		AudioBus.Reset();
		LoudnessMeterInfo = {};
		LoudnessSettings.Reset();
		TruePeakSettings.Reset();

		bUseExternalAudioBus = false;
	}

	TSharedRef<SDockTab> FAtomLoudnessMeter::SpawnTab(const FSpawnTabArgs& Args) const
	{
		return SNew(SDockTab)
			.Clipping(EWidgetClipping::ClipToBounds)
			.Label(RackUnitTypeInfo.DisplayName)
			[
				SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Fill)
					[
						GetWidget()
					]
			];
	}

	TSharedRef<IAudioAnalyzerRackUnit> FAtomLoudnessMeter::MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params)
	{
		// If we have an AudioMaterialMeterStyle create as an AudioMaterialMeter, otherwise create as a standard Slate meter:
		//const FName MaterialMeterStyleName("AudioMaterialMeter.Style");
		/*if (Params.StyleSet->HasWidgetStyle<FAudioMaterialMeterStyle>(MaterialMeterStyleName))
		{
			const AudioWidgets::FAudioMaterialMeterStyle& AudioMaterialMeterStyle = Params.StyleSet->GetWidgetStyle<FAudioMaterialMeterStyle>(MaterialMeterStyleName);
			return MakeShared<FAtomLevelMeter>(Params.AudioBusInfo.GetNumChannels(), Params.AudioBusInfo.AtomRuntimeID, AudioMaterialMeterStyle, Params.AudioBusInfo.AudioBus);
		}
		else*/
		{
			const FAtomLevelMeterDefaultColorStyle& MeterDefaultColorStyle = Params.StyleSet->GetWidgetStyle<FAtomLevelMeterDefaultColorStyle>("AtomLevelMeter.DefaultColorStyle");
			if (Params.AtomRackInfo.AtomRack != nullptr)
			{
				return MakeShared<FAtomLoudnessMeter>(Params.AtomRackInfo.GetNumChannels(), Params.AtomRackInfo.AtomRuntimeID, Params.AtomRackInfo.AtomRack, &MeterDefaultColorStyle);
			}
			
			return MakeShared<FAtomLoudnessMeter>(Params.AudioBusInfo.GetNumChannels(), Params.AudioBusInfo.AtomRuntimeID, Params.AudioBusInfo.AudioBus, &MeterDefaultColorStyle);
		}
	}
} // namespace

#undef LOCTEXT_NAMESPACE
