/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectBiquadFilter.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectBiquadFilter.generated.h"

UENUM(BlueprintType)
enum class EAtomBiquadFilterType : uint8
{
	LowPass,
	HighPass,
	Notch,
	LowShelf,
	HighShelf,
	Peaking
};

 /*
  * FAtomBusEffectBiquadFilterSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectBiquadFilterSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectBiquadFilterSettings();

	//~ Begin effect parameters

	/** Specifies the filter type to operate for the biquad filter.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biquad Filter")
	EAtomBiquadFilterType FilterType;

	/** Specifies the cutoff frequency of the biquad filter. (Hz) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biquad Filter", meta = (ClampMin = "24.0", ClampMax = "24000.0", UIMin = "24.0", UIMax = "24000.0", Units = Hz, SliderExponent = "10.0"))
	float Cutoff;

	/** Specifies the Q factor (resonance) of the biquad filter.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biquad Filter", meta = (ClampMin = "0.1", ClampMax = "10.0", UIMin = "0.1", UIMax = "10.0", SliderExponent = "5.0"))
	float Q;

	/** Specifies the biquad filter output gain for low/high shelf or Peaking filters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biquad Filter", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
	float Gain;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectBiquadFilterStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectBiquadFilterStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BiquadFilter")
	static FAtomBusEffectBiquadFilterSettings& SetLowCutoff(UPARAM(ref) FAtomBusEffectBiquadFilterSettings& BiquadFilterSettings, EAtomBiquadFilterType FilterType)
	{
		BiquadFilterSettings.FilterType = FilterType;
		return BiquadFilterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BiquadFilter")
	static FAtomBusEffectBiquadFilterSettings& SetCutoff(UPARAM(ref) FAtomBusEffectBiquadFilterSettings& BiquadFilterSettings, float Cutoff)
	{
		BiquadFilterSettings.Cutoff = FMath::Clamp(Cutoff, 24.0f, 24000.0f);
		return BiquadFilterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BiquadFilter")
		static FAtomBusEffectBiquadFilterSettings& SetQ(UPARAM(ref) FAtomBusEffectBiquadFilterSettings& BiquadFilterSettings, float Q)
	{
		BiquadFilterSettings.Q = FMath::Clamp(Q, 0.1f, 10.0f);
		return BiquadFilterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BiquadFilter")
	static FAtomBusEffectBiquadFilterSettings& SetGain(UPARAM(ref) FAtomBusEffectBiquadFilterSettings& BiquadFilterSettings, float Gain)
	{
		BiquadFilterSettings.Gain = FMath::Clamp(Gain, 0.0f, 5.0f);
		return BiquadFilterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BiquadFilter")
	static FAtomBusEffectBiquadFilterSettings& SetBypass(UPARAM(ref) FAtomBusEffectBiquadFilterSettings& BiquadFilterSettings, bool bBypass)
	{
		BiquadFilterSettings.bBypass = bBypass;
		return BiquadFilterSettings;
	}
};

/*
 * UAtomBusEffectBiquadFilterPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectBiquadFilterPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectBiquadFilterPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectBiquadFilter)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectBiquadFilter)
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

	// Sets runtime BiquadFilter settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|BiquadFilter", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectBiquadFilterSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BiquadFilter")
	void SetDefaultSettings(const FAtomBusEffectBiquadFilterSettings& InSettings);

	// Set the type of filter to operate.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BiquadFilter")
	void SetFilterType(EAtomBiquadFilterType Type);

	// Set the cutoff frequency, in hertz.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BiquadFilter")
	void SetCutoff(float Frequency);

	// Set the resonance of the filter.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BiquadFilter")
	void SetQ(float Factor);

	// Set the output gain for shelf or peaking filters.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BiquadFilter")
	void SetGain(float Level);

public:

	/** The BiquadFilter preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectBiquadFilterSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectBiquadFilterSettings DynamicSettings;
};
