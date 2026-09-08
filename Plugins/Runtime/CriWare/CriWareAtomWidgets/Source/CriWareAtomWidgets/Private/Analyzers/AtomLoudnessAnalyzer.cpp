
#include "Analyzers/AtomLoudnessAnalyzer.h"

#include "DSP/DeinterleaveView.h"
#include "DSP/FloatArrayMath.h"

#include "CriWareDefines.h"
#include "Atom/Mixer/AtomMixer.h"

namespace AtomLoudnessAnalyzerPrivate
{
	static const float Log10Scale = 1.0f / FMath::Loge(10.f);
}

namespace Atom
{
	// Simple biquad filter structure handling a biquad formulation
	// See: https://en.wikipedia.org/wiki/Digital_biquad_filter
	// Calculations of coefficients are handled outside this class.
	// Filter coefficients are public and are intended to be used externally.
	struct FLoudnessAnalyzer::FBiquadCoeff
	{
	public:

		FBiquadCoeff()
			: A0(1.0f)
			, A1(0.0f)
			, A2(0.0f)
			, B1(0.0f)
			, B2(0.0f)
		{
			Reset();
		}

		// Reset the filter (flush delays)
		void Reset()
		{
			X_Z1 = 0.0f;
			X_Z2 = 0.0f;
			Y_Z1 = 0.0f;
			Y_Z2 = 0.0f;
		}

		FORCEINLINE float ProcessAudio(const float InSample)
		{
			// Use the biquad difference eq: y(n) = a0*x(n) + a1*x(n-1) + a2*x(n-2) - b1*y(n-1) - b2*y(n-2) 
			const float Output = A0 * InSample + A1 * X_Z1 + A2 * X_Z2 - B1 * Y_Z1 - B2 * Y_Z2;

			// Apply the z-transforms
			Y_Z2 = Y_Z1;
			Y_Z1 = Output;

			X_Z2 = X_Z1;
			X_Z1 = InSample;

			return Output;
		}

		// Biquad filter coefficients
		float A0;
		float A1;
		float A2;
		float B1;
		float B2;

	private:

		float X_Z1; // previous inputs z-transforms
		float X_Z2;
		float Y_Z1; // prvious outputs z-transforms
		float Y_Z2;
	};

	/**
	 * FLoudnessAnalyzer class
	 */
	FLoudnessAnalyzer::FLoudnessAnalyzer(float InSampleRate, int32 InNumChannels, const FLoudnessAnalyzerSettings& InSettings)
	{
		Settings = InSettings;
		NumChannels = InNumChannels;

		// Initialize all channel weights to default to 1.0f
		ChannelWeights.Init(1.0f, Atom::MaxSupportedSpeakers);

		// If elevation angle is less than 30 degrees and azimuth is between 60 and 120, set channel weight to 1.41. (Rec. ITU-R BS.1770-2)
		ChannelWeights[(int32)Atom::EMixerSpeaker::SurroundLeft] = 1.41f;
		ChannelWeights[(int32)Atom::EMixerSpeaker::SurroundRight] = 1.41f;
		//ChannelWeights[(int32)Atom::EMixerSpeaker::LowFrequency] = 0.0f;

		MonoBuffer.Reset(InSettings.WindowSize);
		MonoBuffer.AddUninitialized(InSettings.WindowSize);

		ShortTermMesures.SetCapacity(InSettings.ShortTermSize);
		IntegratedMesures.SetCapacity(InSettings.IntegratedSize);

		EnergyScale = 1.f / static_cast<float>(Settings.WindowSize);

		PreFilters = new FBiquadCoeff[NumChannels];
		{
			constexpr float A0 = 1.53512485958697f;
			constexpr float A1 = -2.69169618940638f;
			constexpr float A2 = 1.19839281085285f;
			constexpr float B1 = -1.69065929318241f;
			constexpr float B2 = 0.73248077421585f;

			for (int ChannelNum = 0; ChannelNum < NumChannels; ++ChannelNum)
			{
				FBiquadCoeff& Filter = PreFilters[ChannelNum];
				Filter.A0 = A0;
				Filter.A1 = A1;
				Filter.A2 = A2;
				Filter.B1 = B1;
				Filter.B2 = B2;
			};
		}

		RBLFilters = new FBiquadCoeff[NumChannels];
		{
			constexpr float A0 = 1.f;
			constexpr float A1 = -2.f;
			constexpr float A2 = 1.f;
			constexpr float B1 = -1.99004745483398f;
			constexpr float B2 = 0.99007225036621f;

			for (int ChannelNum = 0; ChannelNum < NumChannels; ++ChannelNum)
			{
				FBiquadCoeff& Filter = RBLFilters[ChannelNum];
				Filter.A0 = A0;
				Filter.A1 = A1;
				Filter.A2 = A2;
				Filter.B1 = B1;
				Filter.B2 = B2;
			};
		}
	}

	FLoudnessAnalyzer::~FLoudnessAnalyzer()
	{
		if (PreFilters)
		{
			delete[] PreFilters;
		}
		if (RBLFilters)
		{
			delete[] RBLFilters;
		}
	}

