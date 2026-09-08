
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"
#include "Delegates/DelegateCombinations.h"

#include "Atom/Analyzers/AtomAnalyzer.h"

#include "AtomMeter.generated.h"

class UAtomMeterAnalyzer;

UENUM(BlueprintType)
enum class EAtomMeterPeakType : uint8
{
	MeanSquared,
	RootMeanSquared,
	Peak,
	Count UMETA(Hidden)
};

/** UAtomMeterSettings
 *
 * Settings for a UMeterAnalyzer.
 */
UCLASS(Blueprintable)
class CRIWAREATOMWIDGETS_API UAtomMeterSettings : public UAtomAnalyzerSettings
{
	GENERATED_BODY()
public:

	UAtomMeterSettings() {}

	/** Number of seconds between meter measurements */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0.01", ClampMax = "0.25"))
	float AnalysisPeriod = 0.01f;

	/** Meter envelope type type */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	EAtomMeterPeakType PeakMode = EAtomMeterPeakType::RootMeanSquared;

	/** Meter attack time, in milliseconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0"))
	int32 MeterAttackTime = 300;

	/** Meter release time, in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0"))
	int32 MeterReleaseTime = 300;

	/** Peak hold time, in milliseconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0"))
	int32 PeakHoldTime = 100;

	/** What volume threshold to throw clipping detection notifications. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0.0"))
	float ClippingThreshold = 1.0f;

	/** Convert UMeterSettings to IAnalyzerSettings */
	TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const int32 InSampleRate, const int32 InNumChannels) const;

#if WITH_EDITOR
	//virtual FText GetAssetActionName() const override;

	//virtual UClass* GetSupportedClass() const override;
#endif
};

/** The results of the meter analysis. */
USTRUCT(BlueprintType)
struct CRIWAREATOMWIDGETS_API FAtomMeterResults
{
	GENERATED_USTRUCT_BODY()

	// The time in seconds since analysis began of this meter analysis result
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float TimeSeconds = 0.0f;

	// The meter value
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float MeterValue = 0.0f;

	// The peak value
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float PeakValue = 0.0f;

	// The number of samples in the period which were above the clipping threshold. Will be 0 if no clipping was detected.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	int32 NumSamplesClipping = 0;

	// The value (if non-zero) if clipping was detected above the clipping threshold
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float ClippingValue = 0.0f;
};

/** Delegate to receive all overall loudness results (time-stamped in an array) since last delegate call. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOverallAtomMeterResults, const TArray<FAtomMeterResults>&, MeterResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnOverallAtomMeterResultsNative, UAtomMeterAnalyzer*, const TArray<FAtomMeterResults>&);

/** Delegate to receive only the most recent overall meter results. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLatestOverallAtomMeterResults, const FAtomMeterResults&, LatestOverallMeterResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLatestOverallAtomMeterResultsNative, UAtomMeterAnalyzer*, const FAtomMeterResults&);

/** Delegate to receive all meter results per channel (time-stamped in an array) since last delegate call. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPerChannelAtomMeterResults, int32, ChannelIndex, const TArray<FAtomMeterResults>&, MeterResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPerChannelAtomMeterResultsNative, UAtomMeterAnalyzer*, int32, const TArray<FAtomMeterResults>&);

/** Delegate to receive only the most recent overall meter result per channel. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLatestPerChannelAtomMeterResults, int32, ChannelIndex, const FAtomMeterResults&, LatestMeterResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLatestPerChannelAtomMeterResultsNative, UAtomMeterAnalyzer*, int32, const FAtomMeterResults&);

/** UAtomMeterAnalyzer
 *
 * UAtomMeterAnalyzer calculates the current amplitude of an
 * Atom audio bus in real-time.
 */
UCLASS(Blueprintable)
class CRIWAREATOMWIDGETS_API UAtomMeterAnalyzer : public UAtomAnalyzer
{
	GENERATED_BODY()

public:

	UAtomMeterAnalyzer();

	/** The settings for the meter audio analyzer.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomAnalyzer)
	TObjectPtr<UAtomMeterSettings> Settings;

	/** Delegate to receive all overall meter results since last delegate call. */
	UPROPERTY(BlueprintAssignable)
	FOnOverallAtomMeterResults OnOverallMeterResults;

	FOnOverallAtomMeterResultsNative OnOverallMeterResultsNative;

	/** Delegate to receive all meter results, per-channel, since last delegate call. */
	UPROPERTY(BlueprintAssignable)
	FOnPerChannelAtomMeterResults OnPerChannelMeterResults;

	FOnPerChannelAtomMeterResultsNative OnPerChannelMeterResultsNative;

	/** Delegate to receive the latest overall meter results. */
	UPROPERTY(BlueprintAssignable)
	FOnLatestOverallAtomMeterResults OnLatestOverallMeterResults;

	FOnLatestOverallAtomMeterResultsNative OnLatestOverallMeterResultsNative;

	/** Delegate to receive the latest per-channel meter results. */
	UPROPERTY(BlueprintAssignable)
	FOnLatestPerChannelAtomMeterResults OnLatestPerChannelMeterResults;

	FOnLatestPerChannelAtomMeterResultsNative OnLatestPerChannelMeterResultsNative;

	/** Convert UAtomMeterSettings to IAnalyzerSettings */
	TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const int32 InSampleRate, const int32 InNumChannels) const override;

	/** Broadcasts results to any delegates if hooked up. */
	void BroadcastResults() override;

protected:

	/** Return the name of the IAtomAnalyzerFactory associated with this UAtomAnalyzer */
	FName GetAnalyzerFactoryName() const override;
};
