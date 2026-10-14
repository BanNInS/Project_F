
#include "AtomAudioVectorscope.h"

#include "SAudioVectorscopePanelWidget.h"
#include "Widgets/Docking/SDockTab.h"
#include "AudioWidgetsStyle.h"

#define LOCTEXT_NAMESPACE "AtomWidgets::FAudioVectorscope"

namespace AtomWidgets
{
	const FAudioAnalyzerRackUnitTypeInfo FAudioVectorscope::RackUnitTypeInfo
	{
		.TypeName = TEXT("FAudioVectorscope"),
		.DisplayName = LOCTEXT("AudioVectorscopeDisplayName", "Vectorscope"),
		.OnMakeAudioAnalyzerRackUnit = FOnMakeAudioAnalyzerRackUnit::CreateStatic(&MakeRackUnit),
		.VerticalSizeCoefficient = 0.25f,
	};

	FAudioVectorscope::FAudioVectorscope(FAtomRuntimeId InAtomRuntimeID,
		const uint32 InNumChannels,
		const float InTimeWindowMs,
		const float InMaxTimeWindowMs,
		const float InAnalysisPeriodMs,
		const EAudioPanelLayoutType InPanelLayoutType,
		const FAudioVectorscopePanelStyle* InPanelStyle,
		TObjectPtr<UAtomAudioBus> InExternalAudioBus)
	{
		VectorscopePanelStyle = InPanelStyle ? *InPanelStyle : FAudioWidgetsStyle::Get().GetWidgetStyle<FAudioVectorscopePanelStyle>("AudioVectorscope.PanelStyle");

		if (InExternalAudioBus != nullptr)
		{
			ensure(InExternalAudioBus->GetNumChannels() == InNumChannels);
			AudioBus = TStrongObjectPtr(InExternalAudioBus.Get());
		}
		else if (InNumChannels > 0)
		{
			CreateAudioBus(InNumChannels);
		}
		
		CreateDataProvider(InAtomRuntimeID, InTimeWindowMs, InMaxTimeWindowMs, InAnalysisPeriodMs);
		CreateVectorscopeWidget(InPanelLayoutType);
	}

	void FAudioVectorscope::CreateAudioBus(const uint32 InNumChannels)
	{
		AudioBus = TStrongObjectPtr(NewObject<UAtomAudioBus>());
		AudioBus->AudioBusChannels = AtomAudioBusUtils::ConvertIntToEAtomAudioBusChannels(InNumChannels);
	}

	void FAudioVectorscope::CreateDataProvider(FAtomRuntimeId InAtomRuntimeID, const float InTimeWindowMs,	const float InMaxTimeWindowMs, const float InAnalysisPeriodMs)
	{
		if (InAtomRuntimeID == FAudioBusInfo::InvalidAtomRuntimeID)
		{
			// Can't have a data provider without a valid audio device.
			AudioSamplesDataProvider.Reset();
			return;
		}

		check(AudioBus);

		AudioSamplesDataProvider = MakeShared<FWaveformAudioSamplesDataProvider>(InAtomRuntimeID, AudioBus.Get(), AudioBus->GetNumChannels(), InTimeWindowMs, InMaxTimeWindowMs, InAnalysisPeriodMs);
	}

	void FAudioVectorscope::CreateVectorscopeWidget(const EAudioPanelLayoutType InPanelLayoutType, const FAudioVectorscopePanelStyle* PanelStyle)
	{
		FFixedSampledSequenceView SequenceView{ .NumDimensions = 2, .SampleRate = 48000 }; // Initialize with usable fallback values (but no sample data in the view).

		if (AudioSamplesDataProvider.IsValid())
		{
			// Get the actual sequence view from the data provider:
			SequenceView = AudioSamplesDataProvider->GetDataView();
		}

		if (PanelStyle)
		{
			VectorscopePanelStyle = *PanelStyle;
		}

		if (!VectorscopePanelWidget.IsValid())
		{
			VectorscopePanelWidget = SNew(SAudioVectorscopePanelWidget, SequenceView)
				.PanelLayoutType(InPanelLayoutType)
				.PanelStyle(&VectorscopePanelStyle);
		}
		else
		{
			VectorscopePanelWidget->BuildWidget(SequenceView, InPanelLayoutType);
		}

		if (AudioSamplesDataProvider.IsValid())
		{
			// Interconnect data provider and widget
			AudioSamplesDataProvider->OnDataViewGenerated.AddSP(VectorscopePanelWidget.Get(), &SAudioVectorscopePanelWidget::ReceiveSequenceView);

			if (InPanelLayoutType == EAudioPanelLayoutType::Advanced)
			{
				VectorscopePanelWidget->OnDisplayPersistenceValueChanged.AddSP(AudioSamplesDataProvider.Get(), &FWaveformAudioSamplesDataProvider::SetTimeWindow);
			}
		}
	}

	void FAudioVectorscope::StartProcessing()
	{
		if (AudioSamplesDataProvider.IsValid())
		{
			AudioSamplesDataProvider->StartProcessing();
		}
	}

	void FAudioVectorscope::StopProcessing()
	{
		if (AudioSamplesDataProvider.IsValid())
		{
			AudioSamplesDataProvider->StopProcessing();
		}
	}

	UAtomAudioBus* FAudioVectorscope::GetAudioBus() const
	{
		return AudioBus.Get();
	}

	TSharedRef<SWidget> FAudioVectorscope::GetPanelWidget() const
	{
		return VectorscopePanelWidget.ToSharedRef();
	}

	void FAudioVectorscope::SetAudioBusInfo(const FAudioBusInfo& AudioBusInfo)
	{
		AudioBus = TStrongObjectPtr(AudioBusInfo.AudioBus.Get());
		CreateDataProvider(AudioBusInfo.AtomRuntimeID, RackUnitTimeWindowMs, RackUnitMaxTimeWindowMs, RackUnitAnalysisPeriodMs);
		CreateVectorscopeWidget(RackUnitPanelLayoutType);
	}

	TSharedRef<SDockTab> FAudioVectorscope::SpawnTab(const FSpawnTabArgs& Args) const
	{
		return SNew(SDockTab)
			.Clipping(EWidgetClipping::ClipToBounds)
			.Label(RackUnitTypeInfo.DisplayName)
			[
				GetPanelWidget()
			];
	}

	TSharedRef<IAudioAnalyzerRackUnit> FAudioVectorscope::MakeRackUnit(const FAudioAnalyzerRackUnitConstructParams& Params)
	{
		return MakeShared<FAudioVectorscope>(
			Params.AudioBusInfo.AtomRuntimeID,
			Params.AudioBusInfo.GetNumChannels(),
			RackUnitTimeWindowMs,
			RackUnitMaxTimeWindowMs,
			RackUnitAnalysisPeriodMs,
			RackUnitPanelLayoutType,
			&Params.StyleSet->GetWidgetStyle<FAudioVectorscopePanelStyle>("AudioVectorscope.PanelStyle"),
			Params.AudioBusInfo.AudioBus);
	}
} // namespace

#undef LOCTEXT_NAMESPACE
