/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectSurrounderWithEQ.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"
#include "Atom/Effects/AtomBusEffectMultiBandEQ.h"

#include "AtomBusEffectSurrounderWithEQ.generated.h"

UENUM(BlueprintType)
enum class EAtomSurrounderMode : uint8
{
	Straight,
	Cross,
	Matrix
};

 /*
  * FAtomBusEffectSurrounderWithEQSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectSurrounderWithEQSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectSurrounderWithEQSettings();

	//~ Begin effect parameters

	/** Specifies the surrounder mode to operate.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surround With EQ")
	EAtomSurrounderMode Mode;

	/** Specifies the delay time of the surrounder in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surround With EQ", meta = (ClampMin = "0.0", ClampMax = "500.0", UIMin = "0.0", UIMax = "500.0", ForceUnits = ms))
	float DelayTime;

	/** Specify the final output level of the surrounder in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surround With EQ", meta = (ClampMin = "-96.0", ClampMax = "48.0", UIMin = "-96.0", UIMax = "48.0", SliderExponent = "0.75"))
	float Gain;

protected:

	/** Specifies the EQ filters (biquad filters) of the multi-band equalizer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surround With EQ", EditFixedSize, meta = (EditFixedOrder))
	TArray<FAtomBandEQFilter> BandFilters;

	//~ End effect parameters

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "0.0", ClampMax = "500.0", UIMin = "0.0", UIMax = "500.0", ForceUnits = ms))
	float MaximumDelayTime;

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;

	// c++ accessors for band filters.
	FORCEINLINE FAtomBandEQFilter& GetBandFilter(int BandIndex)
	{
		return BandFilters[FMath::Clamp(BandIndex, 0, BandFilters.Num() - 1)];
	}

	FORCEINLINE const FAtomBandEQFilter& GetBandFilter(int BandIndex) const
	{
		return BandFilters[FMath::Clamp(BandIndex, 0, BandFilters.Num() - 1)];
	}

	friend class UAtomBusEffectSurrounderWithEQPreset;
};

/*
 * UAtomBusEffectSurrounderWithEQStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectSurrounderWithEQStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	static FAtomBusEffectSurrounderWithEQSettings& SetMode(UPARAM(ref) FAtomBusEffectSurrounderWithEQSettings& SurrounderWithEQSettings, EAtomSurrounderMode Mode)
	{
		SurrounderWithEQSettings.Mode = Mode;
		return SurrounderWithEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	static FAtomBusEffectSurrounderWithEQSettings& SetDelayTime(UPARAM(ref) FAtomBusEffectSurrounderWithEQSettings& SurrounderWithEQSettings, float DelayTime)
	{
		SurrounderWithEQSettings.DelayTime = FMath::Clamp(DelayTime, 0.0f, 500.0f);
		//SurrounderWithEQSettings.MaximumRoomSize = FMath::Max(DelayTime, SurrounderWithEQSettings.MaximumDelayTime);
		return SurrounderWithEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	static FAtomBusEffectSurrounderWithEQSettings& SetGain(UPARAM(ref) FAtomBusEffectSurrounderWithEQSettings& SurrounderWithEQSettings, float Gain)
	{
		SurrounderWithEQSettings.Gain = FMath::Clamp(Gain, -96.0, 48.0f);
		return SurrounderWithEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	static FAtomBusEffectSurrounderWithEQSettings& SetBandFilterType(UPARAM(ref) FAtomBusEffectSurrounderWithEQSettings& SurrounderWithEQSettings, int BandIndex, EAtomBandEQFilterType FilterType)
	{
		SurrounderWithEQSettings.GetBandFilter(BandIndex).FilterType = FilterType;
		return SurrounderWithEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	static FAtomBusEffectSurrounderWithEQSettings& SetBandFrequency(UPARAM(ref) FAtomBusEffectSurrounderWithEQSettings& SurrounderWithEQSettings, int BandIndex, float Frequency)
	{
		SurrounderWithEQSettings.GetBandFilter(BandIndex).Frequency = FMath::Clamp(Frequency, 24.0f, 24000.0f);
		return SurrounderWithEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	static FAtomBusEffectSurrounderWithEQSettings& SetBandQ(UPARAM(ref) FAtomBusEffectSurrounderWithEQSettings& SurrounderWithEQSettings, int BandIndex, float Q)
	{
		SurrounderWithEQSettings.GetBandFilter(BandIndex).Q = FMath::Clamp(Q, 0.1f, 10.0f);
		return SurrounderWithEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	static FAtomBusEffectSurrounderWithEQSettings& SetBandGain(UPARAM(ref) FAtomBusEffectSurrounderWithEQSettings& SurrounderWithEQSettings, int BandIndex, float Gain)
	{
		SurrounderWithEQSettings.GetBandFilter(BandIndex).Gain = FMath::Clamp(Gain, 0.0f, 5.0f);
		return SurrounderWithEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	static FAtomBusEffectSurrounderWithEQSettings& SetMaximumDelayTime(UPARAM(ref) FAtomBusEffectSurrounderWithEQSettings& SurrounderWithEQSettings, float MaximumDelayTime)
	{
		SurrounderWithEQSettings.MaximumDelayTime = FMath::Clamp(MaximumDelayTime, 0.0f, 500.0f);
		SurrounderWithEQSettings.DelayTime = FMath::Max(SurrounderWithEQSettings.MaximumDelayTime, SurrounderWithEQSettings.DelayTime);
		return SurrounderWithEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	static FAtomBusEffectSurrounderWithEQSettings& SetBypass(UPARAM(ref) FAtomBusEffectSurrounderWithEQSettings& SurrounderWithEQSettings, bool bBypass)
	{
		SurrounderWithEQSettings.bBypass = bBypass;
		return SurrounderWithEQSettings;
	}
};

/*
 * UAtomBusEffectSurrounderWithEQPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectSurrounderWithEQPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectSurrounderWithEQPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectSurrounderWithEQ)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectSurrounderWithEQ)
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

	// Sets runtime Multi-Band EQ settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Surrounder With EQ", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectSurrounderWithEQSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	void SetDefaultSettings(const FAtomBusEffectSurrounderWithEQSettings& InSettings);

	// Set the surrounder mode to operate.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	void SetMode(EAtomSurrounderMode Mode);

	// Set the delay time, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	void SetDelayTime(float Time);

	// Set the output gain, in decibels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	void SetGain(float Gain);

	// Set the filter type to operate.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	void SetBandFilterType(int BandIndex, EAtomBandEQFilterType Type);

	// Set the filter frequency, in hertz.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	void SetBandFrequency(int BandIndex, float Frequency);

	// Set the resonance of the filter.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	void SetBandQ(int BandIndex, float Factor);

	// Set the output gain of the filter.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Surrounder With EQ")
	void SetBandGain(int BandIndex, float Level);

	// Get the maximum delay time possible.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Delay")
	float GetMaxDelayTime() const
	{
		return DynamicSettings.MaximumDelayTime;
	};

public:

	/** The SurrounderWithEQ preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectSurrounderWithEQSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectSurrounderWithEQSettings DynamicSettings;
};

