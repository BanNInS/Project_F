#pragma once

/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectMatrix.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Atom/AtomBusEffectPreset.h"
#include "Atom/AtomBus.h"

#include "AtomBusEffectMatrix.generated.h"

 /*
  * FAtomBusEffectMatrixSettings Structure
  *****************************************************************************/

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomBusEffectMatrixSettings
{
	GENERATED_BODY()

public:

	FAtomBusEffectMatrixSettings();

	//~ Begin effect parameters

	/** Specifies the level matrix to apply to each channels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Matrix", meta = (ShowOnlyInnerProperties))
	FAtomChannelLevelMatrix LevelMatrix;

	//~ End effect parameters

	/** If true, input audio is directly routed to output audio with applying any effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BusEffectPreset)
	bool bBypass;
};

/*
 * UAtomBusEffectMatrixStatics Blueprint Library
 * Helper for blueprint.
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectMatrixStatics
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Matrix")
	static FAtomBusEffectMatrixSettings& SetLevelMatrix(UPARAM(ref) FAtomBusEffectMatrixSettings& MatrixSettings, const FAtomChannelLevelMatrix& LevelMatrix)
	{
		MatrixSettings.LevelMatrix = LevelMatrix;
		return MatrixSettings;
	}

	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Matrix")
	static FAtomBusEffectMatrixSettings& SetBypass(UPARAM(ref) FAtomBusEffectMatrixSettings& MatrixSettings, bool bBypass)
	{
		MatrixSettings.bBypass = bBypass;
		return MatrixSettings;
	}
};

/*
 * UAtomBusEffectMatrixPreset Class
 *****************************************************************************/

UCLASS()
class CRIWARECORE_API UAtomBusEffectMatrixPreset
	: public UAtomBusEffectPreset
{
	GENERATED_BODY()

public:

	UAtomBusEffectMatrixPreset(const FObjectInitializer& ObjectInitializer);

	ATOM_EFFECT_PRESET_METHODS(AtomBusEffectMatrix)

protected:

	//~ Begin UAtomBusEffectPreset Interface
	ATOM_BUS_EFFECT_PRESET_METHODS(AtomBusEffectMatrix)
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
	UFUNCTION(BlueprintCallable, BlueprintSetter, Category = "Atom|Effects|Matrix", meta = (DisplayName = "Set Dynamic Settings"))
	void SetSettings(const FAtomBusEffectMatrixSettings& InSettings);

	// Sets object's default settings. This will update both the default UObject settings (and mark it as dirty),
	// as well as any dynamically set settings.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Matrix")
	void SetDefaultSettings(const FAtomBusEffectMatrixSettings& InSettings);

	// Set the channel level matrix.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Matrix")
	void SetLevelMatrix(const FAtomChannelLevelMatrix& LevelMatrix);

	// Set only one channel level.
	UFUNCTION(BlueprintCallable, Category = "Atom|Effects|Matrix")
	void SetChannelLevel(EAtomSpeaker InChannel, EAtomSpeaker OutChannel, float Level);

public:

	/** The reverb preset Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetDefaultSettings, Category = "BusEffectPreset", Meta = (ShowOnlyInnerProperties))
	FAtomBusEffectMatrixSettings Settings;

	UPROPERTY(Transient)
	FAtomBusEffectMatrixSettings DynamicSettings;
};
