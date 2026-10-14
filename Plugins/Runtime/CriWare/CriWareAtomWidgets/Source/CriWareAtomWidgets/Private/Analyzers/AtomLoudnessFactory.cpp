
#include "Analyzers/AtomLoudnessFactory.h"
#include "DSP/SlidingWindow.h"

namespace Atom
{
	void FLoudnessResult::Add(const FLoudnessEntry& InEntry)
	{
		// Store loudness data
		LoudnessArray.Add(InEntry);
	}

	const TArray<FLoudnessEntry>& FLoudnessResult::GetLoudnessArray() const
	{
		return LoudnessArray;
	}

	FLoudnessWorker::FLoudnessWorker(const FAnalyzerParameters& InParams, const FLoudnessSettings& InAnalyzerSettings)
		: NumChannels(InParams.NumChannels)
		, NumAnalyzedBuffers(0)
		, NumHopFrames(0)
		, SampleRate(InParams.SampleRate)
	{
		check(NumChannels > 0);
		check(SampleRate > 0);

		// Include NumChannels in calculating window size because windows are generated 
		// with interleaved samples and deinterleaved later. 
		int32 NumWindowSamples = FMath::Max(1, InAnalyzerSettings.WindowSize * NumChannels);

		NumHopFrames = FMath::CeilToInt(InAnalyzerSettings.AnalysisPeriod * InParams.SampleRate);
		NumHopFrames = FMath::Max(1, NumHopFrames);

		int32 NumHopSamples = NumHopFrames * NumChannels;

		InternalBuffer = MakeUnique<Audio::TSlidingBuffer<float>>(NumWindowSamples, NumHopSamples);

		Analyzer = MakeUnique<FLoudnessAnalyzer>(InParams.SampleRate, InParams.NumChannels, InAnalyzerSettings);
	}

	void FLoudnessWorker::Analyze(TArrayView<const float> InAudio, IAnalyzerResult* OutResult) 
	{
		FLoudnessResult* LoudnessResult = static_cast<FLoudnessResult*>(OutResult);
		check(nullptr != LoudnessResult);

		Audio::TAutoSlidingWindow<float> SlidingWindow(*InternalBuffer, InAudio, InternalWindow);

		// Loop through entire array of input samples. 
		for (const TArray<float>& Window : SlidingWindow)
		{
			AnalyzeWindow(Window, *LoudnessResult);
		}
	}

	void FLoudnessWorker::AnalyzeWindow(TArrayView<const float> InWindow, FLoudnessResult& OutResult)
	{
		FLoudnessAnalyzerResults Results;

		// Calculate perceptual magnitude
		Analyzer->CalculateLoudness(InWindow, NumChannels, Results);

		// Calculate timestamp
		float Timestamp = ((NumAnalyzedBuffers * NumHopFrames) + (0.5f * Analyzer->GetSettings().WindowSize)) / SampleRate;

		// Add overall FLoudnessEntry to result
		const FLoudnessEntry OverallLoudnessEntry =
		{
			Timestamp, 
			Results.Momentary, Results.ShortTerm, Results.Integrated
		};
		OutResult.Add(OverallLoudnessEntry);

		NumAnalyzedBuffers++;
	}

	FName FLoudnessFactory::GetName() const 
	{
		static FName FactoryName(TEXT("AtomLoudnessFactory"));
		return FactoryName;
	}

	FString FLoudnessFactory::GetTitle() const
	{
		return TEXT("Real-Time Loudness Analyzer");
	}

	TUniquePtr<IAnalyzerResult> FLoudnessFactory::NewResult() const
	{
		TUniquePtr<FLoudnessResult> Result = MakeUnique<FLoudnessResult>();
		return Result;
	}

	TUniquePtr<IAnalyzerWorker> FLoudnessFactory::NewWorker(const FAnalyzerParameters& InParams, const IAnalyzerSettings* InSettings) const
	{
		const FLoudnessSettings* LoudnessSettings = static_cast<const FLoudnessSettings*>(InSettings);

		check(nullptr != LoudnessSettings);

		return MakeUnique<FLoudnessWorker>(InParams, *LoudnessSettings);
	}
}

