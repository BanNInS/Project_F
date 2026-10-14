
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"
#include "Delegates/DelegateCombinations.h"

#include "Atom/Analyzers/AtomAnalyzer.h"

#include "AtomLoudness.generated.h"

#define CRI_API CRIWAREATOMWIDGETS_API

/** ULoudnessSettings
 *
 * Settings for a ULoudness analyzer.
 */
UCLASS(MinimalAPI, Blueprintable)
class UAtomLoudnessSettings : public UAtomAnalyzerSettings
{
	GENERATED_BODY()
public:

	UAtomLoudnessSettings() {}

	/** Number of seconds between loudness measurements */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0.01", ClampMax = "0.25"))
	float AnalysisPeriod = 0.1f;

	/** Number of seconds for sample window used for momentary loudness measurements. (400ms) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0.01", ClampMax = "0.25"))
	float MomentarySize = 0.4f;

	/** Number of seconds for momentary loudness integration to measure the short-term loudness. (3 seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0.01", ClampMax = "0.25"))
	float ShortTermSize = 3.0f;

	/** Maxmium Number of seconds for momentary loudness integration to measure the long-term loudness. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0.01", ClampMax = "0.25"))
	float MaxIntegratedSize = 600.0f;

	/** Threshold in LUFS. Discard momentary loudness that fall bellow this threshold for integrated loudness calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "-96.0", ClampMax = "0.0"))
	float AbsoluteSilenceThreshold = -70.0f;

	/** Threshold in LU. Discard values that relatively fall bellow this threshold for integrated loudness calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "-96.0", ClampMax = "96.0"))
	float RelativeSilenceThreshold = -10.0f;

	/** Convert ULoudnessSettings to FLoudnessSettings */
	CRI_API TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const int32 InSampleRate, const int32 InNumChannels) const;

#if WITH_EDITOR
	//CRI_API virtual FText GetAssetActionName() const override;

	//CRI_API virtual UClass* GetSupportedClass() const override;
#endif
};

/** The results of the loudness analyis. */
USTRUCT(BlueprintType)
struct FAtomLoudnessResults
{
	GENERATED_BODY()

	// The momentary loudness in LUFS
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float Momentary = 0.0f;

	// The short-term loudness in LUFS
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float ShortTerm = 0.0f;

	// The integrated loudness in LUFS
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float Integrated = 0.0f;

	// The time in seconds since analysis began
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float TimeSeconds = 0.0f;
};

/** Delegate to receive all overall loudness results (time-stamped in an array) since last delegate call. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOverallAtomLoudnessResults, const TArray<FAtomLoudnessResults>&, OverallLoudnessResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnOverallAtomLoudnessResultsNative, UAtomLoudnessAnalyzer*, const TArray<FAtomLoudnessResults>&);

/** Delegate to receive only the most recent overall loudness result. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLatestOverallAtomLoudnessResults, const FAtomLoudnessResults&, LatestOverallLoudnessResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLatestOverallAtomLoudnessResultsNative, UAtomLoudnessAnalyzer*, const FAtomLoudnessResults&);


/** UAtomLoudnessAnalyzer
 *
 * ULoudnessAnalyzer calculates the temporal evolution of perceptual loudness for a given
 * audio bus in real-time. Loudness is available for individual channels or the overall audio bus. Normalized
 * loudness values convert the range to 0.0 to 1.0 where 0.0 is the noise floor and 1.0 is the
 * maximum loudness of the particular sound.
 */
UCLASS(MinimalAPI, Blueprintable)
class UAtomLoudnessAnalyzer : public UAtomAnalyzer
{
	GENERATED_BODY()

public:

	CRI_API UAtomLoudnessAnalyzer();

	/** The settings for the audio analyzer.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomAnalyzer, meta = (ExposeOnSpawn))
	TObjectPtr<UAtomLoudnessSettings> Settings;

	/** Delegate to receive all overall loudness results since last delegate call. */
	UPROPERTY(BlueprintAssignable, Category = AtomAnalyzer)
	FOnOverallAtomLoudnessResults OnOverallLoudnessResults;

	FOnOverallAtomLoudnessResultsNative OnOverallLoudnessResultsNative;

	/** Delegate to receive the latest overall loudness results. */
	UPROPERTY(BlueprintAssignable, Category = AtomAnalyzer)
	FOnLatestOverallAtomLoudnessResults OnLatestOverallLoudnessResults;

	FOnLatestOverallAtomLoudnessResultsNative OnLatestOverallLoudnessResultsNative;

	/** Convert ULoudnessSettings to FLoudnessSettings */
	CRI_API TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const int32 InSampleRate, const int32 InNumChannels) const override;

	/** Broadcasts results to any delegates if hooked up. */
	CRI_API void BroadcastResults() override;

protected:

	/** Return the name of the IAtomAnalyzerFactory associated with this UAtomAnalyzer */
	CRI_API FName GetAnalyzerFactoryName() const override;
};

#undef CRI_API
