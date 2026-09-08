
#include "Analyzers/AtomTruePeakAnalyzer.h"

#include "DSP/FloatArrayMath.h"
#include "DSP/DeinterleaveView.h"

namespace Atom
{
	static void UpSampling4xChannel_Vectorized(uint32 NumSamples, const float* InAudioBuffer, float* OutAudioBuffer, float DelayBuffer[12])
	{
		static const float FilterCoefs[4][12] = {
			{ 0.0017089843750f,  0.0109863281250f, -0.0196533203125f,  0.0332031250000f, -0.0594482421875f,  0.1373291015625f,  0.9721679687500f, -0.1022949218750f,  0.0476074218750f, -0.0266113281250f,  0.0148925781250f, -0.0083007812500},
			{-0.0291748046875f,  0.0292968750000f, -0.0517578125000f,  0.0891113281250f, -0.1665039062500f,  0.4650878906250f,  0.7797851562500f, -0.2003173828125f,  0.1015625000000f, -0.0582275390625f,  0.0330810546875f, -0.0189208984375},
			{-0.0189208984375f,  0.0330810546875f, -0.0582275390625f,  0.1015625000000f, -0.2003173828125f,  0.7797851562500f,  0.4650878906250f, -0.1665039062500f,  0.0891113281250f, -0.0517578125000f,  0.0292968750000f, -0.0291748046875},
			{-0.0083007812500f,  0.0148925781250f, -0.0266113281250f,  0.0476074218750f, -0.1022949218750f,  0.9721679687500f,  0.1373291015625f, -0.0594482421875f,  0.0332031250000f, -0.0196533203125f,  0.0109863281250f,  0.0017089843750}
		};

		uint32 i, j;
		const float* InBuffer = InAudioBuffer;
		float* OutBuffer = OutAudioBuffer;

		for (i = 0; i < NumSamples; i++)
		{
			DelayBuffer[0] = *InBuffer++;
			for (j = 0; j < 4; j++)
			{
				const float* Coef = FilterCoefs[j];
				*OutBuffer++ =
					DelayBuffer[0] * Coef[0]
					+ DelayBuffer[1] * Coef[1]
					+ DelayBuffer[2] * Coef[2]
					+ DelayBuffer[3] * Coef[3]
					+ DelayBuffer[4] * Coef[4]
					+ DelayBuffer[5] * Coef[5]
					+ DelayBuffer[6] * Coef[6]
					+ DelayBuffer[7] * Coef[7]
					+ DelayBuffer[8] * Coef[8]
					+ DelayBuffer[9] * Coef[9]
					+ DelayBuffer[10] * Coef[10]
					+ DelayBuffer[11] * Coef[11];
			}

			DelayBuffer[11] = DelayBuffer[10];
			DelayBuffer[10] = DelayBuffer[9];
			DelayBuffer[9] = DelayBuffer[8];
			DelayBuffer[8] = DelayBuffer[7];
			DelayBuffer[7] = DelayBuffer[6];
			DelayBuffer[6] = DelayBuffer[5];
			DelayBuffer[5] = DelayBuffer[4];
			DelayBuffer[4] = DelayBuffer[3];
			DelayBuffer[3] = DelayBuffer[2];
			DelayBuffer[2] = DelayBuffer[1];
			DelayBuffer[1] = DelayBuffer[0];
		}
	}

	FTruePeakAnalyzer::FTruePeakAnalyzer(float InSampleRate, int32 InNumChannels, const FTruePeakAnalyzerSettings& InSettings)
	: Settings(InSettings)
	, SampleRate(InSampleRate)
	, NumChannels(InNumChannels)
	{
		if (!ensure(NumChannels > 0))
		{
			NumChannels = 1;
		}
		if (!ensure(SampleRate > 0.f))
		{
			SampleRate = 48000.f;
		}

		PeakDataPerChannel.AddDefaulted(NumChannels);
		ClippingDataPerChannel.AddDefaulted(NumChannels);
		PeakHoldFrames = FMath::Max(1, FMath::RoundToInt(static_cast<float>(InSettings.PeakHoldTime) * SampleRate / 1000.f));
	}

	FTruePeakAnalyzerResults FTruePeakAnalyzer::ProcessAudio(TArrayView<const float> InSampleView)
	{
		// Feed audio through the envelope followers
		const float* InData = InSampleView.GetData();
		const int32 NumSamples = InSampleView.Num();
		const int32 NumFrames = NumSamples / NumChannels;

		check((NumSamples % NumChannels) == 0);

		// Reset the clipping data for next block of audio
		for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ChannelIndex++)
		{
			ClippingDataPerChannel[ChannelIndex].ClippingValue = 0.0f;
			ClippingDataPerChannel[ChannelIndex].NumSamplesClipping = 0;
		}

