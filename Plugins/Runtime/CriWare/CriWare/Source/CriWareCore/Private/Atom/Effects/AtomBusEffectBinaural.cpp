/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectBinaural.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectBinaural.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectBinaural"

 /*
  * FAtomBusEffectBinauralSettings Implementation
  *****************************************************************************/

FAtomBusEffectBinauralSettings::FAtomBusEffectBinauralSettings()
	: Gain(0.0f)
	, Balance(0.0f)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectBinauralPreset Implementation
 *****************************************************************************/

UAtomBusEffectBinauralPreset::UAtomBusEffectBinauralPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_HEADPHONE_VIRTUAL_SURROUND_NAME);
	NumParameters = CRIATOMEXASR_HEADPHONE_VIRTUAL_SURROUND_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectBinauralPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < 2);// FAtomRuntime::GetAtomExSoundRendererEffectNumParameters(EffectName));

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_HEADPHONE_VIRTUAL_SURROUND_GAIN: return DynamicSettings.Gain;
	case CRIATOMEXASR_HEADPHONE_VIRTUAL_SURROUND_BALANCE: return DynamicSettings.Balance;
	}

	return 0.0f;
}

void UAtomBusEffectBinauralPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < 2);// FAtomRuntime::GetAtomExSoundRendererEffectNumParameters(EffectName));

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_HEADPHONE_VIRTUAL_SURROUND_GAIN: DynamicSettings.Gain = Value; break;
	case CRIATOMEXASR_HEADPHONE_VIRTUAL_SURROUND_BALANCE: DynamicSettings.Balance = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectBinauralPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectBinauralPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectBinauralPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectBinauralPreset::SetSettings(const FAtomBusEffectBinauralSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectBinauralPreset::SetDefaultSettings(const FAtomBusEffectBinauralSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectBinauralPreset::SetGain(float Gain)
{
	DynamicSettings.Gain = Gain;

	UpdateParameter(CRIATOMEXASR_HEADPHONE_VIRTUAL_SURROUND_GAIN);
}

void UAtomBusEffectBinauralPreset::SetBalance(float Gain)
{
	DynamicSettings.Balance = Gain;

	UpdateParameter(CRIATOMEXASR_HEADPHONE_VIRTUAL_SURROUND_BALANCE);
}

#undef LOCTEXT_NAMESPACE
