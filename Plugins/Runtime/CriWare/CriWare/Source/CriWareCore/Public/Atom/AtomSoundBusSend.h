
#pragma once

#include "CoreMinimal.h"

#include "Curves/CurveFloat.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "AtomSoundBusSend.generated.h"

// Forward Declarations
class UAtomRackBase;
class UAtomBus;

UENUM(BlueprintType)
enum class EAtomSpectrumBandPresetType : uint8
{
	/** Band which contains frequencies generally related to kick drums. */
	KickDrum,

	/** Band which contains frequencies generally related to snare drums. */
	SnareDrum,

	/** Band which contains frequencies generally related to vocals. */
	Voice,

	/** Band which contains frequencies generally related to cymbals. */
	Cymbals
};

USTRUCT(BlueprintType)
struct FAtomBusSpectralAnalysisBandSettings
{
	GENERATED_BODY()

	// The frequency band for the magnitude to analyze
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomBusSpectralAnalysis, meta = (ClampMin = "10.0", ClampMax = "20000.0", UIMin = "10.0", UIMax = "10000.0"))
	float BandFrequency = 440.0f;

	// The attack time for the FFT band interpolation for delegate callback
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomBusSpectralAnalysis, meta = (ClampMin = "0.0", UIMin = "10.0", UIMax = "10000.0"))
	int32 AttackTimeMsec = 10;

	// The release time for the FFT band interpolation for delegate callback
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AtomBusSpectralAnalysis, meta = (ClampMin = "0.0", UIMin = "10.0", UIMax = "10000.0"))
	int32 ReleaseTimeMsec = 500;

	// The ratio of the bandwidth divided by the center frequency. Only used when the spectral analysis type is set to Constant Q.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = AtomBusSpectralAnalysis, meta = (ClampMin = "0.001", UIMin = "0.1", UIMax = "100.0"))
	float QFactor = 10.0f;
};

USTRUCT(BlueprintType)
struct FAtomLevelMeterMeasure
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelMeter, DisplayName = "RMS Level")
	float Level = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelMeter)
	float PeakLevel = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LevelMeter)
	float PeakHoldLevel = 0.0f;
};

inline bool operator==(const FAtomLevelMeterMeasure& lhs, const FAtomLevelMeterMeasure& rhs)
{
	return FMath::IsNearlyEqual(lhs.Level, rhs.Level) &&
		FMath::IsNearlyEqual(lhs.PeakLevel, rhs.PeakLevel) &&
		FMath::IsNearlyEqual(lhs.PeakHoldLevel, rhs.PeakHoldLevel);
}

UCLASS(BlueprintType, MinimalAPI)
class UAtomSoundLevelMeterSettings : public UObject
{
	GENERATED_BODY()

public:

	/** Number of seconds between meter measurements */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelMeter, meta = (ClampMin = "0.01", ClampMax = "0.25"))
	float AnalysisPeriod = 0.02f;

	/** Peak hold time, in milliseconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelMeter, meta = (ClampMin = "0"))
	int32 PeakHoldTime = 1000;
};

USTRUCT(BlueprintType)
struct FAtomLoudnessMeterMeasure
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LoudnessMeter)
	float Momentary = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LoudnessMeter)
	float ShortTerm = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LoudnessMeter)
	float Integrated = 0.0f;
};

inline bool operator==(const FAtomLoudnessMeterMeasure& lhs, const FAtomLoudnessMeterMeasure& rhs)
{
	return FMath::IsNearlyEqual(lhs.Momentary, rhs.Momentary) &&
		FMath::IsNearlyEqual(lhs.ShortTerm, rhs.ShortTerm) &&
		FMath::IsNearlyEqual(lhs.Integrated, rhs.Integrated);
}

UCLASS(BlueprintType, MinimalAPI)
class UAtomSoundLoudnessMeterSettings : public UObject
{
	GENERATED_BODY()

public:

	/** Number of seconds for the short-term loudness average */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LoudnessMeter, meta = (ClampMin = "1", ClampMax = "20"))
	int32 ShortTermTime = 3;

	/** Number of seconds for the long-term loudness average */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LoudnessMeter, meta = (ClampMin = "100", ClampMax = "1000"))
	int32 IntegratedTime = 600;
};

USTRUCT(BlueprintType)
struct FAtomTruePeakMeterMeasure
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TruePeakMeter)
	float Level = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = TruePeakMeter)
	float HoldLevel = 0.0f;
};

inline bool operator==(const FAtomTruePeakMeterMeasure& lhs, const FAtomTruePeakMeterMeasure& rhs)
{
	return FMath::IsNearlyEqual(lhs.Level, rhs.Level) &&
		FMath::IsNearlyEqual(lhs.HoldLevel, rhs.HoldLevel);
}

UCLASS(BlueprintType, MinimalAPI)
class UAtomSoundTruePeakMeterSettings : public UObject
{
	GENERATED_BODY()

public:

