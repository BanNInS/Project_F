/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectBandpassFilter.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectBandpassFilter.generated.h"

 /*
  * FAtomBusEffectBandpassFilterSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectBandpassFilterSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectBandpassFilterSettings();

	//~ Begin effect parameters

	/** Specifies the low cutoff frequency of the band pass filter output in hertz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bandpass Filter", meta = (ClampMin = "24.0", ClampMax = "24000.0", UIMin = "24.0", UIMax = "24000.0", Units = Hz, SliderExponent = "10.0"))
	float LowCutoff;

	/** Specifies the high cutoff frequency of the band pass filter output in hertz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bandpass Filter", meta = (ClampMin = "24.0", ClampMax = "24000.0", UIMin = "24.0", UIMax = "24000.0", Units = Hz, SliderExponent = "10.0"))
	float HighCutoff;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectBandpassFilterStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectBandpassFilterStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BandpassFilter")
	static FAtomBusEffectBandpassFilterSettings& SetLowCutoff(UPARAM(ref) FAtomBusEffectBandpassFilterSettings& BandpassFilterSettings, float LowCutoff)
	{
		BandpassFilterSettings.LowCutoff = FMath::Clamp(LowCutoff, 24.0f, 24000.0f);
		return BandpassFilterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BandpassFilter")
	static FAtomBusEffectBandpassFilterSettings& SetHighCutoff(UPARAM(ref) FAtomBusEffectBandpassFilterSettings& BandpassFilterSettings, float HighCutoff)
	{
		BandpassFilterSettings.HighCutoff = FMath::Clamp(HighCutoff, 24.0f, 24000.0f);
		return BandpassFilterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BandpassFilter")
	static FAtomBusEffectBandpassFilterSettings& SetBypass(UPARAM(ref) FAtomBusEffectBandpassFilterSettings& BandpassFilterSettings, bool bBypass)
	{
		BandpassFilterSettings.bBypass = bBypass;
		return BandpassFilterSettings;
	}
};

/*
 * UAtomBusEffectBandpassFilterPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectBandpassFilterPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectBandpassFilterPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectBandpassFilter)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectBandpassFilter)
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

	// Sets runtime BandpassFilter settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|BandpassFilter", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectBandpassFilterSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BandpassFilter")
	void SetDefaultSettings(const FAtomBusEffectBandpassFilterSettings& InSettings);

	// Set the low cutoff frequency, in hertz.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BandpassFilter")
	void SetLowCutoff(float Frenquency);

	// Set the high cutoff frequency, in hertz.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BandpassFilter")
	void SetHighCutoff(float Frenquency);

public:

	/** The BandpassFilter preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectBandpassFilterSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectBandpassFilterSettings DynamicSettings;
};
