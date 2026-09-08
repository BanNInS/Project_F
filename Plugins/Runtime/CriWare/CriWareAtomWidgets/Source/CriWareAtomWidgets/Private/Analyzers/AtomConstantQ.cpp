
#include "Analyzers/AtomConstantQ.h"
#include "Analyzers/AtomConstantQFactory.h"

#include "Atom/AtomBus.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomConstantQ)

namespace AtomConstantQPrivate
{
	/** Convenience template function for converting from blueprint types to audio types */
	template<typename InType, typename OutType>
	OutType ConvertType(const TMap<InType, OutType>& InMap, const InType& InValue, const OutType& InDefaultValue)
	{
		checkf(InMap.Contains(InValue), TEXT("Unhandled value conversion"));

		if (InMap.Contains(InValue))
		{
			return InMap[InValue];
		}

		return InDefaultValue;
	}


	// EAtomConstantQFFTSizeEnum to int
	int32 BPConstantQFFTSizeEnumToInt(const EAtomConstantQFFTSizeEnum InFFTSizeEnum)
	{
		static const TMap<EAtomConstantQFFTSizeEnum, int32> FFTSizeMap =
		{
			{EAtomConstantQFFTSizeEnum::Min, 		64},
			{EAtomConstantQFFTSizeEnum::XXSmall, 	128},
			{EAtomConstantQFFTSizeEnum::XSmall, 	256},
			{EAtomConstantQFFTSizeEnum::Small,		512},
			{EAtomConstantQFFTSizeEnum::Medium, 	1024},
			{EAtomConstantQFFTSizeEnum::Large,		2048},
			{EAtomConstantQFFTSizeEnum::XLarge, 	4096},
			{EAtomConstantQFFTSizeEnum::XXLarge, 	8192},
			{EAtomConstantQFFTSizeEnum::Max, 		16384}
		};

		return ConvertType(FFTSizeMap, InFFTSizeEnum, 2048);

	}

	// EAtomFFTWindowType to Audio::EWindowType
	Audio::EWindowType BPWindowTypeToAudioWindowType(const EAtomFFTWindowType InWindowType)
	{
		static const TMap<EAtomFFTWindowType, Audio::EWindowType> WindowTypeMap =
		{
			{EAtomFFTWindowType::None, 		Audio::EWindowType::None},
			{EAtomFFTWindowType::Hamming, 	Audio::EWindowType::Hamming},
			{EAtomFFTWindowType::Hann, 		Audio::EWindowType::Hann},
			{EAtomFFTWindowType::Blackman,	Audio::EWindowType::Blackman}
		};

		return ConvertType(WindowTypeMap, InWindowType, Audio::EWindowType::None);
	}

	// EAtomSpectrumType to Audio::ESpectrumType
	Audio::ESpectrumType BPSpectrumTypeToAudioSpectrumType(const EAtomSpectrumType InSpectrumType)
	{
		static const TMap<EAtomSpectrumType, Audio::ESpectrumType> SpectrumTypeMap =
		{
			{EAtomSpectrumType::MagnitudeSpectrum,	Audio::ESpectrumType::MagnitudeSpectrum},
			{EAtomSpectrumType::PowerSpectrum, 	Audio::ESpectrumType::PowerSpectrum}
		};

		return ConvertType(SpectrumTypeMap, InSpectrumType, Audio::ESpectrumType::PowerSpectrum);
	}

	// EAtomConstantQNormalizationEnum to Audio::EPseudoConstantQNormalization
	Audio::EPseudoConstantQNormalization BPCQTNormalizationToAudioCQTNormalization(const EAtomConstantQNormalizationEnum InNormalization)
	{
		static const TMap<EAtomConstantQNormalizationEnum, Audio::EPseudoConstantQNormalization> NormalizationMap =
		{
			{EAtomConstantQNormalizationEnum::EqualAmplitude, 		Audio::EPseudoConstantQNormalization::EqualAmplitude},
			{EAtomConstantQNormalizationEnum::EqualEuclideanNorm,	Audio::EPseudoConstantQNormalization::EqualEuclideanNorm},
			{EAtomConstantQNormalizationEnum::EqualEnergy, 			Audio::EPseudoConstantQNormalization::EqualEnergy}
		};

		return ConvertType(NormalizationMap, InNormalization, Audio::EPseudoConstantQNormalization::EqualEnergy);
	}
}


/** Convert UConstantQSettings to FConstantQSettings */
TUniquePtr<Atom::IAnalyzerSettings> UAtomConstantQSettings::GetSettings(const float InSampleRate, const int32 InNumChannels) const
{
	using namespace AtomConstantQPrivate;

	TUniquePtr<Atom::FConstantQSettings> Settings = MakeUnique<Atom::FConstantQSettings>();

	Settings->AnalysisPeriodInSeconds = AnalysisPeriodInSeconds;
	Settings->bDownmixToMono = bDownmixToMono;
	Settings->FFTSize = BPConstantQFFTSizeEnumToInt(FFTSize);
	Settings->WindowType = BPWindowTypeToAudioWindowType(WindowType);
	Settings->SpectrumType = BPSpectrumTypeToAudioSpectrumType(SpectrumType);
	Settings->NumBands = NumBands;
	Settings->NumBandsPerOctave = NumBandsPerOctave;
	Settings->KernelLowestCenterFreq = StartingFrequencyHz;
	Settings->BandWidthStretch = BandWidthStretch;
	Settings->Normalization = BPCQTNormalizationToAudioCQTNormalization(CQTNormalization);
	Settings->Scaling = (SpectrumType == EAtomSpectrumType::Decibel) ? Atom::EConstantQScaling::Decibel : Atom::EConstantQScaling::Linear;

	return Settings;
}