	/** Number of seconds between meter measurements */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = TruePeakMeter, meta = (ClampMin = "0.01", ClampMax = "0.25"))
	float AnalysisPeriod = 0.02f;

	/** Peak hold time, in milliseconds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = TruePeakMeter, meta = (ClampMin = "0"))
	int32 PeakHoldTime = 1000;

	/** Whether to clip samples */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = TruePeakMeter)
	bool SampleClipping = false;
};

USTRUCT(BlueprintType)
struct FAtomPerformanceMonitorResult
{
	GENERATED_BODY()

	/**Number of signal generation processes*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PerformanceMonitor)
	int32 ProcessCount = 0;

	/** Last measured value of processing time(in microseconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PerformanceMonitor)
	int32 LastProcessTime = 0;

	/*JP<Maximum value of processing time (in microseconds)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PerformanceMonitor)
	int32 MaxProcessTime = 0;

	/*JP<Average value of processing time (in microseconds)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PerformanceMonitor)
	int32 AverageProcessTime = 0;

	/*JP<Last measured value of processing interval (in microseconds)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PerformanceMonitor)
	int32 LastProcessInterval = 0;

	/*JP<Maximum value of processing interval (in microseconds)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PerformanceMonitor)
	int32 MaxProcessInterval = 0;

	/*JP<Average value of processing interval (in microseconds)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PerformanceMonitor)
	int32 AverageProcessInterval = 0;

	/*JP<Last measured value of number of samples generated by unit processing*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PerformanceMonitor)
	int32 LastProcessSamples = 0;

	/*JP<Maximum value of number of samples generated by unit processing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PerformanceMonitor)
	int32 MaxProcessSamples = 0;

	/* JP<Average number of samples generated per unit process */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PerformanceMonitor)
	int32 AverageProcessSamples = 0;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAtomBusEnvelopeBP, const TArray<float>&, Envelope);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAtomBusSpectralAnalysisBP, const TArray<float>&, Magnitudes);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAtomRackLevelMeterMeasureBP, const TArray<FAtomLevelMeterMeasure>&, Measures);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAtomRackLoudnessMeterMeasureBP, const FAtomLoudnessMeterMeasure&, Measure);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAtomRackTruePeakMeterMeasureBP, const TArray<FAtomTruePeakMeterMeasure>&, Measures);
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnAtomRackRenderedSamplesBP, int32, RackID, int64, NumSamples, int32, SampleRate);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAtomRackPerformanceMonitorResultBP, const FAtomPerformanceMonitorResult&, Result);

UENUM(BlueprintType)
enum class EAtomSendLevelControlMethod : uint8
{
	// A send based on linear interpolation between a distance range and send-level range
	Linear,

	// A send based on a supplied curve
	CustomCurve,

	// A manual send level (Uses the specified constant send level value. Useful for 2D sounds.)
	Manual,
};

// Class used to send audio to rack from UAtomSoundBase
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomSoundToRackSend
{
	GENERATED_BODY()

	FAtomSoundToRackSend()
		: Rack(nullptr)
		, SendLevel(0.0f)
	{
	}

	// The rack to send the audio to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RackSend)
	TObjectPtr<UAtomRackBase> Rack;

	// The amount of audio to send
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RackSend)
	float SendLevel;
};

// Class used to send audio to bus from UAtomSoundBase
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomSoundToBusSend
{
	GENERATED_BODY()

	FAtomSoundToBusSend()
		: SendLevelControlMethod(EAtomSendLevelControlMethod::Manual)
		, Bus(nullptr)
		, SendLevel(0.0f)
		, DisableManualSendClamp(false)
		, MinSendLevel(0.0f)
		, MaxSendLevel(1.0f)
		, MinSendDistance(100.0f)
		, MaxSendDistance(1000.0f)
	{
	}

	/*
		Manual: Use Send Level only
		Linear: Interpolate between Min and Max Send Levels based on listener distance (between Distance Min and Distance Max)
		Custom Curve: Use the float curve to map Send Level to distance (0.0-1.0 on curve maps to Distance Min - Distance Max)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusSend)
	EAtomSendLevelControlMethod SendLevelControlMethod;

	// The bus to send the audio to
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusSend)
	TObjectPtr<UAtomBus> Bus;

	// The amount of audio to send
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusSend)
	float SendLevel;

	// Whether to disable the 0-1 clamp for manual SendLevel control
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusSend)
	bool DisableManualSendClamp;

	// The amount to send to master when sound is located at a distance equal to value specified in the min send distance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusSend)
	float MinSendLevel;

	// The amount to send to master when sound is located at a distance equal to value specified in the max send distance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusSend)
	float MaxSendLevel;

	// The min distance to send to the master
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusSend)
	float MinSendDistance;

	// The max distance to send to the master
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusSend)
	float MaxSendDistance;

	// The custom reverb send curve to use for distance-based send level.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusSend)
	FRuntimeFloatCurve CustomSendLevelCurve;
};

/* ~ Bus Send */
