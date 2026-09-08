/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectCompressor.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CriWareDefines.h"
#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectCompressor.generated.h"

/*UENUM(BlueprintType)
enum class EAtomCompressorMode : uint8
{
	Normal,
	SideChain
};

UENUM(BlueprintType)
enum class EAtomCompressorType : uint8
{
	Peak,
	RMS
};*/

UENUM(BlueprintType)
enum class EAtomSplitEQFilterType : uint8
{
	None,
	LowPass,
	HighPass,
	BandPass,
	Notch
};

/*
 * FAtomSplitEQFilter Structure
 *****************************************************************************/

USTRUCT(BlueprintType)
struct FAtomSplitEQFilter
{
	GENERATED_BODY()

public:

	/** Specifies the filter type to use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EQ Filter")
	EAtomSplitEQFilterType FilterType = EAtomSplitEQFilterType::LowPass;

	/** Specifies the frequency of the filter in hertz (Hz). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EQ Filter", meta = (ClampMin = "24.0", ClampMax = "24000.0", UIMin = "24.0", UIMax = "24000.0", Units = Hz, SliderExponent = "10.0"))
	float Frequency = ATOM_MAX_FILTER_FREQUENCY;

	/** Specifies the Q factor (resonance) of the filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EQ Filter", meta = (ClampMin = "0.1", ClampMax = "10.0", UIMin = "0.1", UIMax = "10.0", SliderExponent = "5.0"))
	float Q = 1.0f;
};

 /*
  * FAtomBusEffectCompressorSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectCompressorSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectCompressorSettings();

	//~ Begin effect parameters
		
	/** Specifies the compressor threshold at which the amplitude starts to be compressed in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compressor", meta = (ClampMin = "-96.0", ClampMax = "0.0", UIMin = "-96.0", UIMax = "0.0", SliderExponent = "0.75"))
	float Threshold;

	/** Specifies the compressor ratio. 
	 * There are no minimum and maximum limits, but the minimum value is limited to 0.01f during signal processing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compressor", meta = (ClampMin = "1.0", ClampMax = "10000.0", Units = x, SliderExponent = "5.0"))
	float Ratio;

	/** Specifies the attack time of the compressor in milliseconds.
	 * The time until compression is completed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compressor", meta = (ClampMin = "0.0", ClampMax = "3000.0", UIMin = "0.0", UIMax = "3000.0", ForceUnits = ms))
	float AttackTime;

	/** Specifies the release time of the compressor in milliseconds.
	 * The time from when the amplitude becomes below the threshold until the end of compression. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compressor", meta = (ClampMin = "0.0", ClampMax = "50000.0", UIMin = "0.0", UIMax = "50000.0", ForceUnits = ms))
	float ReleaseTime;

	/** Specify the final output gain of the compressor in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compressor", meta = (ClampMin = "-96.0", ClampMax = "48.0", UIMin = "-96.0", UIMax = "48.0", SliderExponent = "0.75"))
	float Gain;

	/** Specify the surround link strength of the compressor.
	 * Surround link function changes the compression strength using the lowest amplitude value in all channels.
	 * 100% fully uses the lowest amplitude value in all channels, while 0% turns off the surround link function.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compressor", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", Units = %))
	float SurroundLink;

	/** Specifies the split EQ parameters of the compressor.
	 * Split EQ is an equalizer that divides the band into two. Compresses only the band extracted by the filter and synthesizes it with the remaining band. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compressor|Split EQ")
	FAtomSplitEQFilter SplitEQ;

	/** Specifies the compressor amplitude detection value type.*/
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compressor")
	//EAtomCompressorType Type;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectCompressorStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectCompressorStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	static FAtomBusEffectCompressorSettings& SetThreshold(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, float Threshold)
	{
		CompressorSettings.Threshold = FMath::Clamp(Threshold, -96.0, 0.0f);
		return CompressorSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	static FAtomBusEffectCompressorSettings& SetRatio(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, float Ratio)
	{
		CompressorSettings.Ratio = Ratio;
		return CompressorSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	static FAtomBusEffectCompressorSettings& SetAttackTime(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, float AttackTime)
	{
		CompressorSettings.AttackTime = FMath::Clamp(AttackTime, 0.0f, 3000.0f);
		return CompressorSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	static FAtomBusEffectCompressorSettings& SetReleaseTime(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, float ReleaseTime)
	{
		CompressorSettings.ReleaseTime = FMath::Clamp(ReleaseTime, 0.0f, 50000.0f);
		return CompressorSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	static FAtomBusEffectCompressorSettings& SetGain(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, float Gain)
	{
		CompressorSettings.Gain = FMath::Clamp(Gain, -96.0, 48.0f);
		return CompressorSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	static FAtomBusEffectCompressorSettings& SetSurroundLink(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, float SurroundLink)
	{
		CompressorSettings.SurroundLink = FMath::Clamp(SurroundLink, 0.0, 100.0f);
		return CompressorSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor|Split EQ")
	static FAtomBusEffectCompressorSettings& SetSplitEQFilterType(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, EAtomSplitEQFilterType FilterType)
	{
		CompressorSettings.SplitEQ.FilterType = FilterType;
		return CompressorSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor|Split EQ")
	static FAtomBusEffectCompressorSettings& SetSplitEQFrequency(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, float Frequency)
	{
		CompressorSettings.SplitEQ.Frequency = FMath::Clamp(Frequency, 24.0f, 24000.0f);
		return CompressorSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor|Split EQ")
	static FAtomBusEffectCompressorSettings& SetSplitEQQ(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, float Q)
	{
		CompressorSettings.SplitEQ.Q = FMath::Clamp(Q, 0.1f, 10.0f);
		return CompressorSettings;
	}

	/*UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	static FAtomBusEffectCompressorSettings& SetType(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, EAtomCompressorType Type)
	{
		CompressorSettings.Type = Type;
		return CompressorSettings;
	}*/

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	static FAtomBusEffectCompressorSettings& SetBypass(UPARAM(ref) FAtomBusEffectCompressorSettings& CompressorSettings, bool bBypass)
	{
		CompressorSettings.bBypass = bBypass;
		return CompressorSettings;
	}
};

/*
 * UAtomBusEffectCompressorPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectCompressorPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectCompressorPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectCompressor)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectCompressor)
	virtual float GetParameterValue(int ParameterIndex) const override;
	virtual void SetParameterValue(int ParameterIndex, float Value) override;
	//~ End UAtomBusEffectPreset Interface

public:

	//~ Begin UObject interface.
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject interface.

	// Sets runtime compressor settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Compressor", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectCompressorSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	void SetDefaultSettings(const FAtomBusEffectCompressorSettings& InSettings);

	// Set the compressor threshold, in decibels [-96, 0].
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	void SetThreshold(float Gain);

	// Set the compressor ratio.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	void SetRatio(float Ratio);

	// Set the compressor attack time, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	void SetAttackTime(float Time);

	// Set the compressor release time, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	void SetReleaseTime(float Time);

	// Set the output gain, in decibels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	void SetGain(float Gain);

	// Set the strength of the surround link.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	void SetSurroundLink(float Percent);

	// Set the split EQ filter type to operate.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	void SetSplitEQFilterType(EAtomSplitEQFilterType Type);

	// Set the split EQ filter frequency, in hertz.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	void SetSplitEQFrequency(float Frequency);

	// Set the split EQ resonance of the filter.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	void SetSplitEQQ(float Factor);

	// Set the amplitude detection value type.
	//UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Compressor")
	//void SetType(EAtomCompressorType Type);

public:

	/** The Delay preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectCompressorSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectCompressorSettings DynamicSettings;
};
