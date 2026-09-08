/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectBitCrusher.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectBitCrusher.generated.h"

 /*
  * FAtomBusEffectBitCrusherSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectBitCrusherSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectBitCrusherSettings();

	//~ Begin effect parameters

	/** Specify the drive gain of the bit-crusher. (dB) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bit-crusher", meta = (ClampMin = "0.0", ClampMax = "48.0", UIMin = "0.0", UIMax = "48.0"))
	float Drive;

	/** Specifies the quantization bit number of the bit-crusher. A small value makes amplitude to become Lo-Fi. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bit-crusher", meta = (ClampMin = "1", ClampMax = "24", UIMin = "1", UIMax = "24"))
	int BitDepth;

	/** Specifies the down sampling value of the bit-crusher. As this value is increased, the sampling rate decrease, thus stronger is the Lo-Fi feeling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bit-crusher", meta = (ClampMin = "1", ClampMax = "100", UIMin = "1", UIMax = "100"))
	int DownSampling;

	/** Specifies the mixing ratio of the dry (original sound) of the bit-crusher. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bit-crusher", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DryMix;

	/** Specifies the mixing ratio of the wet (modified sound) of the bit-crusher. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bit-crusher", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float WetMix;

	/** Specifies the bit-crusher output gain. (dB) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bit-crusher", meta = (ClampMin = "-96.0", ClampMax = "48.0", UIMin = "-96.0", UIMax = "48.0", SliderExponent = "0.75"))
	float Gain;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectBitCrusherStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectBitCrusherStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	static FAtomBusEffectBitCrusherSettings& SetDrive(UPARAM(ref) FAtomBusEffectBitCrusherSettings& BitCrusherSettings, float Drive)
	{
		BitCrusherSettings.Drive = FMath::Clamp(Drive, 0.0f, 48.0f);
		return BitCrusherSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	static FAtomBusEffectBitCrusherSettings& SetBitDepth(UPARAM(ref) FAtomBusEffectBitCrusherSettings& BitCrusherSettings, int BitDepth)
	{
		BitCrusherSettings.BitDepth = FMath::Clamp(BitDepth, 1, 24);
		return BitCrusherSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	static FAtomBusEffectBitCrusherSettings& SetDownSampling(UPARAM(ref) FAtomBusEffectBitCrusherSettings& BitCrusherSettings, int DownSampling)
	{
		BitCrusherSettings.DownSampling = FMath::Clamp(DownSampling, 1, 100);
		return BitCrusherSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	static FAtomBusEffectBitCrusherSettings& SetDryMix(UPARAM(ref) FAtomBusEffectBitCrusherSettings& BitCrusherSettings, float DryMix)
	{
		BitCrusherSettings.DryMix = FMath::Clamp(DryMix, 0.0f, 1.0f);
		return BitCrusherSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	static FAtomBusEffectBitCrusherSettings& SetWetMix(UPARAM(ref) FAtomBusEffectBitCrusherSettings& BitCrusherSettings, float WetMix)
	{
		BitCrusherSettings.WetMix = FMath::Clamp(WetMix, 0.0f, 1.0f);
		return BitCrusherSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	static FAtomBusEffectBitCrusherSettings& SetGain(UPARAM(ref) FAtomBusEffectBitCrusherSettings& BitCrusherSettings, float Gain)
	{
		BitCrusherSettings.Gain = FMath::Clamp(Gain, -96.0f, 48.0f);
		return BitCrusherSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	static FAtomBusEffectBitCrusherSettings& SetBypass(UPARAM(ref) FAtomBusEffectBitCrusherSettings& BitCrusherSettings, bool bBypass)
	{
		BitCrusherSettings.bBypass = bBypass;
		return BitCrusherSettings;
	}
};

/*
 * UAtomBusEffectBitCrusherPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectBitCrusherPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectBitCrusherPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectBitCrusher)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectBitCrusher)
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

	// Sets runtime BitCrusher settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|BitCrusher", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectBitCrusherSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	void SetDefaultSettings(const FAtomBusEffectBitCrusherSettings& InSettings);

	// Set the drive gain.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	void SetDrive(float Gain);

	// Set the bit-depth. Possible values are in the range of 1 to 24 bits.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	void SetBitDepth(int BitDepth);

	// Set the down sampling value.	
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	void SetDownSampling(int Number);

	// Set the dry mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	void SetDryMix(float Level);

	// Get the wet mix level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	void SetWetMix(float Level);

	// Set the output gain.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|BitCrusher")
	void SetGain(float Gain);

public:

	/** The BitCrusher preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectBitCrusherSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectBitCrusherSettings DynamicSettings;
};