/*#if WITH_EDITOR
FText UAtomConstantQSettings::GetAssetActionName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AssetSoundSynesthesiaConstantQSettings", "Synesthesia Real-Time Settings (ConstantQ)");
}

UClass* UAtomConstantQSettings::GetSupportedClass() const
{
	return UAtomConstantQSettings::StaticClass();
}
#endif // WITH_EDITOR*/

UAtomConstantQAnalyzer::UAtomConstantQAnalyzer()
{
	Settings = CreateDefaultSubobject<UAtomConstantQSettings>(TEXT("DefaultConstantQSettings"));
}

TUniquePtr<Atom::IAnalyzerSettings> UAtomConstantQAnalyzer::GetSettings(const int32 InSampleRate, const int32 InNumChannels) const
{
	TUniquePtr<Atom::IAnalyzerSettings> AnalyzerSettings;

	if (Settings)
	{
		AnalyzerSettings = Settings->GetSettings(InSampleRate, InNumChannels);
	}

	return AnalyzerSettings;
}

static TArray<FAtomConstantQResults> ConvertToBlueprintResults(UAtomConstantQSettings* Settings, const TArray<Atom::FConstantQFrame>& InConstantQFrameArray)
{
	TArray<FAtomConstantQResults> ResultsArray;

	for (const Atom::FConstantQFrame& ConstantQFrame : InConstantQFrameArray)
	{
		FAtomConstantQResults NewResults;

		NewResults.SpectrumValues = ConstantQFrame.Spectrum;
		NewResults.TimeSeconds = ConstantQFrame.Timestamp;

		ResultsArray.Add(NewResults);
	}

	// Sort by time priority (lowest priority/latest first).
	ResultsArray.Sort([](const FAtomConstantQResults& A, const FAtomConstantQResults& B)
		{
			return A.TimeSeconds < B.TimeSeconds;
		});

	return MoveTemp(ResultsArray);
}

void UAtomConstantQAnalyzer::BroadcastResults()
{
	TUniquePtr<const Atom::FConstantQResult> ConstantQResults = GetResults<Atom::FConstantQResult>();
	if (!ConstantQResults.IsValid())
	{
		return;
	}

	const int32 NumChannels = ConstantQResults->GetNumChannels();

	if (NumChannels <= 0)
	{
		return;
	}

	bool bIsOnConstantQResultsBound = OnConstantQResults.IsBound() || OnConstantQResultsNative.IsBound();
	bool bIsOnLatestConstantQResultsBound = OnLatestConstantQResults.IsBound() || OnLatestConstantQResultsNative.IsBound();

	if (bIsOnConstantQResultsBound || bIsOnLatestConstantQResultsBound)
	{
		for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
		{
			const TArray<Atom::FConstantQFrame>& ConstantQFrameArray = ConstantQResults->GetFramesForChannel(ChannelIndex);
			if (ConstantQFrameArray.Num() > 0)
			{
				// Sorts results by timestamp
				TArray<FAtomConstantQResults> Results = ConvertToBlueprintResults(Settings, ConstantQFrameArray);
				ensure(Results.Num() > 0);

				OnConstantQResults.Broadcast(ChannelIndex, Results);
				OnConstantQResultsNative.Broadcast(this, ChannelIndex, Results);

				OnLatestConstantQResults.Broadcast(ChannelIndex, Results[Results.Num() - 1]);
				OnLatestConstantQResultsNative.Broadcast(this, ChannelIndex, Results[Results.Num() - 1]);
			}
		}
	}
}

void UAtomConstantQAnalyzer::GetCenterFrequencies(TArray<float>& OutCenterFrequencies)
{
	const int32 NumFrequencies = GetNumCenterFrequencies();
	if (NumFrequencies == OutCenterFrequencies.Num())
	{
		const float PitchStep = FMath::Pow(2.0f, 1.0f / Settings->NumBandsPerOctave);

		float CurrentFrequency = Settings->StartingFrequencyHz;
		for (int32 i = 0; i < NumFrequencies; ++i)
		{
			OutCenterFrequencies[i] = CurrentFrequency;
			CurrentFrequency *= PitchStep;
		}
	}
}

const int32 UAtomConstantQAnalyzer::GetNumCenterFrequencies() const
{
	return Settings->NumBands;
}

FName UAtomConstantQAnalyzer::GetAnalyzerFactoryName() const
{
	static const FName FactoryName(TEXT("AtomConstantQFactory"));
	return FactoryName;
}
