/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectDistortion.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectDistortion.generated.h"

 /*
  * FAtomBusEffectDistortionSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectDistortionSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectDistortionSettings();

	//~ Begin effect parameters

	/** Specifies the strength of distortion (drive) in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distortion", meta = (ClampMin = "0.0", ClampMax = "48.0", UIMin = "0.0", UIMax = "48.0"))
	float Drive;

	/** Specifies the mixing ratio of the dry (original sound) component of the distortion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distortion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DryMix;

	/** Specifies the mixing ratio of the wet (distortion) component of the distortion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distortion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float WetMix;

	/** Specify the final output level of the distortion in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Distortion", meta = (ClampMin = "-96.0", ClampMax = "48.0", UIMin = "-96.0", UIMax = "48.0", SliderExponent = "0.75"))
	float Gain;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectDistortionStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectDistortionStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Distortion")
	static FAtomBusEffectDistortionSettings& SetDrive(UPARAM(ref) FAtomBusEffectDistortionSettings& DistortionSettings, float Drive)
	{
		DistortionSettings.Drive = FMath::Clamp(Drive, 0.0f, 48.0f);
		return DistortionSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Distortion")
	static FAtomBusEffectDistortionSettings& SetDryMix(UPARAM(ref) FAtomBusEffectDistortionSettings& DistortionSettings, float DryMix)
	{
		DistortionSettings.DryMix = FMath::Clamp(DryMix, 0.0f, 1.0f);
		return DistortionSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Distortion")
	static FAtomBusEffectDistortionSettings& SetWetMix(UPARAM(ref) FAtomBusEffectDistortionSettings& DistortionSettings, float WetMix)
	{
		DistortionSettings.WetMix = FMath::Clamp(WetMix, 0.0f, 1.0f);
		return DistortionSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Distortion")
	static FAtomBusEffectDistortionSettings& SetGain(UPARAM(ref) FAtomBusEffectDistortionSettings& DistortionSettings, float Gain)
	{
		DistortionSettings.Gain = FMath::Clamp(Gain, -96.0, 48.0f);
		return DistortionSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Distortion")
	static FAtomBusEffectDistortionSettings& SetBypass(UPARAM(ref) FAtomBusEffectDistortionSettings& DistortionSettings, bool bBypass)
	{
		DistortionSettings.bBypass = bBypass;
		return DistortionSettings;
	}
};

/*
 * UAtomBusEffectDistortionPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectDistortionPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectDistortionPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectDistortion)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectDistortion)
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

	// Sets runtime Distortion settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Distortion", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectDistortionSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Distortion")
	void SetDefaultSettings(const FAtomBusEffectDistortionSettings& InSettings);

	// Set the drive, in decibels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Distortion")
	void SetDrive(float Drive);

	// Set the dry mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Distortion")
	void SetDryMix(float Level);

	// Set the wet mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Distortion")
	void SetWetMix(float Level);

	// Set the output gain, in decibels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Distortion")
	void SetGain(float Gain);

public:

	/** The Distortion preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectDistortionSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectDistortionSettings DynamicSettings;
};
