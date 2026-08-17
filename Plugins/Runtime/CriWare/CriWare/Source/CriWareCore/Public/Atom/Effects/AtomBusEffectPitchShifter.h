/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectPitchShifter.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectPitchShifter.generated.h"

UENUM(BlueprintType)
enum class EAtomPitchShifterMode : uint8
{
	Music,
	Vocal,
	SoundEffects,
	Speech
};

 /*
  * FAtomBusEffectPitchShifterSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectPitchShifterSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectPitchShifterSettings();

	//~ Begin effect parameters

	/** Specify the shift amount in semitones of the pitch shifter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch Shifter", meta = (ClampMin = "-24", ClampMax = "24", UIMin = "-24", UIMax = "24"))
	float PitchShift;

	/** Specifies the shift amount in semitones of the formant (harmonic frequencies that occur in the human voice) of the pitch shifter when using Vocal or Speach modes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch Shifter", meta = (ClampMin = "-24", ClampMax = "24", UIMin = "-24", UIMax = "24"))
	float FormantShift;

	/** Specifies the operation mode of the pitch shifter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch Shifter")
	EAtomPitchShifterMode ShiftMode;

	/** Advanced - Specify the pitch frequency of the input audio data in hertz.
	 * This is hint information for formant analysis, but please be aware that if you specify a value 
	 * that is far from the actual voice data, noise will be added to the voice. 
	 * If not specified, the default pitch (459.375 Hz) is used internally. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch Shifter", AdvancedDisplay, meta = (EditCondition = "bUseDataPitchFrequency", ClampMin = "24.0", ClampMax = "24000.0", UIMin = "24.0", UIMax = "24000.0", Units = Hz, SliderExponent = "10.0"))
	float DataPitchFrequency;

	//~ End effect parameters

	/** Advanced - Enable usage of DataPitchFrenquency. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch Shifter", AdvancedDisplay, meta = (InlineEditConditionToggle, DisplayAfter = "FormantShift"))
	bool bUseDataPitchFrequency;

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectPitchShifterStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectPitchShifterStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Pitch Shifter")
	static FAtomBusEffectPitchShifterSettings& SetPitchShift(UPARAM(ref) FAtomBusEffectPitchShifterSettings& PitchShifterSettings, int PitchShift)
	{
		PitchShifterSettings.PitchShift = FMath::Clamp(PitchShift, -2400, 2400);
		return PitchShifterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Pitch Shifter")
	static FAtomBusEffectPitchShifterSettings& SetFormantShift(UPARAM(ref) FAtomBusEffectPitchShifterSettings& PitchShifterSettings, int FormantShift)
	{
		PitchShifterSettings.FormantShift = FMath::Clamp(FormantShift, -2400, 2400);
		return PitchShifterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Pitch Shifter")
	static FAtomBusEffectPitchShifterSettings& SetShiftMode(UPARAM(ref) FAtomBusEffectPitchShifterSettings& PitchShifterSettings, EAtomPitchShifterMode ShiftMode)
	{
		PitchShifterSettings.ShiftMode = ShiftMode;
		return PitchShifterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Pitch Shifter")
	static FAtomBusEffectPitchShifterSettings& SetDataPitchFrequency(UPARAM(ref) FAtomBusEffectPitchShifterSettings& PitchShifterSettings, float DataPitchFrequency)
	{
		PitchShifterSettings.bUseDataPitchFrequency = true;
		PitchShifterSettings.DataPitchFrequency = FMath::Clamp(DataPitchFrequency, 24.0f, 24000.0f);
		return PitchShifterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Pitch Shifter")
	static FAtomBusEffectPitchShifterSettings& SetBypass(UPARAM(ref) FAtomBusEffectPitchShifterSettings& PitchShifterSettings, bool bBypass)
	{
		PitchShifterSettings.bBypass = bBypass;
		return PitchShifterSettings;
	}
};

/*
 * UAtomBusEffectPitchShifterPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectPitchShifterPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectPitchShifterPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectPitchShifter)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectPitchShifter)
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

	// Sets runtime PitchShifter settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Pitch Shifter", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectPitchShifterSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Pitch Shifter")
	void SetDefaultSettings(const FAtomBusEffectPitchShifterSettings& InSettings);

	// Set the shift amount.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Pitch Shifter")
	void SetPitchShift(int Cents);

	// Set the shift amount of the formant.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Pitch Shifter")
	void SetFormantShift(int Cents);

	// Set the operation mode.	
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Pitch Shifter")
	void SetShiftMode(EAtomPitchShifterMode Mode);

	// Adavanced - Set the data pitch frequency. This is hint information for formant analysis.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Pitch Shifter")
	void SetDataPitchFrequency(float Frequency);

public:

	/** The PitchShifter preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectPitchShifterSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectPitchShifterSettings DynamicSettings;
};
