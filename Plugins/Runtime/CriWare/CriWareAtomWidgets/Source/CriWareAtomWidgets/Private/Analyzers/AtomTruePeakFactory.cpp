
#include "Analyzers/AtomTruePeakFactory.h"

#include "DSP/SlidingWindow.h"

namespace Atom
{
	// Use for overall meter storage
	const int32 FTruePeakResult::ChannelIndexOverall = INDEX_NONE;

	void FTruePeakResult::Add(const FTruePeakEntry& InEntry)
	{
		// Store meter data in appropriate channel
		TArray<FTruePeakEntry>& MeterArray = ChannelTruePeakArrays.FindOrAdd(InEntry.Channel);
		MeterArray.Add(InEntry);
	}

	const TArray<FTruePeakEntry>& FTruePeakResult::GetChannelTruePeakArray(int32 ChannelIdx) const
	{
		return ChannelTruePeakArrays[ChannelIdx];
	}

	const TArray<FTruePeakEntry>& FTruePeakResult::GetTruePeakArray() const
	{
		return ChannelTruePeakArrays[ChannelIndexOverall];
	}

	int32 FTruePeakResult::GetNumChannels() const 
	{
		if (ChannelTruePeakArrays.Num() > 0)
		{
			// Channel meter arrays have -1 reserved for overall meter results
			return ChannelTruePeakArrays.Num() - 1;
		}
		return 0;
	}

	FTruePeakWorker::FTruePeakWorker(const FAnalyzerParameters& InParams, const FTruePeakSettings& InAnalyzerSettings)
		: NumChannels(InParams.NumChannels)
		, NumAnalyzedBuffers(0)
		, SampleRate(InParams.SampleRate)
	{
		check(NumChannels > 0);
		check(SampleRate > 0);

		NumHopFrames = FMath::CeilToInt(InAnalyzerSettings.AnalysisPeriod * (float)InParams.SampleRate);
		NumHopFrames = FMath::Max(1, NumHopFrames);

		float NumAnalysisFrames = InAnalyzerSettings.AnalysisPeriod * (float)SampleRate;
		int32 NumWindowSamples = NumAnalysisFrames * NumChannels;
		int32 NumHopSamples = NumHopFrames * NumChannels;

		InternalBuffer = MakeUnique<Audio::TSlidingBuffer<float>>(NumWindowSamples, NumHopSamples);
		TruePeakAnalyzer = MakeUnique<FTruePeakAnalyzer>(InParams.SampleRate, NumChannels, InAnalyzerSettings);
	}

	void FTruePeakWorker::Analyze(TArrayView<const float> InAudio, IAnalyzerResult* OutResult) 
	{
		FTruePeakResult* TruePeakResult = static_cast<FTruePeakResult*>(OutResult);
		check(TruePeakResult != nullptr);

		Audio::TAutoSlidingWindow<float> SlidingWindow(*InternalBuffer, InAudio, InternalWindow);

		// Loop through entire array of input samples. 
		for (const TArray<float>& Window : SlidingWindow)
		{
			const FTruePeakAnalyzerResults& Results = TruePeakAnalyzer->ProcessAudio(Window);

			float OverallPeak = 0.0f;
			float OverallPeakHold = 0.0f;
			float Timestamp = 0.0f;
			int32 MaxClippingSamples = 0;
			float MaxClippingValue = 0.0f;

			// Add the per-channel results
			for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
			{
				FTruePeakEntry NewEntry = 
				{ 
					ChannelIndex, 
					Results.TimeSec, 
					Results.PeakValues[ChannelIndex],
					Results.PeakHoldValues[ChannelIndex],
					Results.ClippingValues[ChannelIndex],
					Results.NumSamplesClipping[ChannelIndex]
				};

				Timestamp = Results.TimeSec;

				// The max peak over all channels will be the "overall" peak
				OverallPeak = FMath::Max(OverallPeak, NewEntry.PeakValue);
				OverallPeakHold = FMath::Max(OverallPeakHold, NewEntry.PeakHoldValue);

				// The "overall" clipping data will be the max number of samples clipped between channels and the max clipping value
				MaxClippingSamples = FMath::Max(MaxClippingSamples, NewEntry.NumSamplesClipping);
				MaxClippingValue = FMath::Max(MaxClippingValue, NewEntry.ClippingValue);

				TruePeakResult->Add(NewEntry);
			}

			// Add to results
			FTruePeakEntry OverallEntry = 
			{ 
				FTruePeakResult::ChannelIndexOverall, 
				Timestamp, 
				OverallPeak,
				OverallPeakHold,
				MaxClippingValue,
				MaxClippingSamples
			};
			TruePeakResult->Add(OverallEntry);
		}
	}

	FName FTruePeakFactory::GetName() const 
	{
		static FName FactoryName(TEXT("AtomTruePeakFactory"));
		return FactoryName;
	}

	FString FTruePeakFactory::GetTitle() const
	{
		return TEXT("Real-Time True Peak Analyzer");
	}

	TUniquePtr<IAnalyzerResult> FTruePeakFactory::NewResult() const
	{
		TUniquePtr<FTruePeakResult> Result = MakeUnique<FTruePeakResult>();
		return Result;
	}

	TUniquePtr<IAnalyzerWorker> FTruePeakFactory::NewWorker(const FAnalyzerParameters& InParams, const IAnalyzerSettings* InSettings) const
	{
		const FTruePeakSettings* TruePeakSettings = static_cast<const FTruePeakSettings*>(InSettings);

		check(nullptr != TruePeakSettings);

		return MakeUnique<FTruePeakWorker>(InParams, *TruePeakSettings);
	}
}

