
#include "AtomAudioOscilloscope.h"

#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"

#include "AtomWaveformAudioSamplesDataProvider.h"
#include "AtomRackAudioSamplesDataProvider.h"

#define LOCTEXT_NAMESPACE "AtomWidgets::FAudioOscilloscope"

namespace AtomWidgets
{
	const FAudioAnalyzerRackUnitTypeInfo FAudioOscilloscope::RackUnitTypeInfo
	{
		.TypeName = TEXT("FAudioOscilloscope"),
		.DisplayName = LOCTEXT("AudioOscilloscopeDisplayName", "Oscilloscope"),
		.OnMakeAudioAnalyzerRackUnit = FOnMakeAudioAnalyzerRackUnit::CreateStatic(&MakeRackUnit),
		.VerticalSizeCoefficient = 0.25f,
	};

	FAudioOscilloscope::FAudioOscilloscope(FAtomRuntimeId InAtomRuntimeID,
		const uint32 InNumChannels, 
		const float InTimeWindowMs, 
		const float InMaxTimeWindowMs, 
		const float InAnalysisPeriodMs, 
		const EAudioPanelLayoutType InPanelLayoutType,
		const FAudioOscilloscopePanelStyle* InOscilloscopePanelStyle,
		TObjectPtr<UAtomAudioBus> InExternalAudioBus)
	{
		OscilloscopePanelStyle = InOscilloscopePanelStyle ? *InOscilloscopePanelStyle : FAudioWidgetsStyle::Get().GetWidgetStyle<FAudioOscilloscopePanelStyle>("AudioOscilloscope.PanelStyle");

		if (InExternalAudioBus != nullptr)
		{
			ensure(InExternalAudioBus->GetNumChannels() == InNumChannels);
			AudioBus = TStrongObjectPtr(InExternalAudioBus.Get());
		}
		else if (InNumChannels > 0)
		{
			CreateAudioBus(InNumChannels);
		}

		CreateDataProvider(InAtomRuntimeID, InTimeWindowMs, InMaxTimeWindowMs, InAnalysisPeriodMs, InPanelLayoutType);
		CreateOscilloscopeWidget(InNumChannels, InPanelLayoutType);
	}

	FAudioOscilloscope::FAudioOscilloscope(FAtomRuntimeId InAtomRuntimeID,
		TObjectPtr<UAtomRackBase> InAtomRack,
		const uint32 InNumChannels,
		const float InTimeWindowMs,
		const float InMaxTimeWindowMs,
		const float InAnalysisPeriodMs,
		const EAudioPanelLayoutType InPanelLayoutType,
		const FAudioOscilloscopePanelStyle* InOscilloscopePanelStyle)
	{
		OscilloscopePanelStyle = InOscilloscopePanelStyle ? *InOscilloscopePanelStyle : FAudioWidgetsStyle::Get().GetWidgetStyle<FAudioOscilloscopePanelStyle>("AudioOscilloscope.PanelStyle");

		SetAtomRack(InAtomRack);
		CreateDataProvider(InAtomRuntimeID, InTimeWindowMs, InMaxTimeWindowMs, InAnalysisPeriodMs, InPanelLayoutType);
		CreateOscilloscopeWidget(InNumChannels, InPanelLayoutType);
	}

	void FAudioOscilloscope::CreateAudioBus(const uint32 InNumChannels)
	{
		AtomRack = nullptr;
		AudioBus = TStrongObjectPtr(NewObject<UAtomAudioBus>());
		AudioBus->AudioBusChannels = AtomAudioBusUtils::ConvertIntToEAtomAudioBusChannels(InNumChannels);
	}

	void FAudioOscilloscope::SetAtomRack(TObjectPtr<UAtomRackBase> InAtomRack)
	{
		AudioBus = nullptr;
		AtomRack = InAtomRack ? TStrongObjectPtr(InAtomRack.Get()) : nullptr;
	}

	void FAudioOscilloscope::CreateDataProvider(FAtomRuntimeId InAtomRuntimeID,
		const float InTimeWindowMs,
		const float InMaxTimeWindowMs,
		const float InAnalysisPeriodMs,
		const EAudioPanelLayoutType InPanelLayoutType)
	{
		if (AudioBus)
		{
			const uint32 NumChannelsToProvide = (InPanelLayoutType == EAudioPanelLayoutType::Advanced) ? 1 : AudioBus->GetNumChannels(); // Advanced mode waveform display is based on channel selection
			AudioSamplesDataProvider = MakeShared<FWaveformAudioSamplesDataProvider>(InAtomRuntimeID, AudioBus.Get(), NumChannelsToProvide, InTimeWindowMs, InMaxTimeWindowMs, InAnalysisPeriodMs);
		}
		else if (AtomRack)
		{
			uint32 NumChannelsToProvide = 1;  // Advanced mode waveform display is based on channel selection
			if (InPanelLayoutType != EAudioPanelLayoutType::Advanced)
			{
				if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
				{
					if (FAtomRuntime* AtomRuntime = RuntimeManager->GetAtomRuntimeRaw(InAtomRuntimeID))
					{
						NumChannelsToProvide = AtomRuntime->GetRackNumOutputChannels(AtomRack.Get());
					}
				}
			}

			AudioSamplesDataProvider = MakeShared<FRackAudioSamplesDataProvider>(InAtomRuntimeID, AtomRack.Get(), NumChannelsToProvide, InTimeWindowMs, InMaxTimeWindowMs, InAnalysisPeriodMs);
		}
	}

