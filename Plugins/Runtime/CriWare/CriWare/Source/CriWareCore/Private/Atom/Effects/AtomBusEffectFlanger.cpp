/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectFlanger.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectFlanger.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectFlanger"

 /*
  * FAtomBusEffectFlangerSettings Implementation
  *****************************************************************************/

FAtomBusEffectFlangerSettings::FAtomBusEffectFlangerSettings()
	: DelayTime(10.0f)
	, Depth(0.1f)
	, Rate(1.0f)
	, Feedback(0.0f)
	, DryMix(0.71f)
	, WetMix(0.71f)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectFlangerPreset Implementation
 *****************************************************************************/

UAtomBusEffectFlangerPreset::UAtomBusEffectFlangerPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_FLANGER_NAME);
	NumParameters = CRIATOMEXASR_FLANGER_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectFlangerPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_FLANGER_PARAMETER_DELAY_TIME_MS: return DynamicSettings.DelayTime;
	case CRIATOMEXASR_FLANGER_PARAMETER_DEPTH: return DynamicSettings.Depth;
	case CRIATOMEXASR_FLANGER_PARAMETER_RATE: return DynamicSettings.Rate;
	case CRIATOMEXASR_FLANGER_PARAMETER_FEEDBACK: return DynamicSettings.Feedback;
	case CRIATOMEXASR_FLANGER_PARAMETER_DRY_MIX: return DynamicSettings.DryMix;
	case CRIATOMEXASR_FLANGER_PARAMETER_WET_MIX: return DynamicSettings.WetMix;
	}

	return 0.0f;
}

void UAtomBusEffectFlangerPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_FLANGER_PARAMETER_DELAY_TIME_MS: DynamicSettings.DelayTime = Value; break;
	case CRIATOMEXASR_FLANGER_PARAMETER_DEPTH: DynamicSettings.Depth = Value; break;
	case CRIATOMEXASR_FLANGER_PARAMETER_RATE: DynamicSettings.Rate = Value; break;
	case CRIATOMEXASR_FLANGER_PARAMETER_FEEDBACK: DynamicSettings.Feedback = Value; break;
	case CRIATOMEXASR_FLANGER_PARAMETER_DRY_MIX: DynamicSettings.DryMix = Value; break;
	case CRIATOMEXASR_FLANGER_PARAMETER_WET_MIX: DynamicSettings.WetMix = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectFlangerPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectFlangerPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectFlangerPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectFlangerPreset::SetSettings(const FAtomBusEffectFlangerSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectFlangerPreset::SetDefaultSettings(const FAtomBusEffectFlangerSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectFlangerPreset::SetDelayTime(float Time)
{
	DynamicSettings.DelayTime = Time;

	UpdateParameter(CRIATOMEXASR_FLANGER_PARAMETER_DELAY_TIME_MS);
}

void UAtomBusEffectFlangerPreset::SetDepth(float Level)
{
	DynamicSettings.Depth = Level;

	UpdateParameter(CRIATOMEXASR_FLANGER_PARAMETER_DEPTH);
}

void UAtomBusEffectFlangerPreset::SetRate(float Rate)
{
	DynamicSettings.Rate = Rate;

	UpdateParameter(CRIATOMEXASR_FLANGER_PARAMETER_RATE);
}

void UAtomBusEffectFlangerPreset::SetFeedback(float Level)
{
	DynamicSettings.Feedback = Level;

	UpdateParameter(CRIATOMEXASR_FLANGER_PARAMETER_FEEDBACK);
}

void UAtomBusEffectFlangerPreset::SetDryMix(float Level)
{
	DynamicSettings.DryMix = Level;

	UpdateParameter(CRIATOMEXASR_FLANGER_PARAMETER_DRY_MIX);
}

void UAtomBusEffectFlangerPreset::SetWetMix(float Level)
{
	DynamicSettings.WetMix = Level;

	UpdateParameter(CRIATOMEXASR_FLANGER_PARAMETER_WET_MIX);
}

#undef LOCTEXT_NAMESPACE
