
#include "Analyzers/AtomTruePeak.h"

#include "Atom/Atom.h"
#include "Analyzers/AtomTruePeakFactory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomTruePeak)

TUniquePtr<Atom::IAnalyzerSettings> UAtomTruePeakSettings::GetSettings(const int32 InSampleRate, const int32 InNumChannels) const
{
	TUniquePtr<Atom::FTruePeakSettings> Settings = MakeUnique<Atom::FTruePeakSettings>();

	Settings->AnalysisPeriod = AnalysisPeriod;

	Settings->PeakHoldTime = PeakHoldTime;
	Settings->ClippingThreshold = ClippingThreshold;

	return Settings;
}

#if WITH_EDITOR
/*FText UAtomTruePeakSettings::GetAssetActionName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AssetSoundSynesthesiaTruePeakSettings", "Synesthesia Real-Time Settings (TruePeak)");
}

UClass* UAtomTruePeakSettings::GetSupportedClass() const
{
	return UAtomTruePeakSettings::StaticClass();
}*/
#endif

UAtomTruePeakAnalyzer::UAtomTruePeakAnalyzer()
{
	Settings = CreateDefaultSubobject<UAtomTruePeakSettings>(TEXT("DefaultTruePeakSettings"));
}

TUniquePtr<Atom::IAnalyzerSettings> UAtomTruePeakAnalyzer::GetSettings(const int32 InSampleRate, const int32 InNumChannels) const
{
	TUniquePtr<Atom::IAnalyzerSettings> AnalyzerSettings;

	if (Settings)
	{
		AnalyzerSettings = Settings->GetSettings(InSampleRate, InNumChannels);
	}

	return AnalyzerSettings;
}

static TArray<FAtomTruePeakResults> ConvertToBlueprintResults(UAtomTruePeakSettings* Settings, const TArray<Atom::FTruePeakEntry>& InTruePeakArray)
{
	TArray<FAtomTruePeakResults> ResultsArray;

	for (const Atom::FTruePeakEntry& TruePeakEntry : InTruePeakArray)
	{
		FAtomTruePeakResults NewResults;

		NewResults.PeakValue = Atom::ConvertToDecibels(TruePeakEntry.PeakValue);
		NewResults.PeakHoldValue = Atom::ConvertToDecibels(TruePeakEntry.PeakHoldValue);
		NewResults.ClippingValue = Atom::ConvertToDecibels(TruePeakEntry.ClippingValue);
		NewResults.NumSamplesClipping = TruePeakEntry.NumSamplesClipping;
		NewResults.TimeSeconds = TruePeakEntry.Timestamp;

		ResultsArray.Add(NewResults);
	}

	// Sort by priority (lowest priority first).
	ResultsArray.Sort([](const FAtomTruePeakResults& A, const FAtomTruePeakResults& B) 		
	{ 
		return A.TimeSeconds < B.TimeSeconds;
	});

	return MoveTemp(ResultsArray);
}

void UAtomTruePeakAnalyzer::BroadcastResults()
{
	TUniquePtr<const Atom::FTruePeakResult> TruePeakResults = GetResults<Atom::FTruePeakResult>();
	if (!TruePeakResults.IsValid())
	{
		return;
	}

	int32 NumChannels = TruePeakResults->GetNumChannels();

	if (NumChannels > 0)
	{
		bool bIsOnOverallTruePeakResultsBound = OnOverallTruePeakResults.IsBound() || OnOverallTruePeakResultsNative.IsBound();
		bool bIsOnLatestOverallTruePeakResultsBound = OnLatestOverallTruePeakResults.IsBound() || OnLatestOverallTruePeakResultsNative.IsBound();
		if (bIsOnOverallTruePeakResultsBound || bIsOnLatestOverallTruePeakResultsBound)
		{
			const TArray<Atom::FTruePeakEntry>& OverallTruePeakArray = TruePeakResults->GetTruePeakArray();
			if (OverallTruePeakArray.Num() > 0)
			{
				TArray<FAtomTruePeakResults> Results = ConvertToBlueprintResults(Settings, OverallTruePeakArray);
				check(Results.Num() > 0);

				OnOverallTruePeakResults.Broadcast(Results);
				OnOverallTruePeakResultsNative.Broadcast(this, Results);

				FAtomTruePeakResults& Latest = Results[Results.Num() - 1];
				OnLatestOverallTruePeakResults.Broadcast(Latest);
				OnLatestOverallTruePeakResultsNative.Broadcast(this, Latest);

			}
		}

		bool bIsOnPerChannelTruePeakResultsBound = OnPerChannelTruePeakResults.IsBound() || OnPerChannelTruePeakResultsNative.IsBound();
		bool bIsOnLatestPerChannelTruePeakResultsBound = OnLatestPerChannelTruePeakResults.IsBound() || OnLatestPerChannelTruePeakResultsNative.IsBound();

		if (bIsOnPerChannelTruePeakResultsBound || bIsOnLatestPerChannelTruePeakResultsBound)
		{
			for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
			{
				const TArray<Atom::FTruePeakEntry>& TruePeakArray = TruePeakResults->GetChannelTruePeakArray(ChannelIndex);
				if (TruePeakArray.Num() > 0)
				{
					TArray<FAtomTruePeakResults> Results = ConvertToBlueprintResults(Settings, TruePeakArray);
					check(Results.Num() > 0);

					OnPerChannelTruePeakResults.Broadcast(ChannelIndex, Results);
					OnPerChannelTruePeakResultsNative.Broadcast(this, ChannelIndex, Results);

					OnLatestPerChannelTruePeakResults.Broadcast(ChannelIndex, Results[Results.Num() - 1]);
					OnLatestPerChannelTruePeakResultsNative.Broadcast(this, ChannelIndex, Results[Results.Num() - 1]);
				}
			}
		}
	}
}

FName UAtomTruePeakAnalyzer::GetAnalyzerFactoryName() const
{
	static const FName FactoryName(TEXT("AtomTruePeakFactory"));
	return FactoryName;
}
