/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectBitCrusher.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectBitCrusher.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectBitCrusher"

 /*
  * FAtomBusEffectBitCrusherSettings Implementation
  *****************************************************************************/

FAtomBusEffectBitCrusherSettings::FAtomBusEffectBitCrusherSettings()
	: Drive(3.0f)
	, BitDepth(8)
	, DownSampling(1)
	, DryMix(0.71f)
	, WetMix(0.71f)
	, Gain(0.0f)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectBitCrusherPreset Implementation
 *****************************************************************************/

UAtomBusEffectBitCrusherPreset::UAtomBusEffectBitCrusherPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_BIT_CRUSHER_NAME);
	NumParameters = CRIATOMEXASR_BIT_CRUSHER_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectBitCrusherPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DRIVE_DB: return DynamicSettings.Drive;
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DEPTH_BITS: return (float)DynamicSettings.BitDepth;
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DOWN_SAMPLING: return (float)DynamicSettings.DownSampling;
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DRY_MIX: return DynamicSettings.DryMix;
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_WET_MIX: return DynamicSettings.WetMix;
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_OUTPUT_GAIN: return Atom::ConvertToLinear(DynamicSettings.Gain); // dB to presure
	}

	return 0.0f;
}

void UAtomBusEffectBitCrusherPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DRIVE_DB: DynamicSettings.Drive = Value; break;
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DEPTH_BITS: DynamicSettings.BitDepth = (float)Value; break;
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DOWN_SAMPLING: DynamicSettings.DownSampling = (float)Value; break;
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DRY_MIX: DynamicSettings.DryMix = Value; break;
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_WET_MIX: DynamicSettings.WetMix = Value; break;
	case CRIATOMEXASR_BIT_CRUSHER_PARAMETER_OUTPUT_GAIN: DynamicSettings.Gain = Atom::ConvertToDecibels(Value); break; // Presure to dB
	}
}

#if WITH_EDITOR
void UAtomBusEffectBitCrusherPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectBitCrusherPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectBitCrusherPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectBitCrusherPreset::SetSettings(const FAtomBusEffectBitCrusherSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectBitCrusherPreset::SetDefaultSettings(const FAtomBusEffectBitCrusherSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectBitCrusherPreset::SetDrive(float Gain)
{
	DynamicSettings.Drive = Gain;

	UpdateParameter(CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DRIVE_DB);
}

void UAtomBusEffectBitCrusherPreset::SetBitDepth(int BitDepth)
{
	DynamicSettings.BitDepth = BitDepth;

	UpdateParameter(CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DEPTH_BITS);
}

void UAtomBusEffectBitCrusherPreset::SetDownSampling(int Number)
{
	DynamicSettings.DownSampling = Number;

	UpdateParameter(CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DOWN_SAMPLING);
}

void UAtomBusEffectBitCrusherPreset::SetDryMix(float Level)
{
	DynamicSettings.DryMix = Level;

	UpdateParameter(CRIATOMEXASR_BIT_CRUSHER_PARAMETER_DRY_MIX);
}

void UAtomBusEffectBitCrusherPreset::SetWetMix(float Level)
{
	DynamicSettings.WetMix = Level;

	UpdateParameter(CRIATOMEXASR_BIT_CRUSHER_PARAMETER_WET_MIX);
}

void UAtomBusEffectBitCrusherPreset::SetGain(float Gain)
{
	DynamicSettings.Gain = Gain;

	UpdateParameter(CRIATOMEXASR_BIT_CRUSHER_PARAMETER_OUTPUT_GAIN);
}

#undef LOCTEXT_NAMESPACE