	float FLoudnessAnalyzer::CalculatePerceptualEnergy(TArrayView<const float> InView, const int32 InNumChannels, TArray<float>& OutChannelEnergys)
	{
		check(InNumChannels == NumChannels);
		check(InView.Num() == (InNumChannels * Settings.WindowSize));

		OutChannelEnergys.Reset(InNumChannels);
		OutChannelEnergys.AddUninitialized(InNumChannels);

		// Deinterleave audio and send to mono loudness analyzer.
		Audio::TAutoDeinterleaveView<float, Audio::FAudioBufferAlignedAllocator> DeinterleaveView(InView, MonoBuffer, InNumChannels);
		for (Audio::TAutoDeinterleaveView<float>::TChannel<Audio::FAudioBufferAlignedAllocator> Channel : DeinterleaveView)
		{
			OutChannelEnergys[Channel.ChannelIndex] = CalculatePerceptualEnergyInternal(Channel.Values, Channel.ChannelIndex);
		}

		// Combine channel energies into overall perceptual energy
		float Magnitude = 0.0f;
		for (int32 ChannelNum = 0; ChannelNum < InNumChannels; ChannelNum++)
		{
			Magnitude += ChannelWeights[ChannelNum] * OutChannelEnergys[ChannelNum];
		}

		return Magnitude;
	}

	float FLoudnessAnalyzer::CalculatePerceptualEnergyInternal(TArrayView<const float> InView, int32 ChannelNum)
	{
		float Total = 0.f;

		for (const float Sample : InView)
		{
			const float A = PreFilters[ChannelNum].ProcessAudio(Sample);
			const float B = RBLFilters[ChannelNum].ProcessAudio(A);

			Total += B * B;
		}

		// Normalize by number of samples to calculate mean squared value.
		Total *= EnergyScale;

		return Total;
	}

	void FLoudnessAnalyzer::CalculateLoudness(TArrayView<const float> InView, const int32 InNumChannels, FLoudnessAnalyzerResults& OutResults)
	{
		TArray<float> PerChannelResults;
		PerChannelResults.AddZeroed(InNumChannels + 1);
		float MomentaryEnergy = CalculatePerceptualEnergy(InView, InNumChannels, PerChannelResults);

		// short-term average - momentary values over 3 seconds
		if (ShortTermMesures.Remainder() == 0)
		{
			ShortTermMesures.Pop();
		}

		ShortTermMesures.Push(MomentaryEnergy);

		TArray<float> OutShortTerm;
		OutShortTerm.AddUninitialized(ShortTermMesures.Num());
		ShortTermMesures.Peek(OutShortTerm.GetData(), ShortTermMesures.Num());

		const float ShortTermEnergy = Audio::ArrayGetAverageValue(OutShortTerm);

		const float AbsoluteThreshold = Settings.AbsoluteSilenceThreshold;
		const float RelativeThreshold = Settings.RelativeSilenceThreshold;

		// gate out -70LUFS
		if (ConvertPerceptualEnergyToLoudness(MomentaryEnergy) >= AbsoluteThreshold)
		{
			if (IntegratedMesures.Remainder() == 0)
			{
				IntegratedMesures.Pop();
			}

			IntegratedMesures.Push(MomentaryEnergy);

			TArray<float> OutIntegrated;
			OutIntegrated.AddUninitialized(IntegratedMesures.Num());
			IntegratedMesures.Peek(OutIntegrated.GetData(), IntegratedMesures.Num());

			const float CurrentGatedValue = Audio::ArrayGetAverageValue(OutIntegrated);

			// apply average with floating gate
			float Sum = 0.0f;
			int32 Num = 0;
			const float Gate = FMath::Max(ConvertPerceptualEnergyToLoudness(CurrentGatedValue) + RelativeThreshold, AbsoluteThreshold);
			for (float Value : OutIntegrated)
			{
				if (ConvertPerceptualEnergyToLoudness(Value) >= Gate)
				{
					Sum += Value;
					Num++;
				}
			}

			if (Num > 0)
			{
				CurrentIntegratedValue = ConvertPerceptualEnergyToLoudness(Sum / Num);
			}
		}

		OutResults.Momentary = ConvertPerceptualEnergyToLoudness(MomentaryEnergy);
		OutResults.ShortTerm = ConvertPerceptualEnergyToLoudness(ShortTermEnergy);
		OutResults.Integrated = CurrentIntegratedValue;
	}

    const FLoudnessAnalyzerSettings& FLoudnessAnalyzer::GetSettings() const
    {
        return Settings;
    }

	float FLoudnessAnalyzer::ConvertPerceptualEnergyToLoudness(float InPerceptualEnergy)
	{
		return FMath::Max(-0.691f + (10.0f * FMath::Loge(InPerceptualEnergy) * AtomLoudnessAnalyzerPrivate::Log10Scale), ATOM_MIN_VOLUME_DECIBELS);
	}
}

