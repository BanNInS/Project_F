// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectMacros.h"
#include "Delegates/DelegateCombinations.h"

#include "Atom/AtomBus.h"
#include "Atom/Analyzers/AtomAnalyzer.h"

#include "AtomConstantQ.generated.h"

#define CRI_API CRIWAREATOMWIDGETS_API


/** EConstantQNormalizationEnum
 *
 * Enumeration of available normalization schemes for ConstantQ frequency domain windows.
 */
UENUM(BlueprintType)
enum class EAtomConstantQNormalizationEnum : uint8
{
	/** Normalize bands by euclidean norm. Good when using magnitude spectrum. */
	EqualEuclideanNorm	UMETA(DisplayName = "Equal Euclidean Norm"),

	/** Normalize bands by energy. Good when using power spectrum. */
	EqualEnergy			UMETA(DisplayName = "Equal Energy"),

	/** Normalize bands by their maximum values. Will result in relatively strong high frequences because the upper constant Q bands have larger bandwidths. */
	EqualAmplitude 		UMETA(DisplayName = "Equal Amplitude"),
};

/** EAtomConstantQFFTSizeEnum
 *
 *  Enumeration of available FFT sizes in audio frames.
 */
UENUM(BlueprintType)
enum class EAtomConstantQFFTSizeEnum : uint8
{
	// 64
	Min UMETA(DisplayName = "64"),

	// 128
	XXSmall UMETA(DisplayName = "128"),

	// 256
	XSmall UMETA(DisplayName = "256"),

	// 512
	Small UMETA(DisplayName = "512"),

	// 1024
	Medium UMETA(DisplayName = "1024"),

	// 2048
	Large UMETA(DisplayName = "2048"),

	// 4096
	XLarge UMETA(DisplayName = "4096"),

	// 8192
	XXLarge UMETA(DisplayName = "8192"),

	// 16384
	Max UMETA(DisplayName = "16384")
};

/** UAtomConstantQSettings
 *
 * Settings for a UConstantQ analyzer.
 */
UCLASS(MinimalAPI, Blueprintable)
class UAtomConstantQSettings : public UAtomAnalyzerSettings
{
	GENERATED_BODY()
public:

	UAtomConstantQSettings() {}

	/** Starting frequency for first bin of CQT */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AtomAnalyzer, DisplayName = "Starting Frequency (Hz)", meta = (ClampMin = "20.0", ClampMax = "20000"))
	float StartingFrequencyHz = 40.0f;

	/** Total number of resulting constant Q bands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AtomAnalyzer, meta = (ClampMin = "1", ClampMax = "96"))
	int32 NumBands = 48;

	/** Number of bands within an octave. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AtomAnalyzer, meta = (ClampMin = "1", ClampMax = "24"))
	float NumBandsPerOctave = 12.0f;

	/** Number of seconds between cqt measurements */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AtomAnalyzer, DisplayName = "Analysis Period (s)", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float AnalysisPeriodInSeconds = 0.01f;

	/** If true, multichannel audio is downmixed to mono with equal amplitude scaling. If false, each channel gets it's own CQT result. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AtomAnalyzer)
	bool bDownmixToMono = false;

	/** Size of FFT. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category=AtomAnalyzer)
	EAtomConstantQFFTSizeEnum FFTSize = EAtomConstantQFFTSizeEnum::XLarge;

	/** Type of window to be applied to input audio */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category=AtomAnalyzer)
	EAtomFFTWindowType WindowType = EAtomFFTWindowType::Blackman;

	/** Type of spectrum to use. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category=AtomAnalyzer)
	EAtomSpectrumType SpectrumType = EAtomSpectrumType::PowerSpectrum;
		
	/** Stretching factor to control overlap of adjacent bands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category=AtomAnalyzer, meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float BandWidthStretch = 1.0f;
		
	/** Normalization scheme used to generate band windows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category=AtomAnalyzer)
	EAtomConstantQNormalizationEnum CQTNormalization = EAtomConstantQNormalizationEnum::EqualEnergy;

	/** Noise floor to use when normalizing CQT */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category=AtomAnalyzer, meta = (DisplayName = "Noise Floor (dB)", ClampMin = "-120.0", ClampMax = "0.0"))
	float NoiseFloorDb = -60.0f;

	/** Convert UAtomConstantQSettings to FConstantQSettings */
	CRI_API TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const float InSampleRate, const int32 InNumChannels) const;

