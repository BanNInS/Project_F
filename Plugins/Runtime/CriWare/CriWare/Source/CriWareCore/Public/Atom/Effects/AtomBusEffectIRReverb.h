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

#include "AtomBusEffectIRReverb.generated.h"

 /*
  * FAtomBusEffectIRReverbSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectIRReverbSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectIRReverbSettings();

	//~ Begin effect parameters

	/** Specifies the mix gain of the dry component of the IR reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float DryGain;

	/** Specifies the mix gain of the wet component of the IR reverb. 
	 * Applies to all wet components output by the effect, including reverberation that is already reverberating.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float WetGain;

	/** Specifies the gain applied when generating reverberation. 
	 * Unlike wet gain, it applies only to subsequent reverberations.
	 * Does not apply to reverberations that are already echoing. 
	 * You can adjust the thickness of the reverberation for each IR by updating it when switching IRs.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float ReverbGain;

	/** Specifies the left-right separation when input signal is stereo-mixed. (0.0 is monoaural, 1.0 is fully spread between respective left and right channels.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float InputSeparationRatio;

	/** Specifies the input gain of the front (L, R channels) component of the IR reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float FrontInputGain;

	/** Specifies the intput gain of the center (C channel) component of the IR reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CenterInputGain;

	/** Specifies the input gain of the surround (surround L, surround R channels) component of the IR reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SurroundInputGain;

	/** Specifies the input gain of the surround Back (surround back L, surround back R channels) component of the IR reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SurroundBackInputGain;

	/** Specifies the output gain of the front (L, R channels) component of the IR reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float FrontOutputGain;

	/** Specifies the output gain of the center (C channel) component of the IR reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float CenterOutputGain;

	/** Specifies the output gain of the surround (surround L, surround R channels) component of the IR reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SurroundOutputGain;

	/** Specifies the output gain of the surround Back (surround back L, surround back R channels) component of the IR reverb. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IRReverb", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SurroundBackOutputGain;

	//~ End effect parameters

	/** Specifies the index number of IR used for convolution from the saved list of IRs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int IRIndex;

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectIRReverbStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectIRReverbStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetDryGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float DryGain)
	{
		ReverbSettings.DryGain = FMath::Clamp(DryGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetWetGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float WetGain)
	{
		ReverbSettings.WetGain = FMath::Clamp(WetGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetReverbGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float ReverbGain)
	{
		ReverbSettings.ReverbGain = FMath::Clamp(ReverbGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetInputSeparationRatio(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float InputSeparationRatio)
	{
		ReverbSettings.InputSeparationRatio = FMath::Clamp(InputSeparationRatio, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetFrontInputGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float FrontInputGain)
	{
		ReverbSettings.FrontInputGain = FMath::Clamp(FrontInputGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetCenterInputGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float CenterInputGain)
	{
		ReverbSettings.CenterInputGain = FMath::Clamp(CenterInputGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetSurroundInputGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float SurroundInputGain)
	{
		ReverbSettings.SurroundInputGain = FMath::Clamp(SurroundInputGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetSurroundBackInputGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float SurroundBackInputGain)
	{
		ReverbSettings.SurroundBackInputGain = FMath::Clamp(SurroundBackInputGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetFrontOutputGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float FrontOutputGain)
	{
		ReverbSettings.FrontOutputGain = FMath::Clamp(FrontOutputGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetCenterOutputGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float CenterOutputGain)
	{
		ReverbSettings.CenterOutputGain = FMath::Clamp(CenterOutputGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetSurroundOutputGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float SurroundOutputGain)
	{
		ReverbSettings.SurroundOutputGain = FMath::Clamp(SurroundOutputGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetSurroundBackOutputGain(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, float SurroundBackOutputGain)
	{
		ReverbSettings.SurroundBackOutputGain = FMath::Clamp(SurroundBackOutputGain, 0.0f, 1.0f);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetIRIndex(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, int IRIndex)
	{
		ReverbSettings.IRIndex = FMath::Max(IRIndex, 0);
		return ReverbSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	static FAtomBusEffectIRReverbSettings& SetBypass(UPARAM(ref) FAtomBusEffectIRReverbSettings& ReverbSettings, bool bBypass)
	{
		ReverbSettings.bBypass = bBypass;
		return ReverbSettings;
	}
};

/*
 * UAtomBusEffectIRReverbPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectIRReverbPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectIRReverbPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectIRReverb)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectIRReverb)
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
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|IRReverb", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectIRReverbSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetDefaultSettings(const FAtomBusEffectIRReverbSettings& InSettings);

	// Set the mix gain of the dry component.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetDryGain(float Gain);

	// Set the mix gain of the wet component.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetWetGain(float Gain);

	// Set the gain applied when generating reverberation.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetReverbGain(float Gain);

	// Set the left-right separation when input signal is stereo-mixed.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetInputSeparationRatio(float Ratio);

	// Set the input gain of the front channels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetFrontInputGain(float Gain);

	// Set the input gain of the center channel.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetCenterInputGain(float Gain);

	// Set the input gain of the surround channels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetSurroundInputGain(float Gain);

	// Set the input gain of the surround back channels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetSurroundBackInputGain(float Gain);

	// Set the output gain of the front channels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetFrontOutputGain(float Gain);

	// Set the output gain of the center channel.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetCenterOutputGain(float Gain);

	// Set the output gain of the surround channels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetSurroundOutputGain(float Gain);

	// Set the output gain of the surround back channels.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	void SetSurroundBackOutputGain(float Gain);

	// Get the maximum pre delay time possible.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|IRReverb")
	int GetIRIndex() const
	{
		return DynamicSettings.IRIndex;
	};

public:

	/** The reverb preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectIRReverbSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectIRReverbSettings DynamicSettings;
};
