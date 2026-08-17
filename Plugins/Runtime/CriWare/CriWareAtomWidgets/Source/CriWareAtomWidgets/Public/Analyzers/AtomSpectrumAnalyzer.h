// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DSP/AudioFFT.h"

#define CRI_API CRIWAREATOMWIDGETS_API

// Forward Declarations
namespace Audio
{ 
	class IFFTAlgorithm;
}

namespace Atom
{
	enum class ESpectrumType : uint8
	{
		// Spectrum frequency values are equal to magnitude of frequency.
		MagnitudeSpectrum,

		// Spectrum frequency values are equal to magnitude squared.
		PowerSpectrum,

		// Returns decibels with a noise floor of -90
		Decibel,
	};

	struct FSpectrumAnalyzerSettings 
	{	
		int32 FFTSize = 512;

		ESpectrumType SpectrumType = ESpectrumType::PowerSpectrum;
		
		Audio::EWindowType WindowType = Audio::EWindowType::None;
		
		bool bDownmixToMono = true;
	};

	class FSpectrumAnalyzer
	{
	public:

		/** Construct analyzer */
		CRI_API FSpectrumAnalyzer(float InSampleRate, const FSpectrumAnalyzerSettings& InSettings);

		/**
		 * Calculate the spectrum values for the input samples. OutSpectrum should have (num input samples / 2) + 1 values. 
		 */
		CRI_API void ProcessAudio(TArrayView<const float> InSampleView, TArrayView<float> OutSpectrum);

		/**
		 * Return const reference to settings used inside this analyzer.
		 */
		CRI_API const FSpectrumAnalyzerSettings& GetSettings() const;

	protected:
		FSpectrumAnalyzerSettings Settings;
		float SampleRate = 0.0f;
		float FFTScaling = 0.0f;

		Audio::FWindow Window;
		Audio::FAlignedFloatBuffer WindowedBuffer;
		Audio::FAlignedFloatBuffer FFTOutput;
		TUniquePtr<Audio::IFFTAlgorithm> FFT;
	};
}

#undef CRI_API
