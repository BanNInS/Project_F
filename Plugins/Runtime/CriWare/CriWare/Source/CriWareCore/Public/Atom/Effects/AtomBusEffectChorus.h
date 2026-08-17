/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectChorus.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectChorus.generated.h"

 /*
  * FAtomBusEffectChorusSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectChorusSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectChorusSettings();

	//~ Begin effect parameters

	/** Specify the delay time of the chorus (delay at the center of the LFO). (milliseconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chorus", meta = (ClampMin = "0.1", ClampMax = "100.0", UIMin = "0.1", UIMax = "100.0", ForceUnits = ms))
	float DelayTime;

	/** Specifies the depth of the chorus LFO (amplitude). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chorus", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Depth;

	/** Specifies the rate of the chorus LFO in hertz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chorus", meta = (ClampMin = "0.01", ClampMax = "100.0", UIMin = "0.01", UIMax = "100.0", Units = Hz, SliderExponent = "10.0"))
	float Rate;

	/** Specifies the chorus feedack gain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chorus", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Feedback;

	/** Specifies the mixing ratio of the dry (original sound) of the chorus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chorus", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DryMix;

	/** Specifies the mixing ratio of the first tap wet (modified sound) of the chorus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chorus", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float WetMix1;

	/** Specifies the mixing ratio of the second tap wet (modified sound) of the chorus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chorus", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float WetMix2;

	/** Specifies the mixing ratio of the third tap wet (modified sound) of the chorus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chorus", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float WetMix3;

	//~ End effect parameters

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "0.1", ClampMax = "100.0", UIMin = "0.1", UIMax = "100.0", ForceUnits = ms))
	float MaximumDelayTime;

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectChorusStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectChorusStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	static FAtomBusEffectChorusSettings& SetDelayTime(UPARAM(ref) FAtomBusEffectChorusSettings& ChorusSettings, float DelayTime)
	{
		ChorusSettings.DelayTime = FMath::Clamp(DelayTime, 0.1f, 100.0f);
		return ChorusSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	static FAtomBusEffectChorusSettings& SetDepth(UPARAM(ref) FAtomBusEffectChorusSettings& ChorusSettings, float Depth)
	{
		ChorusSettings.Depth = FMath::Clamp(Depth, 0.0f, 1.0f);
		return ChorusSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	static FAtomBusEffectChorusSettings& SetRate(UPARAM(ref) FAtomBusEffectChorusSettings& ChorusSettings, float Rate)
	{
		ChorusSettings.Rate = FMath::Clamp(Rate, 0.01f, 100.0f);
		return ChorusSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	static FAtomBusEffectChorusSettings& SetFeedback(UPARAM(ref) FAtomBusEffectChorusSettings& ChorusSettings, float Feedback)
	{
		ChorusSettings.Feedback = FMath::Clamp(Feedback, 0.0f, 1.0f);
		return ChorusSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	static FAtomBusEffectChorusSettings& SetDryMix(UPARAM(ref) FAtomBusEffectChorusSettings& ChorusSettings, float DryMix)
	{
		ChorusSettings.DryMix = FMath::Clamp(DryMix, 0.0f, 1.0f);
		return ChorusSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	static FAtomBusEffectChorusSettings& SetWetMix1(UPARAM(ref) FAtomBusEffectChorusSettings& ChorusSettings, float WetMix1)
	{
		ChorusSettings.WetMix1 = FMath::Clamp(WetMix1, 0.0f, 1.0f);
		return ChorusSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	static FAtomBusEffectChorusSettings& SetWetMix2(UPARAM(ref) FAtomBusEffectChorusSettings& ChorusSettings, float WetMix2)
	{
		ChorusSettings.WetMix2 = FMath::Clamp(WetMix2, 0.0f, 1.0f);
		return ChorusSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	static FAtomBusEffectChorusSettings& SetWetMix3(UPARAM(ref) FAtomBusEffectChorusSettings& ChorusSettings, float WetMix3)
	{
		ChorusSettings.WetMix3 = FMath::Clamp(WetMix3, 0.0f, 1.0f);
		return ChorusSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	static FAtomBusEffectChorusSettings& SetMaximumDelayTime(UPARAM(ref) FAtomBusEffectChorusSettings& ChorusSettings, float MaximumDelayTime)
	{
		ChorusSettings.MaximumDelayTime = FMath::Clamp(MaximumDelayTime, 0.1f, 100.0f);
		ChorusSettings.DelayTime = FMath::Max(ChorusSettings.MaximumDelayTime, ChorusSettings.DelayTime);
		return ChorusSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	static FAtomBusEffectChorusSettings& SetBypass(UPARAM(ref) FAtomBusEffectChorusSettings& ChorusSettings, bool bBypass)
	{
		ChorusSettings.bBypass = bBypass;
		return ChorusSettings;
	}
};

/*
 * UAtomBusEffectChorusPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectChorusPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectChorusPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectChorus)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectChorus)
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

	// Sets runtime Chorus settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Chorus", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectChorusSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	void SetDefaultSettings(const FAtomBusEffectChorusSettings& InSettings);

	// Set the delay time in millisecondes.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	void SetDelayTime(float Time);

	// Set depth, amplitude of LFO.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	void SetDepth(float Level);

	// Set the LFO rate in hertz.	
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	void SetRate(float Rate);

	// Set the feedback level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	void SetFeedback(float Level);

	// Set the dry mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	void SetDryMix(float Level);

	// Get the first tap wet mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	void SetWetMix1(float Level);

	// Get the second tap wet mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	void SetWetMix2(float Level);

	// Get the third tap wet mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	void SetWetMix3(float Level);

	// Get the maximum delay time possible, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Chorus")
	float GetMaxDelayTime() const
	{
		return DynamicSettings.MaximumDelayTime;
	};

public:

	/** The Chorus preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectChorusSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectChorusSettings DynamicSettings;
};
