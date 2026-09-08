
#pragma once

#include "DSP/DSP.h"
#include "CriWareDefines.h"

#define CRI_API CRIWAREATOMWIDGETS_API

namespace Atom
{
	/** Settings for FLoudnessAnalyzer */
    struct FLoudnessAnalyzerSettings
    {
		/** Number of samples per analysis. */
        int32 WindowSize;

		/** Number of results for short-term integration. */
		int32 ShortTermSize;

		/** Number of results for long-term integration. */
		int32 IntegratedSize;

		/** */
		float AbsoluteSilenceThreshold;

		/** */
		float RelativeSilenceThreshold;

        /** Default settings */
		FLoudnessAnalyzerSettings()
			: WindowSize(19200)
			, ShortTermSize(30)
			, IntegratedSize(6000)
			, AbsoluteSilenceThreshold(-70.0f)
			, RelativeSilenceThreshold(-10.0f)
		{}
    };

	struct FLoudnessAnalyzerResults
	{
		/** Loudness over 400ms window */
		float Momentary = 0.f;

		/** Loudness over 3000ms Window */
		float ShortTerm = 0.f;
		
		/** Loudness since analyser is started or reseted and over integrated window (600s by default) */
		float Integrated = 0.f;
	};

	/** FLoudnessAnalyzer
	 *
	 * FLoudnessAnalyzer will calculate perceptual energy or loudness one window 
	 * at a time. This module can handle one or more channels at a time. It expects 
	 * interleaved audio samples.
	 */
    class FLoudnessAnalyzer 
    {
        public:
			/** Construct analyzer. */
			CRI_API FLoudnessAnalyzer(float InSampleRate, int32 NumChannels, const FLoudnessAnalyzerSettings& InSettings);
			CRI_API ~FLoudnessAnalyzer();

			/** 
			 * Calculate perceptual energy. 
			 *
			 * InSampleView		  InSampleView is expected to contain interleaved audio with 
			 *                    (InSettings.GrainSize * InNumChannels) number of samples.
			 *
			 * InNumChannels      Number of input channels.
			 *
			 * OutChannelEnergies Will be populated with energies on a per channel basis.
			 *
			 * The overall perceptual energy is returned.
			 */
			CRI_API float CalculatePerceptualEnergy(TArrayView<const float> InSampleView, const int32 InNumChannels, TArray<float>& OutChannelEnergies);

			/**
			 * Calculate momentary, short-term and integrated loudness overall channels. 
			 *
			 * InSampleView		  InSampleView is expected to contain interleaved audio with
			 *                    (InSettings.GrainSize * InNumChannels) number of samples.
			 *
			 * InNumChannels      Number of input channels.
			 *
			 * OutLoudness Will be populated with loudnesses on a per channel basis.
			 *
			 * The overall momentary loudness is returned.
			 */
			CRI_API void CalculateLoudness(TArrayView<const float> InSampleView, const int32 InNumChannels, FLoudnessAnalyzerResults& OutResults);

			/**
			 * Return const reference to settings used inside this analyzer.
			 */
			CRI_API const FLoudnessAnalyzerSettings& GetSettings() const;

			/**
			 * Convert from perceptual energy to loudness [Loudness = 10 * log_10(Energy)]
			 */
			static CRI_API float ConvertPerceptualEnergyToLoudness(float InPerceptualEnergy);

        private:

			float CalculatePerceptualEnergyInternal(TArrayView<const float> InSampleView, int32 ChannelNum);

			FLoudnessAnalyzerSettings Settings;
			int32 NumChannels = 1;

			TArray<float> ChannelWeights;
			Audio::FAlignedFloatBuffer MonoBuffer;
			Audio::TCircularAudioBuffer<float> ShortTermMesures;
			Audio::TCircularAudioBuffer<float> IntegratedMesures;

			float CurrentIntegratedValue = ATOM_MIN_VOLUME_DECIBELS;
			float EnergyScale = 1.f;

			struct FBiquadCoeff;

			FBiquadCoeff* PreFilters;
			FBiquadCoeff* RBLFilters;
    };
}

#undef CRI_API
