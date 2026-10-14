
#pragma once

#include "AudioResampler.h"

namespace Atom
{
	struct CRIWAREATOMWIDGETS_API FTruePeakAnalyzerSettings
	{
		// Peak detector hold time in milliseconds
		int32 PeakHoldTime = 100;

		// The volume threshold to detect clipping
		float ClippingThreshold = 1.0f;
	};

	// Per-channel analyzer results
	struct CRIWAREATOMWIDGETS_API FTruePeakAnalyzerResults
	{
		float TimeSec = 0.0f;
		TArray<float> PeakValues;
		TArray<float> PeakHoldValues;
		TArray<float> ClippingValues;
		TArray<int32> NumSamplesClipping;
	};

	class CRIWAREATOMWIDGETS_API FTruePeakAnalyzer
	{
	public:

		/** Construct analyzer */
		FTruePeakAnalyzer(float InSampleRate, int32 NumChannels, const FTruePeakAnalyzerSettings& InSettings);

		/**
		 * Calculate the true peak results for the input samples.  Will return the current true peak and the current peak hold value of the analyzer.
		 */
		FTruePeakAnalyzerResults ProcessAudio(TArrayView<const float> InSampleView);

		/**
		 * Return const reference to settings used inside this analyzer.
		 */
		const FTruePeakAnalyzerSettings& GetSettings() const;

	protected:
		// Resampler per channel
		Audio::FAlignedFloatBuffer PeakBuffer;

		// Per-channel clipping data
		struct FClippingData
		{
			// How many samples we've been clipping
			int32 NumSamplesClipping = 0;
			float ClippingValue = 0.0f;
		};
		TArray<FClippingData> ClippingDataPerChannel;

		// State to track the peak data
		struct FPeakData
		{
			float Delay[12] = {};
			float PeakValue = 0.0f;
			float PeakHoldValue = 0.0f;
			int32 FramesUntilPeakHoldReset = 0;

			// Current slope of the peak
			float PriorPeakValue = 0.f;
			bool bPriorPeakSlopeIsPositive = false;
		};
		// Per-channel peak data
		TArray<FPeakData> PeakDataPerChannel;


		FTruePeakAnalyzerSettings Settings;
		float SampleRate = 0.0f;
		int32 NumChannels = 0;
		int32 PeakHoldFrames = 0;
		int64 FrameCounter = 0;
	};
} // namespace