	void FAudioOscilloscope::CreateOscilloscopeWidget(const uint32 InNumChannels, const EAudioPanelLayoutType InPanelLayoutType)
	{
		check(AudioSamplesDataProvider);

		const FFixedSampledSequenceView SequenceView = AudioSamplesDataProvider->GetDataView();

		if (!OscilloscopePanelWidget.IsValid())
		{
			OscilloscopePanelWidget = SNew(SAudioOscilloscopePanelWidget, SequenceView, InNumChannels)
			.PanelLayoutType(InPanelLayoutType)
			.PanelStyle(&OscilloscopePanelStyle);
		}
		else
		{
			OscilloscopePanelWidget->BuildWidget(SequenceView, InNumChannels, InPanelLayoutType);
		}

		// Interconnect data provider and widget
		AudioSamplesDataProvider->OnDataViewGenerated.AddSP(OscilloscopePanelWidget.Get(), &SAudioOscilloscopePanelWidget::ReceiveSequenceView);

		if (InPanelLayoutType == EAudioPanelLayoutType::Advanced)
		{
			OscilloscopePanelWidget->OnSelectedChannelChanged.AddSP(AudioSamplesDataProvider.Get(),  &FAudioSamplesDataProvider::SetChannelToAnalyze);
			OscilloscopePanelWidget->OnTriggerModeChanged.AddSP(AudioSamplesDataProvider.Get(),      &FAudioSamplesDataProvider::SetTriggerMode);
			OscilloscopePanelWidget->OnTriggerThresholdChanged.AddSP(AudioSamplesDataProvider.Get(), &FAudioSamplesDataProvider::SetTriggerThreshold);
			OscilloscopePanelWidget->OnTimeWindowValueChanged.AddSP(AudioSamplesDataProvider.Get(),  &FAudioSamplesDataProvider::SetTimeWindow);
			OscilloscopePanelWidget->OnAnalysisPeriodChanged.AddSP(AudioSamplesDataProvider.Get(),   &FAudioSamplesDataProvider::SetAnalysisPeriod);
		}
	}

	void FAudioOscilloscope::StartProcessing()
	{
		AudioSamplesDataProvider->StartProcessing();
	}

	void FAudioOscilloscope::StopProcessing()
	{
		AudioSamplesDataProvider->StopProcessing();
	}

	UAtomAudioBus* FAudioOscilloscope::GetAudioBus() const
	{
		return AudioBus.Get();
	}

	TSharedRef<SWidget> FAudioOscilloscope::GetPanelWidget() const
	{
		return OscilloscopePanelWidget.ToSharedRef();
	}

	void FAudioOscilloscope::SetAudioBusInfo(const FAudioBusInfo& AudioBusInfo)
	{
		AudioBus = TStrongObjectPtr(AudioBusInfo.AudioBus.Get());
		CreateDataProvider(AudioBusInfo.AtomRuntimeID, RackUnitTimeWindowMs, RackUnitMaxTimeWindowMs, RackUnitAnalysisPeriodMs, RackUnitPanelLayoutType);
		CreateOscilloscopeWidget(AudioBusInfo.AudioBus->GetNumChannels(), RackUnitPanelLayoutType);
	}

	TSharedRef<SDockTab> FAudioOscilloscope::SpawnTab(const FSpawnTabArgs& Args) const
	{
		return SNew(SDockTab)
			.Clipping(EWidgetClipping::ClipToBounds)
			.Label(RackUnitTypeInfo.DisplayName)
			[
				GetPanelWidget()
			];
	}

	TSharedRef<IAudioAnalyzerRackUnit> FAudioOscilloscope::MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params)
	{
		if (Params.AtomRackInfo.AtomRack)
		{
			return MakeShared<FAudioOscilloscope>(
				Params.AtomRackInfo.AtomRuntimeID,
				Params.AtomRackInfo.AtomRack,
				Params.AtomRackInfo.GetNumChannels(),
				RackUnitTimeWindowMs,
				RackUnitMaxTimeWindowMs,
				RackUnitAnalysisPeriodMs,
				RackUnitPanelLayoutType,
				&Params.StyleSet->GetWidgetStyle<FAudioOscilloscopePanelStyle>("AudioOscilloscope.PanelStyle"));
		}

		return MakeShared<FAudioOscilloscope>(
			Params.AudioBusInfo.AtomRuntimeID,
			Params.AudioBusInfo.GetNumChannels(),
			RackUnitTimeWindowMs,
			RackUnitMaxTimeWindowMs,
			RackUnitAnalysisPeriodMs,
			RackUnitPanelLayoutType,
			&Params.StyleSet->GetWidgetStyle<FAudioOscilloscopePanelStyle>("AudioOscilloscope.PanelStyle"),
			Params.AudioBusInfo.AudioBus);
	}
}

#undef LOCTEXT_NAMESPACE