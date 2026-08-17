
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"
#include "Delegates/DelegateCombinations.h"

#include "Atom/Analyzers/AtomAnalyzer.h"
#include "Atom/AtomBus.h"

#include "AtomSpectrumAnalysis.generated.h"

#define CRI_API CRIWAREATOMWIDGETS_API

class UAtomSpectrumAnalyzer;

/** UAtomSpectrumAnalysisSettings
 *
 * Settings for a UAtomSpectrumAnalysisAnalyzer.
 */
UCLASS(MinimalAPI, Blueprintable)
class UAtomSpectrumAnalysisSettings : public UAtomAnalyzerSettings
{
	GENERATED_BODY()
public:

	UAtomSpectrumAnalysisSettings() {}

	/** Number of seconds between AtomSpectrumAnalysis measurements */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer, meta = (ClampMin = "0.01", ClampMax = "0.25"))
	float AnalysisPeriod = 0.01f;

	/** Size of FFT. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	EAtomFFTSize FFTSize = EAtomFFTSize::DefaultSize;
	
	/** Type of spectrum to use. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	EAtomSpectrumType SpectrumType = EAtomSpectrumType::PowerSpectrum;

	/** Type of window to use. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	EAtomFFTWindowType WindowType = EAtomFFTWindowType::Hann;

	/** If true, multichannel audio is downmixed to mono with equal amplitude scaling. If false, each channel gets its own FFT result. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	bool bDownmixToMono = true;

	/** Convert UAtomSpectrumAnalysisSettings to IAnalyzerSettings */
	CRI_API TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const int32 InSampleRate, const int32 InNumChannels) const;

#if WITH_EDITOR
	//CRI_API virtual FText GetAssetActionName() const override;

	//CRI_API virtual UClass* GetSupportedClass() const override;
#endif
};

/** The results of the spectrum analysis. */
USTRUCT(BlueprintType)
struct FAtomSpectrumResults
{
	GENERATED_USTRUCT_BODY()

	// The time in seconds since analysis began of this AtomSpectrumAnalysis analysis result
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float TimeSeconds = 0.0f;

	// The spectrum values from the FFT
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	TArray<float> SpectrumValues;
};

/** Delegate to receive all spectrum results per channel (time-stamped in an array) since last delegate call. If bDownmixToMono setting is true, results will be in channel index 0. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAtomSpectrumResults, int32, ChannelIndex, const TArray<FAtomSpectrumResults>&, SpectrumResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAtomSpectrumResultsNative, UAtomSpectrumAnalyzer*, int32, const TArray<FAtomSpectrumResults>&);

/** Delegate to receive only the most recent overall spectrum result per channel. If bDownmixToMono setting is true, results will be in channel index 0.*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLatestAtomSpectrumResults, int32, ChannelIndex, const FAtomSpectrumResults&, LatestSpectrumResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLatestAtomSpectrumResultsNative, UAtomSpectrumAnalyzer*, int32, const FAtomSpectrumResults&);

/** UAtomSpectrumAnalysisAnalyzer
 *
 * UAtomSpectrumAnalysisAnalyzer calculates the current amplitude of an
 * audio bus in real-time.
 */
UCLASS(MinimalAPI, Blueprintable)
class UAtomSpectrumAnalyzer : public UAtomAnalyzer
{
	GENERATED_BODY()

public:

	CRI_API UAtomSpectrumAnalyzer();

	/** The settings for the AtomSpectrumAnalysis audio analyzer.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomAnalyzer)
	TObjectPtr<UAtomSpectrumAnalysisSettings> Settings;

	/** Delegate to receive all Spectrum results, per-channel, since last delegate call. If bDownmixToMono setting is true, results will be in channel index 0. */
	UPROPERTY(BlueprintAssignable)
	FOnAtomSpectrumResults OnSpectrumResults;

	FOnAtomSpectrumResultsNative OnSpectrumResultsNative;

	/** Delegate to receive the latest per-channel Spectrum results. If bDownmixToMono setting is true, results will be in channel index 0. */
	UPROPERTY(BlueprintAssignable)
	FOnLatestAtomSpectrumResults OnLatestSpectrumResults;

	FOnLatestAtomSpectrumResultsNative OnLatestSpectrumResultsNative;

	/** Convert UAtomSpectrumAnalysisSettings to IAnalyzerSettings */
	CRI_API TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const int32 InSampleRate, const int32 InNumChannels) const override;

	/** Broadcasts results to any delegates if hooked up. */
	CRI_API void BroadcastResults() override;

	UFUNCTION(BlueprintCallable, Category = "Atom Analyzer")
	CRI_API void GetCenterFrequencies(UPARAM(DisplayName = "Sample Rate") const float InSampleRate, UPARAM(DisplayName = "Center Frequencies") TArray<float>& OutCenterFrequencies);

	UFUNCTION(BlueprintCallable, Category = "Atom Analyzer")
	CRI_API UPARAM(DisplayName = "Num Center Frequencies") const int32 GetNumCenterFrequencies() const;

protected:

	/** Return the name of the IAtomAnalyzerFactory associated with this UAtomAnalyzer */
	CRI_API FName GetAnalyzerFactoryName() const override;
};

#undef CRI_API
