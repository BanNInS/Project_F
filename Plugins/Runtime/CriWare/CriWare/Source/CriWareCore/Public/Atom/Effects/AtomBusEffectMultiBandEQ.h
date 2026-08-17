/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectMultiBandEQ.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CriWareDefines.h"
#include "Atom/AtomBusEffectPreset.h"

#include "AtomBusEffectMultiBandEQ.generated.h"

UENUM(BlueprintType)
enum class EAtomBandEQFilterType : uint8
{
	LowShelf,
	HighShelf,
	Peaking
};

/*
 * FAtomBandEQFilter Structure
 *****************************************************************************/

USTRUCT(BlueprintType)
struct FAtomBandEQFilter
{
	GENERATED_BODY()

public:

	/** Specifies the filter type to use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EQ Filter")
	EAtomBandEQFilterType FilterType = EAtomBandEQFilterType::LowShelf;

	/** Specifies the frequency of the filter in hertz. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EQ Filter", meta = (ClampMin = "24.0", ClampMax = "24000.0", UIMin = "24.0", UIMax = "24000.0", Units = Hz, SliderExponent = "10.0"))
	float Frequency = ATOM_MAX_FILTER_FREQUENCY;

	/** Specifies the Q factor (resonance) of the filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EQ Filter", meta = (ClampMin = "0.1", ClampMax = "10.0", UIMin = "0.1", UIMax = "10.0", SliderExponent = "5.0"))
	float Q = 1.0f;

	/** Specifies the filter output gain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EQ Filter", meta = (ClampMin = "0.0", ClampMax = "5.0", UIMin = "0.0", UIMax = "5.0"))
	float Gain = 0.0f;
};

 /*
  * FAtomBusEffectMultiBandEQSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectMultiBandEQSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectMultiBandEQSettings();

protected:

	//~ Begin effect parameters

	/** Specifies the EQ filters (biquad filters) of the multi-band equalizer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Multi-band EQ", EditFixedSize, meta = (EditFixedOrder))
	TArray<FAtomBandEQFilter> BandFilters;

	//~ End effect parameters

public:

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;

	// c++ accessors for band filters.
	FORCEINLINE FAtomBandEQFilter& GetBandFilter(int BandIndex)
	{
		return BandFilters[FMath::Clamp(BandIndex, 0, BandFilters.Num() - 1)];
	}

	FORCEINLINE const FAtomBandEQFilter& GetBandFilter(int BandIndex) const
	{
		return BandFilters[FMath::Clamp(BandIndex, 0, BandFilters.Num() - 1)];
	}

	friend class UAtomBusEffectMultiBandEQPreset;
};

/*
 * UAtomBusEffectMultiBandEQStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectMultiBandEQStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	static FAtomBusEffectMultiBandEQSettings& SetBandFilterType(UPARAM(ref) FAtomBusEffectMultiBandEQSettings& MultiBandEQSettings, int BandIndex, EAtomBandEQFilterType FilterType)
	{
		MultiBandEQSettings.GetBandFilter(BandIndex).FilterType = FilterType;
		return MultiBandEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	static FAtomBusEffectMultiBandEQSettings& SetBandFrequency(UPARAM(ref) FAtomBusEffectMultiBandEQSettings& MultiBandEQSettings, int BandIndex, float Frequency)
	{
		MultiBandEQSettings.GetBandFilter(BandIndex).Frequency = FMath::Clamp(Frequency, 24.0f, 24000.0f);
		return MultiBandEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	static FAtomBusEffectMultiBandEQSettings& SetBandQ(UPARAM(ref) FAtomBusEffectMultiBandEQSettings& MultiBandEQSettings, int BandIndex, float Q)
	{
		MultiBandEQSettings.GetBandFilter(BandIndex).Q = FMath::Clamp(Q, 0.1f, 10.0f);
		return MultiBandEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	static FAtomBusEffectMultiBandEQSettings& SetBandGain(UPARAM(ref) FAtomBusEffectMultiBandEQSettings& MultiBandEQSettings, int BandIndex, float Gain)
	{
		MultiBandEQSettings.GetBandFilter(BandIndex).Gain = FMath::Clamp(Gain, 0.0f, 5.0f);
		return MultiBandEQSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	static FAtomBusEffectMultiBandEQSettings& SetBypass(UPARAM(ref) FAtomBusEffectMultiBandEQSettings& MultiBandEQSettings, bool bBypass)
	{
		MultiBandEQSettings.bBypass = bBypass;
		return MultiBandEQSettings;
	}
};

/*
 * UAtomBusEffectMultiBandEQPreset Class
 *****************************************************************************/

UCLASS(abstract)
class CRIWARECORE_API UAtomBusEffectMultiBandEQPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectMultiBandEQPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectMultiBandEQ)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectMultiBandEQ)
	virtual float GetParameterValue(int ParameterIndex) const override;
	virtual void SetParameterValue(int ParameterIndex, float Value) override;
	//~ End UAtomBusEffectPreset Interface

	// get the number of bands equalizer uses.
	virtual int GetEqualizerNumBands() PURE_VIRTUAL(UAtomBusEffectMultiBandEQPreset::GetEqualizerNumBands, return 0;);

public:

	//~ Begin UObject interface.
	virtual void PostInitProperties() override;
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject interface.

	// Sets runtime Multi-Band EQ settings. This will replace any dynamically added or modified settings without modifying
	// the original UObject.
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Multi-band EQ", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectMultiBandEQSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	void SetDefaultSettings(const FAtomBusEffectMultiBandEQSettings& InSettings);

	// Set the filter type to operate.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	void SetBandFilterType(int BandIndex, EAtomBandEQFilterType Type);

	// Set the filter frequency, in hertz.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	void SetBandFrequency(int BandIndex, float Frequency);

	// Set the resonance of the filter.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	void SetBandQ(int BandIndex, float Factor);

	// Set the output gain of the filter.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Multi-band EQ")
	void SetBandGain(int BandIndex, float Level);

public:

	/** The MultiBandEQ preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectMultiBandEQSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectMultiBandEQSettings DynamicSettings;
};

