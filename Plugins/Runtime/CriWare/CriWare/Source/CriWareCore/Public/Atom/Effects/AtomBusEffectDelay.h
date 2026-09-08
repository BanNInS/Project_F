/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectDelay.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectDelay.generated.h"

 /*
  * FAtomBusEffectDelaySettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectDelaySettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectDelaySettings();

	//~ Begin effect parameters

	/** Specifies the delay time in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delay", meta = (ClampMin = "1.0", ClampMax = "1000.0", UIMin = "1.0", UIMax = "1000.0", ForceUnits = ms))
	float DelayTime;

	//~ End effect parameters

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "1.0", ClampMax = "1000.0", UIMin = "1.0", UIMax = "1000.0", ForceUnits = ms))
	float MaximumDelayTime;

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectDelayStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectDelayStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Delay")
	static FAtomBusEffectDelaySettings& SetDelayTime(UPARAM(ref) FAtomBusEffectDelaySettings& DelaySettings, float DelayTime)
	{
		DelaySettings.DelayTime = FMath::Clamp(DelayTime, 1.0f, 1000.0f);
		//DelaySettings.MaximumRoomSize = FMath::Max(DelayTime, DelaySettings.MaximumDelayTime);
		return DelaySettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Delay")
	static FAtomBusEffectDelaySettings& SetMaximumDelayTime(UPARAM(ref) FAtomBusEffectDelaySettings& DelaySettings, float MaximumDelayTime)
	{
		DelaySettings.MaximumDelayTime = FMath::Clamp(MaximumDelayTime, 1.0f, 1000.0f);
		DelaySettings.DelayTime = FMath::Max(DelaySettings.MaximumDelayTime, DelaySettings.DelayTime);
		return DelaySettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Delay")
	static FAtomBusEffectDelaySettings& SetBypass(UPARAM(ref) FAtomBusEffectDelaySettings& DelaySettings, bool bBypass)
	{
		DelaySettings.bBypass = bBypass;
		return DelaySettings;
	}
};

/*
 * UAtomBusEffectDelayPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectDelayPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectDelayPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectDelay)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectDelay)
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

	// Sets runtime Delay settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Delay", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectDelaySettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Delay")
	void SetDefaultSettings(const FAtomBusEffectDelaySettings& InSettings);

	// Set the Delay time, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Delay")
	void SetDelayTime(float Time);

	// Get the maximum delay time possible.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Delay")
	float GetMaxDelayTime() const
	{
		return DynamicSettings.MaximumDelayTime;
	};

public:

	/** The Delay preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectDelaySettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectDelaySettings DynamicSettings;
};