#if WITH_EDITOR
	//CRI_API virtual FText GetAssetActionName() const override;

	//CRI_API virtual UClass* GetSupportedClass() const override;
#endif // WITH_EDITOR
};

/** The results of the ConstantQ analysis. */
USTRUCT(BlueprintType)
struct FAtomConstantQResults
{
	GENERATED_BODY()

	// The time in seconds since analysis began of this ConstantQ analysis result
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	float TimeSeconds = 0.0f;

	// The spectrum values from the FFT
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AtomAnalyzer)
	TArray<float> SpectrumValues;
};

/** Delegate to receive all ConstantQ results per channel (time-stamped in an array) since last delegate call. If bDownmixToMono setting is true, results will be in channel index 0. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAtomConstantQResults, int32, ChannelIndex, const TArray<FAtomConstantQResults>&, ConstantQResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnAtomConstantQResultsNative, UAtomConstantQAnalyzer*, int32, const TArray<FAtomConstantQResults>&);

/** Delegate to receive only the most recent overall ConstantQ result per channel. If bDownmixToMono setting is true, results will be in channel index 0.*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLatestAtomConstantQResults, int32, ChannelIndex, const FAtomConstantQResults&, LatestConstantQResults);

/** shadow delegate declaration for above */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLatestAtomConstantQResultsNative, UAtomConstantQAnalyzer*, int32, const FAtomConstantQResults&);

/** UConstantQAnalyzer
 *
 * UConstantQAnalyzer calculates the temporal evolution of constant q transform for a given
 * audio bus in real-time. ConstantQ is available for individual channels or the overall audio bus.
 */
UCLASS(MinimalAPI, Blueprintable)
class UAtomConstantQAnalyzer : public UAtomAnalyzer
{
	GENERATED_BODY()
public:
	CRI_API UAtomConstantQAnalyzer();

	/** The settings for the audio analyzer.  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomAnalyzer)
	TObjectPtr<UAtomConstantQSettings> Settings;

	/** Delegate to receive all Spectrum results, per-channel, since last delegate call. If bDownmixToMono setting is true, results will be in channel index 0. */
	UPROPERTY(BlueprintAssignable)
	FOnAtomConstantQResults OnConstantQResults;

	FOnAtomConstantQResultsNative OnConstantQResultsNative;

	/** Delegate to receive the latest per-channel Spectrum results. If bDownmixToMono setting is true, results will be in channel index 0. */
	UPROPERTY(BlueprintAssignable)
	FOnLatestAtomConstantQResults OnLatestConstantQResults;

	FOnLatestAtomConstantQResultsNative OnLatestConstantQResultsNative;

	/** Convert UAtomConstantQSettings to FConstantQSettings */
	CRI_API TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const int32 InSampleRate, const int32 InNumChannels) const override;

	/** Broadcasts results to any delegates if hooked up. */
	CRI_API void BroadcastResults() override;

	UFUNCTION(BlueprintCallable, Category = "Atom Analyzer")
	CRI_API void GetCenterFrequencies(UPARAM(DisplayName = "Center Frequencies") TArray<float>& OutCenterFrequencies);

	UFUNCTION(BlueprintCallable, Category = "Atom Analyzer")
	CRI_API UPARAM(DisplayName = "Num Center Frequencies") const int32 GetNumCenterFrequencies() const;

protected:

	/** Return the name of the IAtomAnalyzerFactory associated with this UAtomAnalyzer */
	CRI_API FName GetAnalyzerFactoryName() const override;
};

#undef CRI_API
