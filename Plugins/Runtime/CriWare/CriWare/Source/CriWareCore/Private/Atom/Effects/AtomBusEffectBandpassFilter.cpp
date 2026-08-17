/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectBandpassFilter.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectBandpassFilter.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectBandpassFilter"

 /*
  * FAtomBusEffectBandpassFilterSettings Implementation
  *****************************************************************************/

FAtomBusEffectBandpassFilterSettings::FAtomBusEffectBandpassFilterSettings()
	: LowCutoff(ATOM_MIN_FILTER_FREQUENCY)
	, HighCutoff(ATOM_MAX_FILTER_FREQUENCY)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectBandpassFilterPreset Implementation
 *****************************************************************************/

UAtomBusEffectBandpassFilterPreset::UAtomBusEffectBandpassFilterPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_BANDPASS_FILTER_NAME);
	NumParameters = CRIATOMEXASR_BANDPASS_FILTER_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectBandpassFilterPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_BANDPASS_FILTER_PARAMETER_LOW_FREQ: return DynamicSettings.LowCutoff;
	case CRIATOMEXASR_BANDPASS_FILTER_PARAMETER_HIGH_FREQ: return DynamicSettings.HighCutoff;
	}

	return 0.0f;
}

void UAtomBusEffectBandpassFilterPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_BANDPASS_FILTER_PARAMETER_LOW_FREQ: DynamicSettings.LowCutoff = Value; break;
	case CRIATOMEXASR_BANDPASS_FILTER_PARAMETER_HIGH_FREQ: DynamicSettings.HighCutoff = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectBandpassFilterPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectBandpassFilterPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectBandpassFilterPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectBandpassFilterPreset::SetSettings(const FAtomBusEffectBandpassFilterSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectBandpassFilterPreset::SetDefaultSettings(const FAtomBusEffectBandpassFilterSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectBandpassFilterPreset::SetLowCutoff(float Frenquency)
{
	DynamicSettings.LowCutoff = Frenquency;

	UpdateParameter(CRIATOMEXASR_BANDPASS_FILTER_PARAMETER_LOW_FREQ);
}

void UAtomBusEffectBandpassFilterPreset::SetHighCutoff(float Frenquency)
{
	DynamicSettings.HighCutoff = Frenquency;

	UpdateParameter(CRIATOMEXASR_BANDPASS_FILTER_PARAMETER_HIGH_FREQ);
}

#undef LOCTEXT_NAMESPACE
