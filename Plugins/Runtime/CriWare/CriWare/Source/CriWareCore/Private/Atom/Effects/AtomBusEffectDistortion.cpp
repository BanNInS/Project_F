/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectDistortion.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectDistortion.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectDistortion"

 /*
  * FAtomBusEffectDistortionSettings Implementation
  *****************************************************************************/

FAtomBusEffectDistortionSettings::FAtomBusEffectDistortionSettings()
	: Drive(0.5f)
	, DryMix(0.71f)
	, WetMix(0.71f)
	, Gain(6.0f)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectDistortionPreset Implementation
 *****************************************************************************/

UAtomBusEffectDistortionPreset::UAtomBusEffectDistortionPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_DISTORTION_NAME);
	NumParameters = CRIATOMEXASR_DISTORTION_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectDistortionPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_DISTORTION_PARAMETER_DRIVE_DB: return DynamicSettings.Drive;
	case CRIATOMEXASR_DISTORTION_PARAMETER_DRY_MIX: return DynamicSettings.DryMix;
	case CRIATOMEXASR_DISTORTION_PARAMETER_WET_MIX: return DynamicSettings.WetMix;
	case CRIATOMEXASR_DISTORTION_PARAMETER_OUTPUT_GAIN: return Atom::ConvertToLinear(DynamicSettings.Gain); // dB to presure
	}

	return 0.0f;
}

void UAtomBusEffectDistortionPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_DISTORTION_PARAMETER_DRIVE_DB: DynamicSettings.Drive = Value; break;
	case CRIATOMEXASR_DISTORTION_PARAMETER_DRY_MIX: DynamicSettings.DryMix = Value; break;
	case CRIATOMEXASR_DISTORTION_PARAMETER_WET_MIX: DynamicSettings.WetMix = Value; break;
	case CRIATOMEXASR_DISTORTION_PARAMETER_OUTPUT_GAIN: DynamicSettings.Gain = Atom::ConvertToDecibels(Value); break; // Presure to dB
	}
}

#if WITH_EDITOR
void UAtomBusEffectDistortionPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectDistortionPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectDistortionPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectDistortionPreset::SetSettings(const FAtomBusEffectDistortionSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectDistortionPreset::SetDefaultSettings(const FAtomBusEffectDistortionSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectDistortionPreset::SetDrive(float Drive)
{
	DynamicSettings.Drive = Drive;

	UpdateParameter(CRIATOMEXASR_DISTORTION_PARAMETER_DRIVE_DB);
}

void UAtomBusEffectDistortionPreset::SetDryMix(float Level)
{
	DynamicSettings.DryMix = Level;

	UpdateParameter(CRIATOMEXASR_DISTORTION_PARAMETER_DRY_MIX);
}

void UAtomBusEffectDistortionPreset::SetWetMix(float Level)
{
	DynamicSettings.WetMix = Level;

	UpdateParameter(CRIATOMEXASR_DISTORTION_PARAMETER_WET_MIX);
}

void UAtomBusEffectDistortionPreset::SetGain(float Gain)
{
	DynamicSettings.Gain = Gain;

	UpdateParameter(CRIATOMEXASR_DISTORTION_PARAMETER_OUTPUT_GAIN);
}

#undef LOCTEXT_NAMESPACE
