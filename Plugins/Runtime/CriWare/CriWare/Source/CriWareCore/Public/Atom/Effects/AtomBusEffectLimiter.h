/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectLimiter.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectLimiter.generated.h"

UENUM(BlueprintType)
enum class EAtomLimiterType : uint8
{
	Peak,
	RMS
};

 /*
  * FAtomBusEffectLimiterSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectLimiterSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectLimiterSettings();

	//~ Begin effect parameters
		
	/** Specifies the limiter threshold for amplitude compression in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limiter", meta = (ClampMin = "-96.0", ClampMax = "0.0", UIMin = "-96.0", UIMax = "0.0", SliderExponent = "0.75"))
	float Threshold;

	/** Specifies the attack time of the limiter in milliseconds.
	 * The time until compression is completed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limiter", meta = (ClampMin = "0.0", ClampMax = "3000.0", UIMin = "0.0", UIMax = "3000.0", ForceUnits = ms))
	float AttackTime;

	/** Specifies the release time of the limiter in milliseconds.
	 * The time from when the amplitude becomes below the threshold until the end of compression. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limiter", meta = (ClampMin = "0.0", ClampMax = "50000.0", UIMin = "0.0", UIMax = "50000.0", ForceUnits = ms))
	float ReleaseTime;

	/** Specify the final output gain of the limiter in decibels (dB). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limiter", meta = (ClampMin = "-96.0", ClampMax = "48.0", UIMin = "-96.0", UIMax = "48.0", SliderExponent = "0.75"))
	float Gain;

	/** Specify the surround link strength of the limiter.
	 * Surround link function changes the compression strength using the lowest amplitude value in all channels.
	 * 100% fully uses the lowest amplitude value in all channels, while 0% turns off the surround link function.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limiter", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", Units = %))
	float SurroundLink;

	/** Specifies the limiter amplitude detection value type.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Limiter")
	EAtomLimiterType Type;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectLimiterStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectLimiterStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	static FAtomBusEffectLimiterSettings& SetThreshold(UPARAM(ref) FAtomBusEffectLimiterSettings& LimiterSettings, float Threshold)
	{
		LimiterSettings.Threshold = FMath::Clamp(Threshold, -96.0, 0.0f);
		return LimiterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	static FAtomBusEffectLimiterSettings& SetAttackTime(UPARAM(ref) FAtomBusEffectLimiterSettings& LimiterSettings, float AttackTime)
	{
		LimiterSettings.AttackTime = FMath::Clamp(AttackTime, 0.0f, 3000.0f);
		return LimiterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	static FAtomBusEffectLimiterSettings& SetReleaseTime(UPARAM(ref) FAtomBusEffectLimiterSettings& LimiterSettings, float ReleaseTime)
	{
		LimiterSettings.ReleaseTime = FMath::Clamp(ReleaseTime, 0.0f, 50000.0f);
		return LimiterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	static FAtomBusEffectLimiterSettings& SetGain(UPARAM(ref) FAtomBusEffectLimiterSettings& LimiterSettings, float Gain)
	{
		LimiterSettings.Gain = FMath::Clamp(Gain, -96.0, 48.0f);
		return LimiterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	static FAtomBusEffectLimiterSettings& SetSurroundLink(UPARAM(ref) FAtomBusEffectLimiterSettings& LimiterSettings, float SurroundLink)
	{
		LimiterSettings.SurroundLink = FMath::Clamp(SurroundLink, 0.0, 100.0f);
		return LimiterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	static FAtomBusEffectLimiterSettings& SetType(UPARAM(ref) FAtomBusEffectLimiterSettings& LimiterSettings, EAtomLimiterType Type)
	{
		LimiterSettings.Type = Type;
		return LimiterSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	static FAtomBusEffectLimiterSettings& SetBypass(UPARAM(ref) FAtomBusEffectLimiterSettings& LimiterSettings, bool bBypass)
	{
		LimiterSettings.bBypass = bBypass;
		return LimiterSettings;
	}
};

/*
 * UAtomBusEffectLimiterPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectLimiterPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectLimiterPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectLimiter)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectLimiter)
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

	// Sets runtime limiter settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Limiter", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectLimiterSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	void SetDefaultSettings(const FAtomBusEffectLimiterSettings& InSettings);

	// Set the limiter threshold, in decibels [-96, 0].
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	void SetThreshold(float Gain);

	// Set the limiter attack time, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	void SetAttackTime(float Time);

	// Set the limiter release time, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	void SetReleaseTime(float Time);

	// Set the output gain, in decibels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	void SetGain(float Gain);

	// Set the strength of the surround link.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	void SetSurroundLink(float Percent);

	// Set the amplitude detection value type.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Limiter")
	void SetType(EAtomLimiterType Type);

public:

	/** The Delay preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectLimiterSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectLimiterSettings DynamicSettings;
};
