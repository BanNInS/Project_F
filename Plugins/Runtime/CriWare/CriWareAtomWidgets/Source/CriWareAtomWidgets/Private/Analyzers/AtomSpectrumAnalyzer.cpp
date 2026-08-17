
#include "Analyzers/AtomSpectrumAnalyzer.h"

#include "DSP/FloatArrayMath.h"

namespace Atom
{
	FSpectrumAnalyzer::FSpectrumAnalyzer(float InSampleRate, const FSpectrumAnalyzerSettings& InSettings)
	: Settings(InSettings)
	, SampleRate(InSampleRate)
	, Window(InSettings.WindowType, InSettings.FFTSize, /*NumChannels=*/1, /*bIsPeriodic=*/false)
	{
		if (!ensure(SampleRate > 0.f))
		{
			SampleRate = 48000.f;
		}

		// Ranges from EFFTSize
		static const int32 MinLog2FFTSize = 6; 
		static const int32 MaxLog2FFTSize = 16; 

		// Create FFT
		Audio::FFFTSettings FFTSettings;

		int32 Log2FFTSize = MinLog2FFTSize; 
		while ((Settings.FFTSize > (1 << Log2FFTSize)) && (Log2FFTSize < MaxLog2FFTSize))
		{
			Log2FFTSize++;
		}

		FFTSettings.Log2Size = Log2FFTSize;
		FFTSettings.bArrays128BitAligned = true;
		FFTSettings.bEnableHardwareAcceleration = true;

		FFT = Audio::FFFTFactory::NewFFTAlgorithm(FFTSettings);
		check(FFT.IsValid());

		// Calculate forward scaling factor to apply to power spectrum
		switch (FFT->ForwardScaling())
		{
		case Audio::EFFTScaling::MultipliedByFFTSize:
			FFTScaling = 1.f / (Settings.FFTSize * Settings.FFTSize);
			break;

		case Audio::EFFTScaling::MultipliedBySqrtFFTSize:
			FFTScaling = 1.f / Settings.FFTSize;
			break;

		case Audio::EFFTScaling::DividedByFFTSize:
			FFTScaling = Settings.FFTSize * Settings.FFTSize;
			break;

		case Audio::EFFTScaling::DividedBySqrtFFTSize:
			FFTScaling = Settings.FFTSize;
			break;

		case Audio::EFFTScaling::None:
		default:
			FFTScaling = 1.f;
			break;
		}

		WindowedBuffer.SetNumUninitialized(FFT->NumInputFloats());
		FFTOutput.SetNumUninitialized(FFT->NumOutputFloats());
	}

	void FSpectrumAnalyzer::ProcessAudio(TArrayView<const float> InSampleView, TArrayView<float> OutSpectrum)
	{
		if (!ensure(InSampleView.Num() == FFT->NumInputFloats()) || !ensure(OutSpectrum.Num() == FFT->NumOutputFloats() / 2))
		{
			return;
		}
		
		// Copy buffer and apply window
		FMemory::Memcpy(WindowedBuffer.GetData(), InSampleView.GetData(), sizeof(float) * Settings.FFTSize);
		Window.ApplyToBuffer(WindowedBuffer.GetData());

		// Perform FFT and convert to power 
		FFT->ForwardRealToComplex(WindowedBuffer.GetData(), FFTOutput.GetData());
		Audio::ArrayComplexToPower(FFTOutput, OutSpectrum);
		Audio::ArrayMultiplyByConstantInPlace(OutSpectrum, FFTScaling);

		switch (Settings.SpectrumType)
		{
			case ESpectrumType::MagnitudeSpectrum:
				Audio::ArraySqrtInPlace(OutSpectrum);
				break;

			// Case already covered by complex to power above 
			case ESpectrumType::PowerSpectrum:
				break;

			case ESpectrumType::Decibel:
				Audio::ArrayPowerToDecibelInPlace(OutSpectrum, -90.f);
				break;

			default:
				checkf(false, TEXT("Unhandled EAtomSpectrumType"));
		}
	}

	const FSpectrumAnalyzerSettings& FSpectrumAnalyzer::GetSettings() const
	{
		return Settings;
	}
}
