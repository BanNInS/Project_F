/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectBiquadFilter.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectBiquadFilter.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectBiquadFilter"

 /*
  * FAtomBusEffectBiquadFilterSettings Implementation
  *****************************************************************************/

FAtomBusEffectBiquadFilterSettings::FAtomBusEffectBiquadFilterSettings()
	: FilterType(EAtomBiquadFilterType::LowPass)
	, Cutoff(ATOM_MAX_FILTER_FREQUENCY)
	, Q(1.0f)
	, Gain(1.0f)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectBiquadFilterPreset Implementation
 *****************************************************************************/

UAtomBusEffectBiquadFilterPreset::UAtomBusEffectBiquadFilterPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_BIQUAD_FILTER_NAME);
	NumParameters = CRIATOMEXASR_BIQUAD_FILTER_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectBiquadFilterPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_TYPE: return (float)DynamicSettings.FilterType;
	case CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_FREQUENCY: return DynamicSettings.Cutoff;
	case CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_QUALITY_FACTOR: return DynamicSettings.Q;
	case CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_GAIN: return DynamicSettings.Gain;
	}

	return 0.0f;
}

void UAtomBusEffectBiquadFilterPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_TYPE: DynamicSettings.FilterType = (EAtomBiquadFilterType)Value; break;
	case CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_FREQUENCY: DynamicSettings.Cutoff = Value; break;
	case CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_QUALITY_FACTOR: DynamicSettings.Q = Value; break;
	case CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_GAIN: DynamicSettings.Gain = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectBiquadFilterPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectBiquadFilterPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectBiquadFilterPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectBiquadFilterPreset::SetSettings(const FAtomBusEffectBiquadFilterSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectBiquadFilterPreset::SetDefaultSettings(const FAtomBusEffectBiquadFilterSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectBiquadFilterPreset::SetFilterType(EAtomBiquadFilterType Type)
{
	DynamicSettings.FilterType = Type;

	UpdateParameter(CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_TYPE);
}

void UAtomBusEffectBiquadFilterPreset::SetCutoff(float Frequency)
{
	DynamicSettings.Cutoff = Frequency;

	UpdateParameter(CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_FREQUENCY);
}

void UAtomBusEffectBiquadFilterPreset::SetQ(float Factor)
{
	DynamicSettings.Q = Factor;

	UpdateParameter(CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_QUALITY_FACTOR);
}

void UAtomBusEffectBiquadFilterPreset::SetGain(float Level)
{
	DynamicSettings.Gain = Level;

	UpdateParameter(CRIATOMEXASR_BIQUAD_FILTER_PARAMETER_GAIN);
}

#undef LOCTEXT_NAMESPACE
