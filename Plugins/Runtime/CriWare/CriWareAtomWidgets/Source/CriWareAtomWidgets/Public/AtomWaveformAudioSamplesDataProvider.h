

#pragma once

#include "Containers/ArrayView.h"
#include "Containers/Ticker.h"
#include "DSP/Dsp.h"
#include "DSP/MultithreadedPatching.h"
#include "FixedSampledSequenceView.h"
#include "IFixedSampledSequenceViewProvider.h"
#include "AudioOscilloscopeEnums.h"

#include "CriWareDefines.h"

class FAtomRuntime;
class UAtomAudioBus;

struct FWaveformAudioSamplesResult;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDataViewGenerated, FFixedSampledSequenceView /*InView*/, const uint32 /*FirstSampleIndex*/)

namespace AtomWidgets
{
	class CRIWAREATOMWIDGETS_API FAudioSamplesDataProvider : public IFixedSampledSequenceViewProvider,
		public TSharedFromThis<FAudioSamplesDataProvider>
	{
	public:
		virtual ~FAudioSamplesDataProvider() {}

		virtual void ResetAudioBuffers() = 0;

		virtual void StartProcessing() = 0;
		virtual void StopProcessing() = 0;

		virtual FFixedSampledSequenceView GetDataView() = 0;
		virtual uint32 GetNumChannels() = 0;

		virtual void SetChannelToAnalyze(const int32 InChannel) = 0;
		virtual void SetTriggerMode(const EAudioOscilloscopeTriggerMode InTriggerMode) = 0;
		virtual void SetTriggerThreshold(const float InTriggerThreshold) = 0;
		virtual void SetTimeWindow(const float InTimeWindowMs) = 0;
		virtual void SetAnalysisPeriod(const float InAnalysisPeriodMs) = 0;

		FOnDataViewGenerated OnDataViewGenerated;

	protected:

	};

	class CRIWAREATOMWIDGETS_API FWaveformAudioSamplesDataProvider : public FAudioSamplesDataProvider
	{
	public:
		FWaveformAudioSamplesDataProvider(const FAtomRuntimeId InAtomRuntimeID,
			UAtomAudioBus* InAudioBus, 
			const uint32 InNumChannelToProvide, 
			const float InTimeWindowMs, 
			const float InMaxTimeWindowMs, 
			const float InAnalysisPeriodMs);

		virtual ~FWaveformAudioSamplesDataProvider();

		void ResetAudioBuffers();

		void StartProcessing();
		void StopProcessing();

		FFixedSampledSequenceView GetDataView() { return DataView; };
		uint32 GetNumChannels() { return NumChannels; }
		const UAtomAudioBus* GetAudioBus() { return AudioBus; }

		void SetChannelToAnalyze(const int32 InChannel);
		void SetTriggerMode(const EAudioOscilloscopeTriggerMode InTriggerMode);
		void SetTriggerThreshold(const float InTriggerThreshold);
		void SetTimeWindow(const float InTimeWindowMs);
		void SetAnalysisPeriod(const float InAnalysisPeriodMs);

		virtual FFixedSampledSequenceView RequestSequenceView(const TRange<double> DataRatioRange) override;

	private:
		void PushAudioSamplesToCircularBuffer();
		bool Tick(float DeltaTime);

		FTSTicker::FDelegateHandle TickerHandle = nullptr;

		uint32 NumChannelsToProvide = 0;
		uint32 NumChannels          = 0;
		uint32 SampleRate           = 0;

		uint32 TimeWindowMaxTimeSamples = 0;

		const FAtomRuntime* AtomRuntime = nullptr;

		UAtomAudioBus* AudioBus = nullptr;
		Audio::FPatchOutputStrongPtr PatchOutput = nullptr;

		TArray<float> TempAudioBuffer;
		Audio::TCircularAudioBuffer<float> AudioSamplesCircularBuffer;
		TArray<float> AudioSamplesForView;
		FFixedSampledSequenceView DataView;

		int32 ChannelIndexToAnalyze = 0;

		EAudioOscilloscopeTriggerMode TriggerMode = EAudioOscilloscopeTriggerMode::None;
		float TriggerThreshold = 0.0f;

		uint32 TimeWindowSamples     = 0;
		uint32 AnalysisPeriodSamples = 0;

		bool bIsProcessing = false;
		bool bHasTriggered = false;

		uint32 NumSamplesPushedToCircularBuffer = 0;
	};
} // namespace
