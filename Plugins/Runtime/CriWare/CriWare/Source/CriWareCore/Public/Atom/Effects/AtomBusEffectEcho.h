/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectEcho.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectEcho.generated.h"

 /*
  * FAtomBusEffectEchoSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectEchoSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectEchoSettings();

	//~ Begin effect parameters

	/** Specifies the delay time of the echo in milliseconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo", meta = (ClampMin = "0.1", ClampMax = "1000.0", UIMin = "0.1", UIMax = "1000.0", ForceUnits = ms))
	float DelayTime;

	/** Specifies the feedback gain of the echo. Please note that the echo sound due to large feedback gain will not stop even if you stop playback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo", meta = (ClampMin = "-0.99", ClampMax = "0.99", UIMin = "-0.99", UIMax = "0.99"))
	float Feedback;

	//~ End effect parameters

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (ClampMin = "0.1", ClampMax = "1000.0", UIMin = "0.1", UIMax = "1000.0", ForceUnits = ms))
	float MaximumDelayTime;

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectEchoStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectEchoStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Echo")
	static FAtomBusEffectEchoSettings& SetDelayTime(UPARAM(ref) FAtomBusEffectEchoSettings& EchoSettings, float DelayTime)
	{
		EchoSettings.DelayTime = FMath::Clamp(DelayTime, 0.1f, 1000.0f);
		//EchoSettings.MaximumDelayTime = FMath::Max(DelayTime, EchoSettings.MaximumDelayTime);
		return EchoSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Echo")
	static FAtomBusEffectEchoSettings& SetFeedback(UPARAM(ref) FAtomBusEffectEchoSettings& EchoSettings, float Feedback)
	{
		EchoSettings.Feedback = FMath::Clamp(Feedback, -0.99f, 0.99f);
		return EchoSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Echo")
	static FAtomBusEffectEchoSettings& SetMaximumDelayTime(UPARAM(ref) FAtomBusEffectEchoSettings& EchoSettings, float MaximumDelayTime)
	{
		EchoSettings.MaximumDelayTime = FMath::Clamp(MaximumDelayTime, 0.1f, 1000.0f);
		EchoSettings.DelayTime = FMath::Max(EchoSettings.MaximumDelayTime, EchoSettings.DelayTime);
		return EchoSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Echo")
	static FAtomBusEffectEchoSettings& SetBypass(UPARAM(ref) FAtomBusEffectEchoSettings& EchoSettings, bool bBypass)
	{
		EchoSettings.bBypass = bBypass;
		return EchoSettings;
	}
};

/*
 * UAtomBusEffectEchoPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectEchoPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectEchoPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectEcho)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectEcho)
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

	// Sets runtime Echo settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Echo", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectEchoSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Echo")
	void SetDefaultSettings(const FAtomBusEffectEchoSettings& InSettings);

	// Set the delay time, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Echo")
	void SetDelayTime(float Time);

	// Set the feedback gain. (Internaly truncated to [-0.99f, 0.99f] to avoid infinite echo.)
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Echo")
	void SetFeedback(float Level);

	// Get the maximum delay time possible, in milliseconds.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Echo")
	float GetMaxDelayTime() const
	{
		return DynamicSettings.MaximumDelayTime;
	};

public:

	/** The Echo preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectEchoSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectEchoSettings DynamicSettings;
};
