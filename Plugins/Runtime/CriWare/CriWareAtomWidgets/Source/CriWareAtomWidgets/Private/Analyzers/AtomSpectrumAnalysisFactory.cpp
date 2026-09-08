
#include "Analyzers/AtomSpectrumAnalysisFactory.h"

#include "DSP/DeinterleaveView.h"
#include "DSP/FloatArrayMath.h"
#include "DSP/SlidingWindow.h"

namespace Atom
{
	void FSpectrumResult::Add(FSpectrumEntry&& InEntry)
	{
		// Store Spectrum data in appropriate channel
		TArray<FSpectrumEntry>& SpectrumArray = ChannelSpectrumArrays.FindOrAdd(InEntry.Channel);
		SpectrumArray.Add(MoveTemp(InEntry));
	}

	const TArray<FSpectrumEntry>& FSpectrumResult::GetChannelSpectrumArray(int32 ChannelIdx) const
	{
		return ChannelSpectrumArrays[ChannelIdx];
	}

	int32 FSpectrumResult::GetNumChannels() const
	{
		return ChannelSpectrumArrays.Num();
	}

	FSpectrumAnalysisWorker::FSpectrumAnalysisWorker(const FAnalyzerParameters& InParams, const FSpectrumAnalysisSettings& InAnalyzerSettings)
		: NumChannels(InParams.NumChannels)
		, SampleRate(InParams.SampleRate)
	{
		check(NumChannels > 0);
		check(SampleRate > 0);
		check(FMath::IsPowerOfTwo(InAnalyzerSettings.FFTSize));
		
		// From IFFTAlgorithm::ForwardRealToComplex required number of output values 
		NumOutputFrames = InAnalyzerSettings.FFTSize / 2 + 1;
		NumWindowFrames = InAnalyzerSettings.FFTSize;
		NumWindowSamples = InAnalyzerSettings.FFTSize * InParams.NumChannels;

		NumHopFrames = FMath::CeilToInt(InAnalyzerSettings.AnalysisPeriod * (float)InParams.SampleRate);
		NumHopFrames = FMath::Max(1, NumHopFrames);

		int32 NumHopSamples = NumHopFrames * NumChannels;

		bDownmixToMono = InAnalyzerSettings.bDownmixToMono;
		if (bDownmixToMono)
		{
			MonoBuffer.Reset(NumWindowFrames);
			MonoBuffer.AddUninitialized(NumWindowFrames);
		}

		InternalBuffer = MakeUnique<Audio::TSlidingBuffer<float>>(NumWindowSamples, NumHopSamples);
		SpectrumAnalyzer = MakeUnique<FSpectrumAnalyzer>(InParams.SampleRate, InAnalyzerSettings);
	}

	void FSpectrumAnalysisWorker::Analyze(TArrayView<const float> InAudio, IAnalyzerResult* OutResult)
	{
		FSpectrumResult* SpectrumResult = static_cast<FSpectrumResult*>(OutResult);
		check(SpectrumResult != nullptr);

		Audio::TAutoSlidingWindow<float> SlidingWindow(*InternalBuffer, InAudio, InternalWindow);

		// Mono
		if (NumChannels == 1)
		{
			for (const TArray<float>& Window : SlidingWindow)
			{
				FSpectrumEntry NewEntry;
				NewEntry.Channel = 0;
				NewEntry.Timestamp = static_cast<float>(FrameCounter) / SampleRate;
				NewEntry.SpectrumValues.AddZeroed(NumOutputFrames);

				SpectrumAnalyzer->ProcessAudio(Window, NewEntry.SpectrumValues);

				SpectrumResult->Add(MoveTemp(NewEntry));
				FrameCounter += NumHopFrames;
			}
			return;
		}

		// Multichannel
		if (bDownmixToMono)
		{
			for (const TArray<float>& Window : SlidingWindow)
			{
				FMemory::Memset(MonoBuffer.GetData(), 0, sizeof(float) * NumWindowFrames);

				// Downmix to mono
				for (int32 FrameIndex = 0; FrameIndex < MonoBuffer.Num(); ++FrameIndex)
				{
					for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
					{
						MonoBuffer[FrameIndex] += Window[FrameIndex * NumChannels + ChannelIndex];
					}
				}

				// Equal power sum. assuming incoherent signals.
				Audio::ArrayMultiplyByConstantInPlace(MonoBuffer, 1.f / FMath::Sqrt(static_cast<float>(NumChannels)));

				// Create results 
				FSpectrumEntry NewEntry;
				NewEntry.Channel = 0; // Mono channel index
				NewEntry.Timestamp = static_cast<float>(FrameCounter) / SampleRate;
				NewEntry.SpectrumValues.AddZeroed(NumOutputFrames);

				SpectrumAnalyzer->ProcessAudio(MonoBuffer, NewEntry.SpectrumValues);

				SpectrumResult->Add(MoveTemp(NewEntry));
				FrameCounter += NumHopFrames;
			}
		}
		else // Calculate per channel 
		{
			for (const TArray<float>& Window : SlidingWindow)
			{
				int32 ChannelIndex = 0;
				Audio::TAutoDeinterleaveView<float, Audio::FAudioBufferAlignedAllocator> DeinterleaveView(Window, ChannelBuffer, NumChannels);
				for (auto Channel : DeinterleaveView)
				{
					// Create results 
					FSpectrumEntry NewEntry;
					NewEntry.Channel = ChannelIndex;
					NewEntry.Timestamp = static_cast<float>(FrameCounter) / SampleRate;
					NewEntry.SpectrumValues.AddZeroed(NumOutputFrames);
					
					SpectrumAnalyzer->ProcessAudio(Channel.Values, NewEntry.SpectrumValues);

					SpectrumResult->Add(MoveTemp(NewEntry));
					ChannelIndex++;
				}
				FrameCounter += NumHopFrames;
			}
		}
	}

	FName FSpectrumAnalysisFactory::GetName() const 
	{
		static FName FactoryName(TEXT("AtomSpectrumAnalysisFactory"));
		return FactoryName;
	}

	FString FSpectrumAnalysisFactory::GetTitle() const
	{
		return TEXT("Real-Time Spectrum Analyzer");
	}

	TUniquePtr<IAnalyzerResult> FSpectrumAnalysisFactory::NewResult() const
	{
		TUniquePtr<FSpectrumResult> Result = MakeUnique<FSpectrumResult>();
		return Result;
	}

	TUniquePtr<IAnalyzerWorker> FSpectrumAnalysisFactory::NewWorker(const FAnalyzerParameters& InParams, const IAnalyzerSettings* InSettings) const
	{
		const FSpectrumAnalysisSettings* SpectrumSettings = static_cast<const FSpectrumAnalysisSettings*>(InSettings);

		check(nullptr != SpectrumSettings);

		return MakeUnique<FSpectrumAnalysisWorker>(InParams, *SpectrumSettings);
	}
}

