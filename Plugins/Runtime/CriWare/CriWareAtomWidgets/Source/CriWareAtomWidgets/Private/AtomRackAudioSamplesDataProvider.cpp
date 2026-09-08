
#include "AtomRackAudioSamplesDataProvider.h"

#include "DSP/DeinterleaveView.h"
#include "AudioWidgetsLog.h"

#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRack.h"

namespace AtomWidgets
{
	FRackAudioSamplesDataProvider::FRackAudioSamplesDataProvider(const FAtomRuntimeId InAtomRuntimeID,
		UAtomRackBase* InAtomRack,
		const uint32 InNumChannelToProvide,
		const float InTimeWindowMs,
		const float InMaxTimeWindowMs,
		const float InAnalysisPeriodMs)
	{
		if (!InAtomRack)
		{
			UE_LOG(LogAudioWidgets, Error, TEXT("Unable to obtain audio samples for visualization without a valid Atom rack."));
			return;
		}

		FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get();
		if (!RuntimeManager)
		{
			UE_LOG(LogAudioWidgets, Error, TEXT("Unable to obtain audio samples for visualization without a valid Atom runtime manager."));
			return;
		}

		AtomRuntime = RuntimeManager->GetAtomRuntimeRaw(InAtomRuntimeID);
		if (!AtomRuntime)
		{
			UE_LOG(LogAudioWidgets, Error, TEXT("Unable to obtain audio samples for visualization without a valid Atom runtime."));
			return;
		}

		AtomRack = InAtomRack;

		NumChannelsToProvide = InNumChannelToProvide;
		NumChannels = AtomRuntime->GetRackNumOutputChannels(AtomRack);

		SampleRate = static_cast<uint32>(AtomRuntime->GetRackSampleRate(AtomRack));

		// Init audio buffers
		TimeWindowMaxTimeSamples = FMath::RoundToInt(InMaxTimeWindowMs / 1000.0f * NumChannelsToProvide * SampleRate);

		AudioSamplesForView.Init(0.0f, TimeWindowMaxTimeSamples);
		DataView = FFixedSampledSequenceView{ MakeArrayView(AudioSamplesForView.GetData(), AudioSamplesForView.Num()), NumChannelsToProvide, SampleRate };

		AudioSamplesCircularBuffer.SetCapacity(TimeWindowMaxTimeSamples * 2); // Twice of the amount needed for display 

		SetTimeWindow(InTimeWindowMs);
		SetAnalysisPeriod(InAnalysisPeriodMs);
	}

	FRackAudioSamplesDataProvider::~FRackAudioSamplesDataProvider()
	{
		StopProcessing();
	}

	void FRackAudioSamplesDataProvider::ResetAudioBuffers()
	{
		AudioSamplesCircularBuffer.Reset(TimeWindowMaxTimeSamples * 2);
		AudioSamplesForView.Init(0.0f, AudioSamplesForView.Num());

		AudioSamplesCircularBuffer.SetNum(TimeWindowSamples);
	}

	void FRackAudioSamplesDataProvider::StartProcessing()
	{
		if (bIsProcessing)
		{
			return;
		}

		check(AtomRack);
		check(AtomRuntime);

		ResetAudioBuffers();

		// start runtime level meter with a delegate
		DelegateObject = TStrongObjectPtr(NewObject<UAtomRackDataLevelMeterDelegate>());
		DelegateObject->LevelMeter = this;

		DelegateHandle.BindUFunction(DelegateObject.Get(), "OnLevelMeterMeasure");
		AtomRuntime->AddLevelMeterDelegate(AtomRack, DelegateHandle);

		FAtomLevelMeterSettings RackLevelMeterSettings;
		RackLevelMeterSettings.AnalysisPeriod = 0.001f;
		RackLevelMeterSettings.PeakHoldTime = 1000;
		AtomRuntime->StartLevelMeterMeasuring(AtomRack, RackLevelMeterSettings);
	}

	void FRackAudioSamplesDataProvider::StopProcessing()
	{
		AtomRuntime->StopLevelMeterMeasuring(AtomRack);
		AtomRuntime->RemoveLevelMeterDelegate(AtomRack, DelegateHandle);
		DelegateHandle.Clear();
		DelegateObject.Reset();
	}

	void FRackAudioSamplesDataProvider::SetChannelToAnalyze(const int32 InChannel)
	{
		ChannelIndexToAnalyze = InChannel - 1;
		ResetAudioBuffers();
	}
	
	void FRackAudioSamplesDataProvider::SetTriggerMode(const EAudioOscilloscopeTriggerMode InTriggerMode)
	{
		TriggerMode = InTriggerMode;
		bHasTriggered = false;
	}
	
