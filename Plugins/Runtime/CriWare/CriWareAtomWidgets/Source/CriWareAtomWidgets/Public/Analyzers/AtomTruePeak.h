
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"
#include "Delegates/DelegateCombinations.h"

#include "Atom/Analyzers/AtomAnalyzer.h"

#include "AtomTruePeak.generated.h"

class UAtomTruePeakAnalyzer;

/** UAtomTruePeakSettings
 *
 * Settings for a UTruePeakAnalyzer.
 */
UCLASS(Blueprintable)
class CRIWAREATOMWIDGETS_API UAtomTruePeakSettings : public UAtomAnalyzerSettings
{
	GENERATED_BODY()
public:

	UAtomTruePeakSettings() {}

	/** Number of seconds between true peak measurements */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0.01", ClampMax = "0.25"))
	float AnalysisPeriod = 0.01f;

	/** Peak hold time, in milliseconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0"))
	int32 PeakHoldTime = 100;

	/** What volume threshold to throw clipping detection notifications. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0.0"))
	float ClippingThreshold = 1.0f;

	/** Convert UTruePeakSettings to IAnalyzerSettings */
	TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const int32 InSampleRate, const int32 InNumChannels) const;

#if WITH_EDITOR
	//virtual FText GetAssetActionName() const override;

	//virtual UClass* GetSupportedClass() const override;
#endif
};

/** The results of the true peak analysis. */
USTRUCT(BlueprintType)
struct CRIWAREATOMWIDGETS_API FAtomTruePeakResults
{
	GENERATED_BODY()

	// The time in seconds since analysis began of this true peak analysis result
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float TimeSeconds = 0.0f;

	// The peak value
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float PeakValue = 0.0f;

	// The hold peak value
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float PeakHoldValue = 0.0f;

	// The number of samples in the period which were above the clipping threshold. Will be 0 if no clipping was detected.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	int32 NumSamplesClipping = 0;

	// The value (if non-zero) if clipping was detected above the clipping threshold
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float ClippingValue = 0.0f;
};

/** Delegate to receive all overall true peak results (time-stamped in an array) since last delegate call. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOverallAtomTruePeakResults, const TArray<FAtomTruePeakResults>&, TruePeakResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnOverallAtomTruePeakResultsNative, UAtomTruePeakAnalyzer*, const TArray<FAtomTruePeakResults>&);

/** Delegate to receive only the most recent overall true peak results. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLatestOverallAtomTruePeakResults, const FAtomTruePeakResults&, LatestOverallTruePeakResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLatestOverallAtomTruePeakResultsNative, UAtomTruePeakAnalyzer*, const FAtomTruePeakResults&);

/** Delegate to receive all true peak results per channel (time-stamped in an array) since last delegate call. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPerChannelAtomTruePeakResults, int32, ChannelIndex, const TArray<FAtomTruePeakResults>&, TruePeakResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPerChannelAtomTruePeakResultsNative, UAtomTruePeakAnalyzer*, int32, const TArray<FAtomTruePeakResults>&);

/** Delegate to receive only the most recent overall true peak result per channel. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLatestPerChannelAtomTruePeakResults, int32, ChannelIndex, const FAtomTruePeakResults&, LatestTruePeakResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLatestPerChannelAtomTruePeakResultsNative, UAtomTruePeakAnalyzer*, int32, const FAtomTruePeakResults&);

/** UAtomTruePeakAnalyzer
 *
 * UAtomTruePeakAnalyzer calculates the current amplitude of an
 * Atom audio bus in real-time.
 */
UCLASS(Blueprintable)
class CRIWAREATOMWIDGETS_API UAtomTruePeakAnalyzer : public UAtomAnalyzer
{
	GENERATED_BODY()

public:

	UAtomTruePeakAnalyzer();

	/** The settings for the true peak audio analyzer.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomAnalyzer)
	TObjectPtr<UAtomTruePeakSettings> Settings;

	/** Delegate to receive all overall true peak results since last delegate call. */
	UPROPERTY(BlueprintAssignable)
	FOnOverallAtomTruePeakResults OnOverallTruePeakResults;

	FOnOverallAtomTruePeakResultsNative OnOverallTruePeakResultsNative;

	/** Delegate to receive all true peak results, per-channel, since last delegate call. */
	UPROPERTY(BlueprintAssignable)
	FOnPerChannelAtomTruePeakResults OnPerChannelTruePeakResults;

	FOnPerChannelAtomTruePeakResultsNative OnPerChannelTruePeakResultsNative;

	/** Delegate to receive the latest overall true peak results. */
	UPROPERTY(BlueprintAssignable)
	FOnLatestOverallAtomTruePeakResults OnLatestOverallTruePeakResults;

	FOnLatestOverallAtomTruePeakResultsNative OnLatestOverallTruePeakResultsNative;

	/** Delegate to receive the latest per-channel true peak results. */
	UPROPERTY(BlueprintAssignable)
	FOnLatestPerChannelAtomTruePeakResults OnLatestPerChannelTruePeakResults;

	FOnLatestPerChannelAtomTruePeakResultsNative OnLatestPerChannelTruePeakResultsNative;

	/** Convert UAtomTruePeakSettings to IAnalyzerSettings */
	TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const int32 InSampleRate, const int32 InNumChannels) const override;

	/** Broadcasts results to any delegates if hooked up. */
	void BroadcastResults() override;

protected:

	/** Return the name of the IAtomAnalyzerFactory associated with this UAtomAnalyzer */
	FName GetAnalyzerFactoryName() const override;
};
