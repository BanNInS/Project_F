/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectI3DL2Reverb.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectI3DL2Reverb.generated.h"

 /*
  * FAtomBusEffectI3DL2ReverbSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectI3DL2ReverbSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectI3DL2ReverbSettings();

	//~ Begin effect parameters

	/** Specifies the final output volume of the I3DL2 reverb in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "-100.0", ClampMax = "0.0", UIMin = "-100.0", UIMax = "0.0", SliderExponent = "0.75"))
	float Room;

	/** Specifies the volume of the reference frequency (high frequency) of the I3DL2 reverb in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "-100.0", ClampMax = "0.0", UIMin = "-100.0", UIMax = "0.0", SliderExponent = "0.75"))
	float RoomHF;

	/** Specifies the decay time of the rear reverberation of the I3DL2 reverb in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "0.1", ClampMax = "20.0", UIMin = "0.1", UIMax = "20.0", ForceUnits = ms))
	float DecayTime;

	/** specifies the ratio of high frequency attenuation to low frequency attenuation in the rear reverberation of the I3DL2 reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "0.1", ClampMax = "2.0", UIMin = "0.1", UIMax = "2.0", Units = x))
	float DecayHFRatio;
	
	/** Specifies the volume of the initial reflected sound of the I3DL2 reverb in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "-100.0", ClampMax = "10.0", UIMin = "-100.0", UIMax = "10.0", SliderExponent = "0.75"))
	float Reflections;

	/** Specifies the delay time of the initial reflected sound of the I3DL2 reverb in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "0.0", ClampMax = "0.3", UIMin = "0.0", UIMax = "0.3", ForceUnits = ms))
	float ReflectionsDelay;

	/** Specifies the volume of the rear reverberation of the I3DL2 reverb in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "-100.0", ClampMax = "20.0", UIMin = "-100.0", UIMax = "20.0", SliderExponent = "0.75"))
	float Reverb;

	/** Specifies the delay time of the rear reverberation of the I3DL2 reverb in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "0.0", ClampMax = "0.1", UIMin = "0.0", UIMax = "0.1", ForceUnits = ms))
	float ReverbDelay;

	/** Specifies the echo density of the rear reverberation of the I3DL2 reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", Units = %))
	float Diffusion;

	/** Specifies the modal density of the rear reverberation of the I3DL2 reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", Units = %))
	float Density;

	/** Specifies the reference frequency (high range) of Room HF of the I3DL2 reverb in hertz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "20.0", ClampMax = "20000.0", UIMin = "20.0", UIMax = "20000.0", Units = Hz, SliderExponent = "10.0"))
	float HFReference;

	/** Specifies the input level of the front (L, R channels) component of the I3DL2 reverb in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "-100.0", ClampMax = "0.0", UIMin = "-100.0", UIMax = "0.0", SliderExponent = "0.75"))
	float FrontInput;

	/** Specifies the input level of the rear (surround L, surround R channels) component of the I3DL2 reverb in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "-100.0", ClampMax = "0.0", UIMin = "-100.0", UIMax = "0.0", SliderExponent = "0.75"))
	float RearInput;

	/** Specifies the input level of the center (C channel) component of the I3DL2 reverb in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "-100.0", ClampMax = "0.0", UIMin = "-100.0", UIMax = "0.0", SliderExponent = "0.75"))
	float CenterInput;

	/** Specifies the output level of the front (L, R channels) component of the I3DL2 reverb in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "-100.0", ClampMax = "0.0", UIMin = "-100.0", UIMax = "0.0", SliderExponent = "0.75"))
	float FrontOutput;

	/** Specifies the output level of the rear (surround L, surround R channels) component of the I3DL2 reverb in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "-100.0", ClampMax = "0.0", UIMin = "-100.0", UIMax = "0.0", SliderExponent = "0.75"))
	float RearOutput;

	/** Specifies the output level of the center (C channel) component of the I3DL2 reverb in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "I3DL2Reverb", meta = (ClampMin = "-100.0", ClampMax = "0.0", UIMin = "-100.0", UIMax = "0.0", SliderExponent = "0.75"))
	float CenterOutput;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectI3DL2ReverbStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectI3DL2ReverbStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetRoom(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float Room)
	{
		//DelaySettings.MaximumDelayLength = FMath::Max(0.0f, MaximumDelayLength);
		//DelaySettings.DelayLength = FMath::Min(DelaySettings.MaximumDelayLength, DelaySettings.DelayLength);
		ReverbSettings.Room = FMath::Clamp(Room, -100.0f, 0.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetRoomHF(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float RoomHF)
	{
		ReverbSettings.RoomHF = FMath::Clamp(RoomHF, -100.0f, 0.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetDecayTime(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float DecayTime)
	{
		ReverbSettings.DecayTime = FMath::Clamp(DecayTime, 0.1f, 20.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetDecayHFRatio(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float DecayHFRatio)
	{
		ReverbSettings.DecayHFRatio = FMath::Clamp(DecayHFRatio, 0.1f, 2.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetReflections(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float Reflections)
	{
		ReverbSettings.Reflections = FMath::Clamp(Reflections, -100.0f, 10.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetReflectionsDelay(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float ReflectionsDelay)
	{
		ReverbSettings.ReflectionsDelay = FMath::Clamp(ReflectionsDelay, 0.0f, 0.3f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetReverb(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float Reverb)
	{
		ReverbSettings.Reverb = FMath::Clamp(Reverb, -100.0f, 20.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetReverbDelay(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float ReverbDelay)
	{
		ReverbSettings.ReverbDelay = FMath::Clamp(ReverbDelay, 0.0f, 0.1f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetDiffusion(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float Diffusion)
	{
		ReverbSettings.Diffusion = FMath::Clamp(Diffusion, 0.0f, 100.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetDensity(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float Density)
	{
		ReverbSettings.Density = FMath::Clamp(Density, 0.0f, 100.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetHFReference(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float HFReference)
	{
		ReverbSettings.HFReference = FMath::Clamp(HFReference, 20.0f, 20000.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetFrontInput(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float FrontInput)
	{
		ReverbSettings.FrontInput = FMath::Clamp(FrontInput, -100.0f, 0.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetRearInput(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float RearInput)
	{
		ReverbSettings.RearInput = FMath::Clamp(RearInput, -100.0f, 0.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetCenterInput(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float CenterInput)
	{
		ReverbSettings.CenterInput = FMath::Clamp(CenterInput, -100.0f, 0.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetFrontOutput(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float FrontOutput)
	{
		ReverbSettings.FrontOutput = FMath::Clamp(FrontOutput, -100.0f, 0.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetRearOutput(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float RearOutput)
	{
		ReverbSettings.RearOutput = FMath::Clamp(RearOutput, -100.0f, 0.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetCenterOutput(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, float CenterOutput)
	{
		ReverbSettings.CenterOutput = FMath::Clamp(CenterOutput, -100.0f, 0.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	static FAtomBusEffectI3DL2ReverbSettings& SetBypass(UPARAM(ref) FAtomBusEffectI3DL2ReverbSettings& ReverbSettings, bool bBypass)
	{
		ReverbSettings.bBypass = bBypass;
		return ReverbSettings;
	}
};

/*
 * UAtomBusEffectI3DL2ReverbPreset Class
 * Interactive 3D Audio Level 2 reverberation (I3DL2)
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectI3DL2ReverbPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectI3DL2ReverbPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectI3DL2Reverb)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectI3DL2Reverb)
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

	// Sets runtime reverb settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|I3DL2Reverb", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectI3DL2ReverbSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetDefaultSettings(const FAtomBusEffectI3DL2ReverbSettings& InSettings);

	// Set the final output volume.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetRoom(float Gain);

	// the volume of the reference frequency
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetRoomHF(float Gain);

	// Set the decay time of the rear reverberation in miliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetDecayTime(float Time);

	// Set the ratio of high frequency attenuation to low frequency attenuation in the rear reverberation. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetDecayHFRatio(float Ratio);

	// Set the volume of the relefections in miliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetReflections(float Gain);

	// Set the delay time of the relefections in miliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetReflectionsDelay(float Time);

	// Set the volume of the rear reverberation.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetReverb(float Gain);

	// Set the delay time of the rear reverberation in miliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetReverbDelay(float Time);

	// Set the echo density of the rear reverberation.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetDiffusion(float Percent);

	// Set the modal density of the rear reverberation.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetDensity(float Percent);

	/** Set the reference frequency (high range) of Room HF. */
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetHFReference(float Frequency);

	// Set the volume of the front input.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetFrontInput(float Gain);

	// Set the volume of the rear input.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetRearInput(float Gain);

	// Set the volume of the center input.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetCenterInput(float Gain);

	// Set the volume of the front output.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetFrontOutput(float Gain);

	// Set the volume of the rear output.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetRearOutput(float Gain);

	// Set the volume of the center output.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|I3DL2Reverb")
	void SetCenterOutput(float Gain);

private:

#if WITH_EDITOR
	/** Check if the actual settings match a preset value for I3DL2Reverb defined in Atom Asr. */
	bool IsAtomAsrPreset(float AsrPreset[11]);
#endif

public:

	/** The reverb preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectI3DL2ReverbSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectI3DL2ReverbSettings DynamicSettings;
};

/*
 * UAtomBusEffectI3DL2Reverb2Preset Class
 * Interactive 3D Audio Level 2 reverberation version 2 (I3DL2 ver. 2)
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectI3DL2Reverb2Preset
	: public UAtomBusEffectI3DL2ReverbPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectI3DL2Reverb2Preset(const FObjectInitializer& ObjectInitializer);
};
