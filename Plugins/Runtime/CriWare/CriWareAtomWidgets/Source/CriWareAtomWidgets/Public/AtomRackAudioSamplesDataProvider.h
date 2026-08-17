

#pragma once

#include "AtomWaveformAudioSamplesDataProvider.h" 

#include "Atom/Atom.h"

#include "AtomRackAudioSamplesDataProvider.generated.h"

class UAtomRackBase;

namespace AtomWidgets
{
	class CRIWAREATOMWIDGETS_API FRackAudioSamplesDataProvider : public FAudioSamplesDataProvider
	{
	public:
		FRackAudioSamplesDataProvider(FAtomRuntimeId InAtomRuntimeID,
			UAtomRackBase* InAtomRack,
			uint32 InNumChannelToProvide,
			float InTimeWindowMs,
			float InMaxTimeWindowMs,
			float InAnalysisPeriodMs);

		virtual ~FRackAudioSamplesDataProvider();

		void ResetAudioBuffers();

		void StartProcessing();
		void StopProcessing();

		FFixedSampledSequenceView GetDataView() { return DataView; };
		uint32 GetNumChannels() { return NumChannels; }

		void SetChannelToAnalyze(const int32 InChannel);
		void SetTriggerMode(const EAudioOscilloscopeTriggerMode InTriggerMode);
		void SetTriggerThreshold(const float InTriggerThreshold);
		void SetTimeWindow(const float InTimeWindowMs);
		void SetAnalysisPeriod(const float InAnalysisPeriodMs);

		virtual FFixedSampledSequenceView RequestSequenceView(const TRange<double> DataRatioRange) override;

		FOnDataViewGenerated OnDataViewGenerated;

	private:
		void PushAudioSamplesToCircularBuffer(const TArray<FAtomLevelMeterMeasure>& Measures);
		void OnLevelMeterMeasure(const TArray<FAtomLevelMeterMeasure>& Measures);

		uint32 NumChannelsToProvide = 0;
		uint32 NumChannels = 0;
		uint32 SampleRate = 0;

		uint32 TimeWindowMaxTimeSamples = 0;

		FAtomRuntime* AtomRuntime = nullptr;

		UAtomRackBase* AtomRack = nullptr;

		TArray<float> TempAudioBuffer;
		Audio::TCircularAudioBuffer<float> AudioSamplesCircularBuffer;
		TArray<float> AudioSamplesForView;
		FFixedSampledSequenceView DataView;

		int32 ChannelIndexToAnalyze = 0;

		EAudioOscilloscopeTriggerMode TriggerMode = EAudioOscilloscopeTriggerMode::None;
		float TriggerThreshold = 0.0f;

		uint32 TimeWindowSamples = 0;
		uint32 AnalysisPeriodSamples = 0;

		bool bIsProcessing = false;
		bool bHasTriggered = false;

		uint32 NumSamplesPushedToCircularBuffer = 0;


		TStrongObjectPtr<UAtomRackDataLevelMeterDelegate> DelegateObject;
		FOnAtomRackLevelMeterMeasureBP DelegateHandle;

		friend UAtomRackDataLevelMeterDelegate;
	};
} // namespace

UCLASS()
class UAtomRackDataLevelMeterDelegate : public UObject
{
	GENERATED_BODY()

	AtomWidgets::FRackAudioSamplesDataProvider* LevelMeter;

	UFUNCTION()
	void OnLevelMeterMeasure(const TArray<FAtomLevelMeterMeasure>& Measures)
	{
		LevelMeter->OnLevelMeterMeasure(Measures);
	}

	friend AtomWidgets::FRackAudioSamplesDataProvider;
};