/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectPitchShifter.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectPitchShifter.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectPitchShifter"

 /*
  * FAtomBusEffectPitchShifterSettings Implementation
  *****************************************************************************/

FAtomBusEffectPitchShifterSettings::FAtomBusEffectPitchShifterSettings()
	: PitchShift(7)
	, FormantShift(0)
	, ShiftMode(EAtomPitchShifterMode::Music)
	, DataPitchFrequency(459.375f)
	, bUseDataPitchFrequency(false)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectPitchShifterPreset Implementation
 *****************************************************************************/

UAtomBusEffectPitchShifterPreset::UAtomBusEffectPitchShifterPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_PITCH_SHIFTER_NAME);
	NumParameters = CRIATOMEXASR_PITCH_SHIFTER_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectPitchShifterPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_PITCH_RATIO: return Atom::GetFrequencyMultiplier(DynamicSettings.PitchShift);
	case CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_FORMANT_RATIO: return Atom::GetFrequencyMultiplier(DynamicSettings.FormantShift);
	case CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_MODE: return (float)DynamicSettings.ShiftMode;
	case CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_DATA_PITCH_FREQUENCY: return DynamicSettings.bUseDataPitchFrequency ? DynamicSettings.DataPitchFrequency : 459.375f;
	}

	return 0.0f;
}

void UAtomBusEffectPitchShifterPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_PITCH_RATIO: DynamicSettings.PitchShift = Atom::GetSemitones(Value); break;
	case CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_FORMANT_RATIO: DynamicSettings.FormantShift = Atom::GetSemitones(Value); break;
	case CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_MODE: DynamicSettings.ShiftMode = (EAtomPitchShifterMode)Value; break;
	case CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_DATA_PITCH_FREQUENCY: DynamicSettings.DataPitchFrequency = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectPitchShifterPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectPitchShifterPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectPitchShifterPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectPitchShifterPreset::SetSettings(const FAtomBusEffectPitchShifterSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectPitchShifterPreset::SetDefaultSettings(const FAtomBusEffectPitchShifterSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectPitchShifterPreset::SetPitchShift(int Cents)
{
	DynamicSettings.PitchShift = Cents;

	UpdateParameter(CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_PITCH_RATIO);
}

void UAtomBusEffectPitchShifterPreset::SetFormantShift(int Cents)
{
	DynamicSettings.FormantShift = Cents;

	UpdateParameter(CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_FORMANT_RATIO);
}

void UAtomBusEffectPitchShifterPreset::SetShiftMode(EAtomPitchShifterMode Mode)
{
	DynamicSettings.ShiftMode = Mode;

	UpdateParameter(CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_MODE);
}

void UAtomBusEffectPitchShifterPreset::SetDataPitchFrequency(float Frequency)
{
	DynamicSettings.DataPitchFrequency = Frequency;

	UpdateParameter(CRIATOMEXASR_PITCH_SHIFTER_PARAMETER_DATA_PITCH_FREQUENCY);
}

#undef LOCTEXT_NAMESPACE