	void FRackAudioSamplesDataProvider::SetTriggerThreshold(const float InTriggerThreshold)
	{
		TriggerThreshold = InTriggerThreshold;
		bHasTriggered = false;
	}

	void FRackAudioSamplesDataProvider::SetTimeWindow(const float InTimeWindowMs)
	{
		TimeWindowSamples = FMath::RoundToInt(InTimeWindowMs / 1000.0f * NumChannelsToProvide * SampleRate);
		DataView = FFixedSampledSequenceView{ MakeArrayView(AudioSamplesForView.GetData(), AudioSamplesForView.Num()).Slice(0, TimeWindowSamples), NumChannelsToProvide, SampleRate };

		if (NumChannelsToProvide == 1)
		{
			AudioSamplesCircularBuffer.SetNum(TimeWindowSamples);
		}
		else
		{
			// Adjust read pointer so it falls in a sample of the first channel
			const uint32 ChannelsModulo = TimeWindowSamples % NumChannelsToProvide;
			const uint32 TimeWindowSamplesWithOffset = TimeWindowSamples - ChannelsModulo;

			AudioSamplesCircularBuffer.SetNum(TimeWindowSamplesWithOffset);
		}
	}
	
	void FRackAudioSamplesDataProvider::SetAnalysisPeriod(const float InAnalysisPeriodMs)
	{
		AnalysisPeriodSamples = InAnalysisPeriodMs * 0.001f * SampleRate;
		AudioSamplesCircularBuffer.SetNum(TimeWindowSamples);
	}

	void FRackAudioSamplesDataProvider::PushAudioSamplesToCircularBuffer(const TArray<FAtomLevelMeterMeasure>& Measures)
	{
		if (const int32 NumChannelsAvailable = Measures.Num(); NumChannelsAvailable > 0)
		{
			if (NumChannelsToProvide == 1)
			{
				float Data = Atom::ConvertToLinear(Measures[ChannelIndexToAnalyze].PeakLevel);
				NumSamplesPushedToCircularBuffer += AudioSamplesCircularBuffer.Push(&Data, 1);
			}
			else
			{
				TempAudioBuffer.Reset();
				TempAudioBuffer.AddUninitialized(NumChannelsAvailable);

				for (int ChannelIndex = 0; ChannelIndex < NumChannelsAvailable; ++ChannelIndex)
				{
					TempAudioBuffer[ChannelIndex] = Atom::ConvertToLinear(Measures[ChannelIndex].PeakLevel);
				}

				NumSamplesPushedToCircularBuffer += AudioSamplesCircularBuffer.Push(TempAudioBuffer.GetData(), NumChannelsAvailable);
			}
		}
	}

	void FRackAudioSamplesDataProvider::OnLevelMeterMeasure(const TArray<FAtomLevelMeterMeasure>& Measures)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(FAtomRackSamplesDataProvider::Tick);

		//if (PatchOutput.IsValid())
		{
			PushAudioSamplesToCircularBuffer(Measures);

			const uint32 NumAvailableSamplesToReadCircularBuffer = AudioSamplesCircularBuffer.Num();

			if (TriggerMode == EAudioOscilloscopeTriggerMode::None || !bHasTriggered)
			{
				if (NumAvailableSamplesToReadCircularBuffer >= TimeWindowSamples)
				{
					// Obtain the audio samples we want to display
					AudioSamplesCircularBuffer.Peek(AudioSamplesForView.GetData(), TimeWindowSamples);

					// Advance the read pointer
					AudioSamplesCircularBuffer.Pop(AnalysisPeriodSamples * NumChannelsToProvide);
				}
			}
			else
			{
				// Keep refreshing the trigger view with new samples until we reach TimeWindowSamples
				if (NumAvailableSamplesToReadCircularBuffer < TimeWindowSamples)
				{
					AudioSamplesCircularBuffer.Peek(AudioSamplesForView.GetData(), NumAvailableSamplesToReadCircularBuffer);
				}
				else
				{
					AudioSamplesCircularBuffer.Pop(AudioSamplesForView.GetData(), NumAvailableSamplesToReadCircularBuffer);
					bHasTriggered = false;
				}
			}

			// Send data if we have reached the analysis period time
			//if (NumSamplesPushedToCircularBuffer >= AnalysisPeriodSamples * NumChannelsToProvide)
			{
				RequestSequenceView(TRange<double>::Inclusive(0, 1));
				NumSamplesPushedToCircularBuffer = 0;
			}
		}
	}

	FFixedSampledSequenceView FRackAudioSamplesDataProvider::RequestSequenceView(const TRange<double> DataRatioRange)
	{
		// Broadcast audio samples data view
		OnDataViewGenerated.Broadcast(DataView, /*FirstRenderedSample*/ 0);

		return DataView;
	}
} // namespace