		for (int32 i = 0; i < NumSamples; i++)
		{
			if (FMath::Abs(InData[i]) > Settings.ClippingThreshold)
			{
				const int32 ChannelIndex = i % NumChannels;
				FClippingData& ClippingData = ClippingDataPerChannel[ChannelIndex];

				// Track how many samples clipped in this block
				ClippingData.NumSamplesClipping++;

				// Track the max clipping value in this block of audio
				ClippingData.ClippingValue = FMath::Max(ClippingData.ClippingValue, InData[i]);
			}
		}

		TArray<float> ArrayToFill;
		Audio::TDeinterleaveView<float> InBuffer(InSampleView, NumChannels);
		auto ChannelIterator = InBuffer.begin(ArrayToFill);

		// Find peaks in the buffer.
		for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ChannelIndex++)
		{
			FPeakData& PeakData = PeakDataPerChannel[ChannelIndex];
			// Reset max envelope data. 
			PeakData.PeakValue = 0.0f;
			if (PeakData.FramesUntilPeakHoldReset < 1)
			{
				PeakData.PeakHoldValue = 0;
			}

			int32 PeakNumSamples = NumFrames * 4; // 4x sampler

			PeakBuffer.Reset(PeakNumSamples);
			PeakBuffer.AddUninitialized(PeakNumSamples);
			
			auto InChannel = *ChannelIterator;
			UpSampling4xChannel_Vectorized(NumFrames, InChannel.Values.GetData(), PeakBuffer.GetData(), PeakDataPerChannel[ChannelIndex].Delay);
			++ChannelIterator;

			Audio::ArrayAbsInPlace(PeakBuffer);
			const float* Peaks = PeakBuffer.GetData();

			// Temp variables to hold onto prior frame's peak info.
			float PriorValue = PeakData.PriorPeakValue;
			bool bPriorSlopeIsPositive = PeakData.bPriorPeakSlopeIsPositive;

			for (int32 SampleIndex = 0; SampleIndex < PeakNumSamples; SampleIndex++)
			{
				// Get maximum value in resampled peak.
				PeakData.PeakValue = FMath::Max(Peaks[SampleIndex], PeakData.PeakValue);

				// Positive if we're rising
				bool bNewSlopeIsNegative = (Peaks[SampleIndex] - PriorValue) >= 0.0f;
				bool bIsPeak = bNewSlopeIsNegative && bPriorSlopeIsPositive;

				// Detect if the new peak value is less than the previous peak value (i.e. local maximum)
				if (bIsPeak)
				{
					const int32 FrameIndex = SampleIndex / 4;

					// Check if the peak value is larger than the current peak value or if enough time has elapsed to store a new peak
					if ((PeakData.PeakHoldValue < Peaks[SampleIndex]) || (FrameIndex > PeakData.FramesUntilPeakHoldReset))
					{
						PeakData.PeakHoldValue = Peaks[SampleIndex];
						PeakData.FramesUntilPeakHoldReset = FrameIndex + PeakHoldFrames;
					}
				}

				// Update the current peak envelope data to the new value
				PriorValue = Peaks[SampleIndex];
				bPriorSlopeIsPositive = !bNewSlopeIsNegative;
			}

			// Save for next call
			PeakData.PriorPeakValue = PriorValue;
			PeakData.bPriorPeakSlopeIsPositive = bPriorSlopeIsPositive;

			if (PeakData.FramesUntilPeakHoldReset > 0)
			{
				// Decrement frame counters for next call.
				PeakData.FramesUntilPeakHoldReset -= NumFrames;
			}
		}

		// Build the results data
		FTruePeakAnalyzerResults Results;

		FrameCounter += NumFrames;
		Results.TimeSec = static_cast<float>(FrameCounter) / SampleRate;

		for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
		{
			// Add clipping data
			FClippingData& ClippingData = ClippingDataPerChannel[ChannelIndex];

			Results.ClippingValues.Add(ClippingData.ClippingValue);
			Results.NumSamplesClipping.Add(ClippingData.NumSamplesClipping);

			// update sample count of the env data
			const FPeakData& PeakData = PeakDataPerChannel[ChannelIndex];

			Results.PeakValues.Add(PeakData.PeakValue);
			Results.PeakHoldValues.Add(PeakData.PeakHoldValue);
		}

		return Results;
	}

	const FTruePeakAnalyzerSettings& FTruePeakAnalyzer::GetSettings() const
	{
		return Settings;
	}
} // namespace
