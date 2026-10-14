/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectReverb.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectReverb.generated.h"

 /*
  * FAtomBusEffectReverbSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectReverbSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectReverbSettings();

	//~ Begin effect parameters

	/** Specifies the reverb time in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "1.0", ClampMax = "20000.0", UIMin = "1.0", UIMax = "20000.0", ForceUnits = ms))
	float ReverbTime;

	/** Specifies the room size of the reverb in meters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "1.0", ClampMax = "100.0", UIMin = "1.0", UIMax = "100.0", Units = m))
	float RoomSize;

	/** Specifies the reverb pre-delay time in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "10.0", ClampMax = "1000.0", UIMin = "10.0", UIMax = "1000.0", ForceUnits = ms))
	float PreDelayTime;

	/** Specifies the low cutoff frequency of the reverb output in hertz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "0.0", ClampMax = "24000.0", UIMin = "0.0", UIMax = "24000.0", Units = Hz, SliderExponent = "10.0"))
	float LowCutoff;

	/** Specifies the high cutoff frequency of the reverb output in hertz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverb", meta = (ClampMin = "8000.0", ClampMax = "24000.0", UIMin = "8000.0", UIMax = "24000.0", Units = Hz, SliderExponent = "10.0"))
	float HighCutoff;

	//~ End effect parameters

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "1.0", ClampMax = "100.0", UIMin = "1.0", UIMax = "100.0", Units = m))
	float MaximumRoomSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "10.0", ClampMax = "1000.0", UIMin = "10.0", UIMax = "1000.0", ForceUnits = ms))
	float MaximumPreDelayTime;

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectReverbStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectReverbStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	static FAtomBusEffectReverbSettings& SetReverbTime(UPARAM(ref) FAtomBusEffectReverbSettings& ReverbSettings, float ReverbTime)
	{
		ReverbSettings.ReverbTime = FMath::Clamp(ReverbTime, 1.0f, 20000.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	static FAtomBusEffectReverbSettings& SetRoomSize(UPARAM(ref) FAtomBusEffectReverbSettings& ReverbSettings, float RoomSize)
	{
		ReverbSettings.RoomSize = FMath::Clamp(RoomSize, 1.0f, 50.0f);
		//ReverbSettings.MaximumRoomSize = FMath::Max(ReverbSettings.MaximumRoomSize, ReverbSettings.RoomSize);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	static FAtomBusEffectReverbSettings& SetPreDelayTime(UPARAM(ref) FAtomBusEffectReverbSettings& ReverbSettings, float PreDelayTime)
	{
		ReverbSettings.PreDelayTime = FMath::Clamp(PreDelayTime, 10.0f, 1000.0f);
		//ReverbSettings.MaximumPreDelayTime = FMath::Max(ReverbSettings.MaximumPreDelayTime, ReverbSettings.PreDelayTime);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	static FAtomBusEffectReverbSettings& SetLowCutoff(UPARAM(ref) FAtomBusEffectReverbSettings& ReverbSettings, float LowCutoff)
	{
		ReverbSettings.LowCutoff = FMath::Clamp(LowCutoff, 8000.0f, 24000.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	static FAtomBusEffectReverbSettings& SetHighCutoff(UPARAM(ref) FAtomBusEffectReverbSettings& ReverbSettings, float HighCutoff)
	{
		ReverbSettings.HighCutoff = FMath::Clamp(HighCutoff, 1.0f, 24000.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	static FAtomBusEffectReverbSettings& SetMaximumRoomSize(UPARAM(ref) FAtomBusEffectReverbSettings& ReverbSettings, float MaximumRoomSize)
	{
		ReverbSettings.MaximumRoomSize = FMath::Max(1.0f, MaximumRoomSize);
		ReverbSettings.RoomSize = FMath::Min(ReverbSettings.MaximumRoomSize, ReverbSettings.RoomSize);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	static FAtomBusEffectReverbSettings& SetMaximumPreDelayTime(UPARAM(ref) FAtomBusEffectReverbSettings& ReverbSettings, float MaximumPreDelayTime)
	{
		ReverbSettings.MaximumPreDelayTime = FMath::Max(10.0f, MaximumPreDelayTime);
		ReverbSettings.PreDelayTime = FMath::Min(ReverbSettings.MaximumPreDelayTime, ReverbSettings.PreDelayTime);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	static FAtomBusEffectReverbSettings& SetBypass(UPARAM(ref) FAtomBusEffectReverbSettings& ReverbSettings, bool bBypass)
	{
		ReverbSettings.bBypass = bBypass;
		return ReverbSettings;
	}
};

/*
 * UAtomBusEffectReverbPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectReverbPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectReverbPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectReverb)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectReverb)
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
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Reverb", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectReverbSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	void SetDefaultSettings(const FAtomBusEffectReverbSettings& InSettings);

	// Set the reverb time, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	void SetReverbTime(float Time);

	// Set the room size, in meters.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	void SetRoomSize(float Meters);

	// Set the pre delay time, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	void SetPreDelayTime(float Time);

	// Set the low cutoff frequency, in hertz.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	void SetLowCutoff(float Frenquency);

	// Set the high cutoff frequency, in hertz.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	void SetHighCutoff(float Frenquency);

	// Get the maximum room size possible.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	float GetMaxRoomSizeInMeters() const
	{
		return DynamicSettings.MaximumRoomSize;
	};

	// Get the maximum pre delay time possible.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Reverb")
	float GetMaxPreDelayInMilliseconds() const
	{
		return DynamicSettings.MaximumPreDelayTime;
	};

public:

	/** The reverb preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectReverbSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectReverbSettings DynamicSettings;
};
