
#include "Analyzers/AtomSpectrumAnalysis.h"
#include "Analyzers/AtomSpectrumAnalysisFactory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomSpectrumAnalysis)

namespace
{
	// Convert EFFTSize to int32
	const int32 BPFFTSizeEnumToInt(EAtomFFTSize InFFTSizeEnum)
	{
		switch (InFFTSizeEnum)
		{
		case EAtomFFTSize::Min:
			return 64;
		case EAtomFFTSize::Small:
			return 256;
		case EAtomFFTSize::Medium:
			return 512;
		case EAtomFFTSize::Large:
			return 1024;
		case EAtomFFTSize::VeryLarge:
			return 2048;
		case EAtomFFTSize::Max:
			return 4096;

		case EAtomFFTSize::DefaultSize:
		default:
			return 512;
		};
	}

	// EAudioSpectrumType to Audio::EAtomSpectrumType
	const Atom::ESpectrumType BPSpectrumTypeToAtomSpectrumType(EAtomSpectrumType InSpectrumType)
	{
		switch (InSpectrumType)
		{
		case EAtomSpectrumType::MagnitudeSpectrum:
			return Atom::ESpectrumType::MagnitudeSpectrum;

		case EAtomSpectrumType::Decibel:
			return Atom::ESpectrumType::Decibel;

		case EAtomSpectrumType::PowerSpectrum:
		default:
			return Atom::ESpectrumType::PowerSpectrum;
		};
	}

	// EFFTWindowType to Audio::EAtomFFTWindowType
	const Audio::EWindowType BPWindowTypeToWindowType(EAtomFFTWindowType InSpectrumType)
	{
		switch (InSpectrumType)
		{
		case EAtomFFTWindowType::Hamming:
			return Audio::EWindowType::Hamming;

		case EAtomFFTWindowType::Hann:
			return Audio::EWindowType::Hann;

		case EAtomFFTWindowType::Blackman:
			return Audio::EWindowType::Blackman;

		case EAtomFFTWindowType::None:
		default:
			return Audio::EWindowType::None;
		};
	}
}

TUniquePtr<Atom::IAnalyzerSettings> UAtomSpectrumAnalysisSettings::GetSettings(const int32 InSampleRate, const int32 InNumChannels) const
{
	TUniquePtr<Atom::FSpectrumAnalysisSettings> Settings = MakeUnique<Atom::FSpectrumAnalysisSettings>();

	Settings->AnalysisPeriod = AnalysisPeriod;
	Settings->FFTSize = BPFFTSizeEnumToInt(FFTSize);
	Settings->SpectrumType = BPSpectrumTypeToAtomSpectrumType(SpectrumType);
	Settings->WindowType = BPWindowTypeToWindowType(WindowType);
	Settings->bDownmixToMono = bDownmixToMono;
	return Settings;
}

/*#if WITH_EDITOR
FText UAtomSpectrumAnalysisSettings::GetAssetActionName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AssetSoundAtomSpectrumSettings", "Atom Real-Time Settings (Spectrum)");
}

UClass* UAtomSpectrumAnalysisSettings::GetSupportedClass() const
{
	return UAtomSpectrumAnalysisSettings::StaticClass();
}
#endif*/

UAtomSpectrumAnalyzer::UAtomSpectrumAnalyzer()
{
	Settings = CreateDefaultSubobject<UAtomSpectrumAnalysisSettings>(TEXT("DefaultAtomSpectrumSettings"));
}

TUniquePtr<Atom::IAnalyzerSettings> UAtomSpectrumAnalyzer::GetSettings(const int32 InSampleRate, const int32 InNumChannels) const
{
	TUniquePtr<Atom::IAnalyzerSettings> AnalyzerSettings;

	if (Settings)
	{
		AnalyzerSettings = Settings->GetSettings(InSampleRate, InNumChannels);
	}

	return AnalyzerSettings;
}

static TArray<FAtomSpectrumResults> ConvertToBlueprintResults(UAtomSpectrumAnalysisSettings* Settings, const TArray<Atom::FSpectrumEntry>& InAtomSpectrumArray)
{
	TArray<FAtomSpectrumResults> ResultsArray;

	for (const Atom::FSpectrumEntry& AtomSpectrumEntry : InAtomSpectrumArray)
	{
		FAtomSpectrumResults NewResults;

		NewResults.SpectrumValues = AtomSpectrumEntry.SpectrumValues;
		NewResults.TimeSeconds = AtomSpectrumEntry.Timestamp;

		ResultsArray.Add(NewResults);
	}

	// Sort by time priority (lowest priority/latest first).
	ResultsArray.Sort([](const FAtomSpectrumResults& A, const FAtomSpectrumResults& B) 		
	{ 
		return A.TimeSeconds < B.TimeSeconds;
	});

	return MoveTemp(ResultsArray);
}

void UAtomSpectrumAnalyzer::BroadcastResults()
{
	TUniquePtr<const Atom::FSpectrumResult> AtomSpectrumResults = GetResults<Atom::FSpectrumResult>();
	if (!AtomSpectrumResults.IsValid())
	{
		return;
	}

	int32 NumChannels = AtomSpectrumResults->GetNumChannels();

	if (NumChannels <= 0)
	{
		return;
	}

	bool bIsOnSpectrumResultsBound = OnSpectrumResults.IsBound() || OnSpectrumResultsNative.IsBound();
	bool bIsOnLatestSpectrumResultsBound = OnLatestSpectrumResults.IsBound() || OnLatestSpectrumResultsNative.IsBound();

	if (bIsOnSpectrumResultsBound || bIsOnLatestSpectrumResultsBound)
	{
		for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
		{
			const TArray<Atom::FSpectrumEntry>& AtomSpectrumArray = AtomSpectrumResults->GetChannelSpectrumArray(ChannelIndex);
			if (AtomSpectrumArray.Num() > 0)
			{
				// Sorts results by timestamp
				TArray<FAtomSpectrumResults> Results = ConvertToBlueprintResults(Settings, AtomSpectrumArray);
				check(Results.Num() > 0);

				OnSpectrumResults.Broadcast(ChannelIndex, Results);
				OnSpectrumResultsNative.Broadcast(this, ChannelIndex, Results);

				OnLatestSpectrumResults.Broadcast(ChannelIndex, Results[Results.Num() - 1]);
				OnLatestSpectrumResultsNative.Broadcast(this, ChannelIndex, Results[Results.Num() - 1]);
			}
		}
	}
}

void UAtomSpectrumAnalyzer::GetCenterFrequencies(const float InSampleRate, TArray<float>& OutCenterFrequencies)
{
	const int32 NumFrequencies = GetNumCenterFrequencies();
	if (NumFrequencies == OutCenterFrequencies.Num())
	{
		// Calculate frequencies 0 to Nyquist inclusive
		float BinSize = InSampleRate / BPFFTSizeEnumToInt(Settings->FFTSize);
		for (int i = 0; i < NumFrequencies; ++i)
		{
			OutCenterFrequencies[i] = i * BinSize;
		}
	}
}

const int32 UAtomSpectrumAnalyzer::GetNumCenterFrequencies() const
{
	return BPFFTSizeEnumToInt(Settings->FFTSize) / 2 + 1;
}

FName UAtomSpectrumAnalyzer::GetAnalyzerFactoryName() const
{
	static const FName FactoryName(TEXT("AtomSpectrumAnalysisFactory"));
	return FactoryName;
}
