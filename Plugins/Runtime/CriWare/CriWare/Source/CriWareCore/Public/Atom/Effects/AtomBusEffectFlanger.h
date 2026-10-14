/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectFlanger.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectFlanger.generated.h"

 /*
  * FAtomBusEffectFlangerSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectFlangerSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectFlangerSettings();

	//~ Begin effect parameters

	/** Specify the delay time of the flanger (delay at the center of the LFO). (milliseconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flanger", meta = (ClampMin = "0.1", ClampMax = "10.0", UIMin = "0.1", UIMax = "10.0", ForceUnits = ms))
	float DelayTime;

	/** Specifies the depth of the flanger LFO (amplitude). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flanger", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Depth;

	/** Specifies the rate of the flanger LFO. (Hz) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flanger", meta = (ClampMin = "0.01", ClampMax = "100.0", UIMin = "0.01", UIMax = "100.0", Units = Hz, SliderExponent = "10.0"))
	float Rate;

	/** Specifies the flanger feedack gain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flanger", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Feedback;

	/** Specifies the mixing ratio of the dry (original sound) of the flanger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flanger", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DryMix;

	/** Specifies the mixing ratio of the wet (modified sound) of the flanger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flanger", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float WetMix;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectFlangerStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectFlangerStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	static FAtomBusEffectFlangerSettings& SetDelayTime(UPARAM(ref) FAtomBusEffectFlangerSettings& FlangerSettings, float DelayTime)
	{
		FlangerSettings.DelayTime = FMath::Clamp(DelayTime, 0.1f, 10.0f);
		return FlangerSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	static FAtomBusEffectFlangerSettings& SetDepth(UPARAM(ref) FAtomBusEffectFlangerSettings& FlangerSettings, float Depth)
	{
		FlangerSettings.Depth = FMath::Clamp(Depth, 0.0f, 1.0f);
		return FlangerSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	static FAtomBusEffectFlangerSettings& SetRate(UPARAM(ref) FAtomBusEffectFlangerSettings& FlangerSettings, float Rate)
	{
		FlangerSettings.Rate = FMath::Clamp(Rate, 0.01f, 100.0f);
		return FlangerSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	static FAtomBusEffectFlangerSettings& SetFeedback(UPARAM(ref) FAtomBusEffectFlangerSettings& FlangerSettings, float Feedback)
	{
		FlangerSettings.Feedback = FMath::Clamp(Feedback, 0.0f, 1.0f);
		return FlangerSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	static FAtomBusEffectFlangerSettings& SetDryMix(UPARAM(ref) FAtomBusEffectFlangerSettings& FlangerSettings, float DryMix)
	{
		FlangerSettings.DryMix = FMath::Clamp(DryMix, 0.0f, 1.0f);
		return FlangerSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	static FAtomBusEffectFlangerSettings& SetWetMix(UPARAM(ref) FAtomBusEffectFlangerSettings& FlangerSettings, float WetMix)
	{
		FlangerSettings.WetMix = FMath::Clamp(WetMix, 0.0f, 1.0f);
		return FlangerSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	static FAtomBusEffectFlangerSettings& SetBypass(UPARAM(ref) FAtomBusEffectFlangerSettings& FlangerSettings, bool bBypass)
	{
		FlangerSettings.bBypass = bBypass;
		return FlangerSettings;
	}
};

/*
 * UAtomBusEffectFlangerPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectFlangerPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectFlangerPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectFlanger)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectFlanger)
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

	// Sets runtime Flanger settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Flanger", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectFlangerSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	void SetDefaultSettings(const FAtomBusEffectFlangerSettings& InSettings);

	// Set the delay time in millisecondes.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	void SetDelayTime(float Time);

	// Set depth, amplitude of LFO.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	void SetDepth(float Level);

	// Set the LFO rate in hertz.	
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	void SetRate(float Rate);

	// Set the feedback level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	void SetFeedback(float Level);

	// Set the dry mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	void SetDryMix(float Level);

	// Get the wet mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Flanger")
	void SetWetMix(float Level);

public:

	/** The Flanger preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectFlangerSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectFlangerSettings DynamicSettings;
};
