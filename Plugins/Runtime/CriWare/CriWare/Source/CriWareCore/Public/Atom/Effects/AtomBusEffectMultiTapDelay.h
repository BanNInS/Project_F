/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectMultiTapDelay.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectMultiTapDelay.generated.h"

/*
 * FAtomTapDelay Structure
 *****************************************************************************/

USTRUCT(BlueprintType)
struct FAtomTapDelay
{
	GENERATED_BODY()

public:

	/** Specifies the tap delay time in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tap Delay", meta = (ClampMin = "1.0", ClampMax = "10000.0", UIMin = "1.0", UIMax = "10000.0", ForceUnits = ms))
	float DelayTime = 1.0f;

	/** Specifies the level of this tap delay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tap Delay", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Level = 0.0f;

	/** Specifies the horizontal 3d panning angle of this tap delay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tap Delay", meta = (ClampMin = "-180.0", ClampMax = "180.0", UIMin = "-180.0", UIMax = "180.0", Units = deg))
	float Azimuth = 0.0f;

	/** Specifies the tap delay ouput feedback level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tap Delay", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Feedback = 0.0f;
};

 /*
  * FAtomBusEffectMultiTapDelaySettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectMultiTapDelaySettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectMultiTapDelaySettings();

protected:

	//~ Begin effect parameters

	/** Specifies the tap delays of the multi-tap delay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multi-Tap Delay", EditFixedSize, meta = (EditFixedOrder))
	TArray<FAtomTapDelay> TapDelays;

	//~ End effect parameters

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "1.0", ClampMax = "10000.0", UIMin = "1.0", UIMax = "10000.0", ForceUnits = ms))
	float MaximumDelayTime;

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;

	// c++ accessors for tap delay settings.
	FORCEINLINE FAtomTapDelay& GetTapDelay(int TapIndex)
	{
		return TapDelays[FMath::Clamp(TapIndex, 0, 3)];
	}

	FORCEINLINE const FAtomTapDelay& GetTapDelay(int TapIndex) const
	{
		return TapDelays[FMath::Clamp(TapIndex, 0, 3)];
	}

	friend class UAtomBusEffectMultiTapDelayPreset;
};

/*
 * UAtomBusEffectMultiTapDelayStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectMultiTapDelayStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
	static FAtomBusEffectMultiTapDelaySettings& SetTapDelayTime(UPARAM(ref) FAtomBusEffectMultiTapDelaySettings& MultiTapDelaySettings, int TapIndex, float DelayTime)
	{
		MultiTapDelaySettings.GetTapDelay(TapIndex).DelayTime = FMath::Clamp(DelayTime, 1.0f, 1000.0f);;
		//MultiTapDelaySettings.MaximumRoomSize = FMath::Max(DelayTime, MultiTapDelaySettings.MaximumDelayTime);
		return MultiTapDelaySettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
	static FAtomBusEffectMultiTapDelaySettings& SetTapLevel(UPARAM(ref) FAtomBusEffectMultiTapDelaySettings& MultiTapDelaySettings, int TapIndex, float Level)
	{
		MultiTapDelaySettings.GetTapDelay(TapIndex).Level = FMath::Clamp(Level, 0.0f, 1.0f);
		return MultiTapDelaySettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
		static FAtomBusEffectMultiTapDelaySettings& SetTapAzimuth(UPARAM(ref) FAtomBusEffectMultiTapDelaySettings& MultiTapDelaySettings, int TapIndex, float Azimuth)
	{
		MultiTapDelaySettings.GetTapDelay(TapIndex).Azimuth = FMath::Clamp(Azimuth, 0.1f, 10.0f);
		return MultiTapDelaySettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
	static FAtomBusEffectMultiTapDelaySettings& SetTapFeedback(UPARAM(ref) FAtomBusEffectMultiTapDelaySettings& MultiTapDelaySettings, int TapIndex, float Feedback)
	{
		MultiTapDelaySettings.GetTapDelay(TapIndex).Feedback = FMath::Clamp(Feedback, 0.0f, 1.0f);
		return MultiTapDelaySettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Delay")
	static FAtomBusEffectMultiTapDelaySettings& SetMaximumDelayTime(UPARAM(ref) FAtomBusEffectMultiTapDelaySettings& MultiTapDelaySettings, float MaximumDelayTime)
	{
		MultiTapDelaySettings.MaximumDelayTime = FMath::Clamp(MaximumDelayTime, 1.0f, 1000.0f);
		for (int TapIndex = 0; TapIndex < 4; TapIndex++)
		{
			auto& TapDelay = MultiTapDelaySettings.GetTapDelay(TapIndex);
			TapDelay.DelayTime = FMath::Max(MultiTapDelaySettings.MaximumDelayTime, TapDelay.DelayTime);
		}
		return MultiTapDelaySettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
	static FAtomBusEffectMultiTapDelaySettings& SetBypass(UPARAM(ref) FAtomBusEffectMultiTapDelaySettings& MultiTapDelaySettings, bool bBypass)
	{
		MultiTapDelaySettings.bBypass = bBypass;
		return MultiTapDelaySettings;
	}
};

/*
 * UAtomBusEffectMultiTapDelayPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectMultiTapDelayPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectMultiTapDelayPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectMultiTapDelay)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectMultiTapDelay)
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

	// Sets runtime multi-tap delay settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Multi-tap Delay", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectMultiTapDelaySettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
	void SetDefaultSettings(const FAtomBusEffectMultiTapDelaySettings& InSettings);

	// Set the delay time of the tap in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
	void SetTapDelayTime(int TapIndex, float Time);

	// Set the tap level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
	void SetTapLevel(int TapIndex, float Level);

	// Set the horizontal 3d panning angle in degree [-180, 180] of the tap.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
	void SetTapAzimuth(int TapIndex, float Angle);

	// Set the output feedback gain of the tap.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
	void SetTapFeedback(int TapIndex, float Level);

	// Get the maximum delay time possible.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-tap Delay")
	float GetMaxDelayTime() const
	{
		return DynamicSettings.MaximumDelayTime;
	};

public:

	/** The MultiBandEQ preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectMultiTapDelaySettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectMultiTapDelaySettings DynamicSettings;
};

