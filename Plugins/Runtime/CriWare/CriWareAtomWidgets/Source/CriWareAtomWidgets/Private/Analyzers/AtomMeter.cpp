
#include "Analyzers/AtomMeter.h"

#include "Atom/Atom.h"
#include "Analyzers/AtomMeterFactory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomMeter)

TUniquePtr<Atom::IAnalyzerSettings> UAtomMeterSettings::GetSettings(const int32 InSampleRate, const int32 InNumChannels) const
{
	TUniquePtr<Atom::FMeterSettings> Settings = MakeUnique<Atom::FMeterSettings>();

	Settings->AnalysisPeriod = AnalysisPeriod;

	switch (PeakMode)
	{
	case EAtomMeterPeakType::MeanSquared:
		Settings->MeterPeakMode = Audio::EPeakMode::MeanSquared;
		break;

	case EAtomMeterPeakType::RootMeanSquared:
		Settings->MeterPeakMode = Audio::EPeakMode::RootMeanSquared;
		break;

	default:
	case EAtomMeterPeakType::Peak:
		Settings->MeterPeakMode = Audio::EPeakMode::Peak;
		break;
	}

	Settings->MeterAttackTime = MeterAttackTime;
	Settings->MeterReleaseTime = MeterReleaseTime;
	Settings->PeakHoldTime = PeakHoldTime;
	Settings->ClippingThreshold = ClippingThreshold;

	return Settings;
}

#if WITH_EDITOR
/*FText UAtomMeterSettings::GetAssetActionName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AssetSoundSynesthesiaMeterSettings", "Synesthesia Real-Time Settings (Meter)");
}

UClass* UAtomMeterSettings::GetSupportedClass() const
{
	return UAtomMeterSettings::StaticClass();
}*/
#endif

UAtomMeterAnalyzer::UAtomMeterAnalyzer()
{
	Settings = CreateDefaultSubobject<UAtomMeterSettings>(TEXT("DefaultMeterSettings"));
}

TUniquePtr<Atom::IAnalyzerSettings> UAtomMeterAnalyzer::GetSettings(const int32 InSampleRate, const int32 InNumChannels) const
{
	TUniquePtr<Atom::IAnalyzerSettings> AnalyzerSettings;

	if (Settings)
	{
		AnalyzerSettings = Settings->GetSettings(InSampleRate, InNumChannels);
	}

	return AnalyzerSettings;
}

static TArray<FAtomMeterResults> ConvertToBlueprintResults(UAtomMeterSettings* Settings, const TArray<Atom::FMeterEntry>& InMeterArray)
{
	TArray<FAtomMeterResults> ResultsArray;

	for (const Atom::FMeterEntry& MeterEntry : InMeterArray)
	{
		FAtomMeterResults NewResults;

		NewResults.MeterValue = Atom::ConvertToDecibels(MeterEntry.MeterValue);
		NewResults.PeakValue = Atom::ConvertToDecibels(MeterEntry.PeakValue);
		NewResults.ClippingValue = Atom::ConvertToDecibels(MeterEntry.ClippingValue);
		NewResults.NumSamplesClipping = MeterEntry.NumSamplesClipping;
		NewResults.TimeSeconds = MeterEntry.Timestamp;

		ResultsArray.Add(NewResults);
	}

	// Sort by priority (lowest priority first).
	ResultsArray.Sort([](const FAtomMeterResults& A, const FAtomMeterResults& B) 		
	{ 
		return A.TimeSeconds < B.TimeSeconds;
	});

	return MoveTemp(ResultsArray);
}

void UAtomMeterAnalyzer::BroadcastResults()
{
	TUniquePtr<const Atom::FMeterResult> MeterResults = GetResults<Atom::FMeterResult>();
	if (!MeterResults.IsValid())
	{
		return;
	}

	int32 NumChannels = MeterResults->GetNumChannels();

	if (NumChannels > 0)
	{
		bool bIsOnOverallMeterResultsBound = OnOverallMeterResults.IsBound() || OnOverallMeterResultsNative.IsBound();
		bool bIsOnLatestOverallMeterResultsBound = OnLatestOverallMeterResults.IsBound() || OnLatestOverallMeterResultsNative.IsBound();
		if (bIsOnOverallMeterResultsBound || bIsOnLatestOverallMeterResultsBound)
		{
			const TArray<Atom::FMeterEntry>& OverallMeterArray = MeterResults->GetMeterArray();
			if (OverallMeterArray.Num() > 0)
			{
				TArray<FAtomMeterResults> Results = ConvertToBlueprintResults(Settings, OverallMeterArray);
				check(Results.Num() > 0);

				OnOverallMeterResults.Broadcast(Results);
				OnOverallMeterResultsNative.Broadcast(this, Results);

				FAtomMeterResults& Latest = Results[Results.Num() - 1];
				OnLatestOverallMeterResults.Broadcast(Latest);
				OnLatestOverallMeterResultsNative.Broadcast(this, Latest);

			}
		}

		bool bIsOnPerChannelMeterResultsBound = OnPerChannelMeterResults.IsBound() || OnPerChannelMeterResultsNative.IsBound();
		bool bIsOnLatestPerChannelMeterResultsBound = OnLatestPerChannelMeterResults.IsBound() || OnLatestPerChannelMeterResultsNative.IsBound();

		if (bIsOnPerChannelMeterResultsBound || bIsOnLatestPerChannelMeterResultsBound)
		{
			for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
			{
				const TArray<Atom::FMeterEntry>& MeterArray = MeterResults->GetChannelMeterArray(ChannelIndex);
				if (MeterArray.Num() > 0)
				{
					TArray<FAtomMeterResults> Results = ConvertToBlueprintResults(Settings, MeterArray);
					check(Results.Num() > 0);

					OnPerChannelMeterResults.Broadcast(ChannelIndex, Results);
					OnPerChannelMeterResultsNative.Broadcast(this, ChannelIndex, Results);

					OnLatestPerChannelMeterResults.Broadcast(ChannelIndex, Results[Results.Num() - 1]);
					OnLatestPerChannelMeterResultsNative.Broadcast(this, ChannelIndex, Results[Results.Num() - 1]);
				}
			}
		}
	}
}

FName UAtomMeterAnalyzer::GetAnalyzerFactoryName() const
{
	static const FName FactoryName(TEXT("AtomMeterFactory"));
	return FactoryName;
}
