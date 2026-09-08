/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectPhaser.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectPhaser.generated.h"

 /*
  * FAtomBusEffectPhaserSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectPhaserSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectPhaserSettings();

	//~ Begin effect parameters

	/** Specify the strength of the phaser effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phaser", meta = (ClampMin = "4", ClampMax = "12", UIMin = "4", UIMax = "12", Multiple = 2, Delta = 2))
	int Stages;

	/** Specifies the depth of the phaser LFO (amplitude). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phaser", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Depth;

	/** Specifies the rate of the phaser LFO in hertz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phaser", meta = (ClampMin = "0.01", ClampMax = "100.0", UIMin = "0.01", UIMax = "100.0", Units = Hz, SliderExponent = "10.0"))
	float Rate;

	/** Specifies the phaser feedack gain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phaser", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Feedback;

	/** Specifies the mixing ratio of the dry (original sound) of the phaser. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phaser", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DryMix;

	/** Specifies the mixing ratio of the wet (modified sound) of the phaser. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Phaser", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float WetMix;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectPhaserStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectPhaserStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	static FAtomBusEffectPhaserSettings& SetDelayTime(UPARAM(ref) FAtomBusEffectPhaserSettings& PhaserSettings, int Stages)
	{
		Stages = FMath::Clamp(Stages, 4, 12);
		PhaserSettings.Stages = (Stages % 2) > 0 ? Stages + 1 : Stages;
		return PhaserSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	static FAtomBusEffectPhaserSettings& SetDepth(UPARAM(ref) FAtomBusEffectPhaserSettings& PhaserSettings, float Depth)
	{
		PhaserSettings.Depth = FMath::Clamp(Depth, 0.0f, 1.0f);
		return PhaserSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	static FAtomBusEffectPhaserSettings& SetRate(UPARAM(ref) FAtomBusEffectPhaserSettings& PhaserSettings, float Rate)
	{
		PhaserSettings.Rate = FMath::Clamp(Rate, 0.01f, 100.0f);
		return PhaserSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	static FAtomBusEffectPhaserSettings& SetFeedback(UPARAM(ref) FAtomBusEffectPhaserSettings& PhaserSettings, float Feedback)
	{
		PhaserSettings.Feedback = FMath::Clamp(Feedback, 0.0f, 1.0f);
		return PhaserSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	static FAtomBusEffectPhaserSettings& SetDryMix(UPARAM(ref) FAtomBusEffectPhaserSettings& PhaserSettings, float DryMix)
	{
		PhaserSettings.DryMix = FMath::Clamp(DryMix, 0.0f, 1.0f);
		return PhaserSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	static FAtomBusEffectPhaserSettings& SetWetMix(UPARAM(ref) FAtomBusEffectPhaserSettings& PhaserSettings, float WetMix)
	{
		PhaserSettings.WetMix = FMath::Clamp(WetMix, 0.0f, 1.0f);
		return PhaserSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	static FAtomBusEffectPhaserSettings& SetBypass(UPARAM(ref) FAtomBusEffectPhaserSettings& PhaserSettings, bool bBypass)
	{
		PhaserSettings.bBypass = bBypass;
		return PhaserSettings;
	}
};

/*
 * UAtomBusEffectPhaserPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectPhaserPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectPhaserPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectPhaser)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectPhaser)
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

	// Sets runtime Phaser settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Phaser", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectPhaserSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	void SetDefaultSettings(const FAtomBusEffectPhaserSettings& InSettings);

	// Set the number of stages. The possible values are 4, 6, 8, 10 or 12.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	void SetStages(int Number);

	// Set depth, amplitude of LFO.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	void SetDepth(float Level);

	// Set the LFO rate in hertz.	
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	void SetRate(float Rate);

	// Set the feedback level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	void SetFeedback(float Level);

	// Set the dry mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	void SetDryMix(float Level);

	// Get the wet mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Phaser")
	void SetWetMix(float Level);

public:

	/** The Phaser preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectPhaserSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectPhaserSettings DynamicSettings;
};
