/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectBinaural.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectBinaural.generated.h"

 /*
  * FAtomBusEffectBinauralSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectBinauralSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectBinauralSettings();

	//~ Begin effect parameters

	/** Specifies the output gain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binaural", meta = (ClampMin = "-12.0", ClampMax = "12.0", UIMin = "-12.0", UIMax = "12.0"))
	float Gain;

	/** Specifies the volume balance between front and rear channels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Binaural", meta = (ClampMin = "-12.0", ClampMax = "12.0", UIMin = "-12.0", UIMax = "12.0"))
	float Balance;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectBinauralStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectBinauralStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Binaural")
	static FAtomBusEffectBinauralSettings& SetGain(UPARAM(ref) FAtomBusEffectBinauralSettings& BinauralSettings, float Gain)
	{
		BinauralSettings.Gain = FMath::Clamp(Gain, -12.0f, 12.0f);
		return BinauralSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Binaural")
	static FAtomBusEffectBinauralSettings& SetBalance(UPARAM(ref) FAtomBusEffectBinauralSettings& BinauralSettings, float Balance)
	{
		BinauralSettings.Balance = FMath::Clamp(Balance, -12.0f, 12.0f);
		return BinauralSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Binaural")
	static FAtomBusEffectBinauralSettings& SetBypass(UPARAM(ref) FAtomBusEffectBinauralSettings& BinauralSettings, bool bBypass)
	{
		BinauralSettings.bBypass = bBypass;
		return BinauralSettings;
	}
};

/*
 * UAtomBusEffectBinauralPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectBinauralPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectBinauralPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectBinaural)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectBinaural)
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

	// Sets runtime Binaural settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Binaural", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectBinauralSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Binaural")
	void SetDefaultSettings(const FAtomBusEffectBinauralSettings& InSettings);

	// Set output gain.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Binaural")
	void SetGain(float Gain);

	// Set volume balance between front and rear channels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Binaural")
	void SetBalance(float Gain);

public:

	/** The Binaural preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectBinauralSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectBinauralSettings DynamicSettings;
};
