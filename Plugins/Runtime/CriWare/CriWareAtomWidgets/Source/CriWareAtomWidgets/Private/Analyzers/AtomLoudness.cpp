
#include "Analyzers/AtomLoudness.h"
#include "Analyzers/AtomLoudnessFactory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomLoudness)

TUniquePtr<Atom::IAnalyzerSettings> UAtomLoudnessSettings::GetSettings(const int32 InSampleRate, const int32 InNumChannels) const
{
	TUniquePtr<Atom::FLoudnessSettings> Settings = MakeUnique<Atom::FLoudnessSettings>();

	Settings->AnalysisPeriod = AnalysisPeriod;
	Settings->WindowSize = FMath::CeilToInt(InSampleRate * MomentarySize);
	Settings->ShortTermSize = FMath::CeilToInt(ShortTermSize / AnalysisPeriod);
	Settings->IntegratedSize = FMath::CeilToInt(MaxIntegratedSize / AnalysisPeriod);
	Settings->AbsoluteSilenceThreshold = AbsoluteSilenceThreshold;
	Settings->RelativeSilenceThreshold = RelativeSilenceThreshold;

	return Settings;
}

/*#if WITH_EDITOR
FText UAtomLoudnessSettings::GetAssetActionName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AssetSoundSynesthesiaLoudnessSettings", "Synesthesia Real-Time Settings (Loudness)");
}

UClass* UAtomLoudnessSettings::GetSupportedClass() const
{
	return UAtomLoudnessSettings::StaticClass();
}
#endif*/

UAtomLoudnessAnalyzer::UAtomLoudnessAnalyzer()
{
	if (!Settings)
	{
		Settings = CreateDefaultSubobject<UAtomLoudnessSettings>(TEXT("DefaultLoudnessSettings"));
	}
}

TUniquePtr<Atom::IAnalyzerSettings> UAtomLoudnessAnalyzer::GetSettings(const int32 InSampleRate, const int32 InNumChannels) const
{
	TUniquePtr<Atom::IAnalyzerSettings> AnalyzerSettings;

	if (Settings)
	{
		AnalyzerSettings = Settings->GetSettings(InSampleRate, InNumChannels);
	}

	return AnalyzerSettings;
}

static TArray<FAtomLoudnessResults> ConvertToBlueprintResults(UAtomLoudnessSettings* Settings, const TArray<Atom::FLoudnessEntry>& InLoudnessArray)
{
	// Helper function for "Sort" (higher priority sorts last).
	struct FCompareLoudnessResults
	{
		FORCEINLINE bool operator()(const FAtomLoudnessResults& A, const FAtomLoudnessResults& B) const
		{
			return A.TimeSeconds < B.TimeSeconds;
		}
	};

	TArray<FAtomLoudnessResults> ResultsArray;
	for (const Atom::FLoudnessEntry& LoudnessEntry : InLoudnessArray)
	{
		FAtomLoudnessResults NewResults;
		NewResults.Momentary = LoudnessEntry.Momentary;
		NewResults.ShortTerm = LoudnessEntry.ShortTerm;
		NewResults.Integrated = LoudnessEntry.Integrated;
		NewResults.TimeSeconds = LoudnessEntry.Timestamp;

		ResultsArray.Add(NewResults);
	}

	// Sort by priority (lowest priority first).
	ResultsArray.Sort(FCompareLoudnessResults());
	return MoveTemp(ResultsArray);
}

void UAtomLoudnessAnalyzer::BroadcastResults()
{
	TUniquePtr<const Atom::FLoudnessResult> LoudnessResults = GetResults<Atom::FLoudnessResult>();

	bool bIsOnOverallLoudnessResultsBound = OnOverallLoudnessResults.IsBound() || OnOverallLoudnessResultsNative.IsBound();
	bool bIsOnLatestOverallLoudnessResultsBound = OnLatestOverallLoudnessResults.IsBound() || OnLatestOverallLoudnessResultsNative.IsBound();
	if (bIsOnOverallLoudnessResultsBound || bIsOnLatestOverallLoudnessResultsBound)
	{
		const TArray<Atom::FLoudnessEntry>& OverallLoudnessArray = LoudnessResults->GetLoudnessArray();
		if (OverallLoudnessArray.Num() > 0)
		{
			TArray<FAtomLoudnessResults> Results = ConvertToBlueprintResults(Settings, OverallLoudnessArray);
			check(Results.Num() > 0);

			OnOverallLoudnessResults.Broadcast(Results);
			OnOverallLoudnessResultsNative.Broadcast(this, Results);

			FAtomLoudnessResults& Latest = Results[Results.Num() - 1];
			OnLatestOverallLoudnessResults.Broadcast(Latest);
			OnLatestOverallLoudnessResultsNative.Broadcast(this, Latest);
		}
	}
}

FName UAtomLoudnessAnalyzer::GetAnalyzerFactoryName() const
{
	static const FName FactoryName(TEXT("AtomLoudnessFactory"));
	return FactoryName;
}
