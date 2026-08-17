/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectChorus.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectChorus.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectChorus"

 /*
  * FAtomBusEffectChorusSettings Implementation
  *****************************************************************************/

FAtomBusEffectChorusSettings::FAtomBusEffectChorusSettings()
	: DelayTime(20.0f)
	, Depth(0.1f)
	, Rate(1.0f)
	, Feedback(0.0f)
	, DryMix(0.71f)
	, WetMix1(0.71f)
	, WetMix2(0.71f)
	, WetMix3(0.71f)
	, MaximumDelayTime(100.0f)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectChorusPreset Implementation
 *****************************************************************************/

UAtomBusEffectChorusPreset::UAtomBusEffectChorusPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_CHORUS_NAME);
	NumParameters = CRIATOMEXASR_CHORUS_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectChorusPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_CHORUS_PARAMETER_DELAY_TIME_MS: return DynamicSettings.DelayTime;
	case CRIATOMEXASR_CHORUS_PARAMETER_DEPTH: return DynamicSettings.Depth;
	case CRIATOMEXASR_CHORUS_PARAMETER_RATE: return DynamicSettings.Rate;
	case CRIATOMEXASR_CHORUS_PARAMETER_FEEDBACK: return DynamicSettings.Feedback;
	case CRIATOMEXASR_CHORUS_PARAMETER_DRY_MIX: return DynamicSettings.DryMix;
	case CRIATOMEXASR_CHORUS_PARAMETER_WET_MIX(0): return DynamicSettings.WetMix1;
	case CRIATOMEXASR_CHORUS_PARAMETER_WET_MIX(1): return DynamicSettings.WetMix2;
	case CRIATOMEXASR_CHORUS_PARAMETER_WET_MIX(2): return DynamicSettings.WetMix3;
	}

	return 0.0f;
}

void UAtomBusEffectChorusPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_CHORUS_PARAMETER_DELAY_TIME_MS: DynamicSettings.DelayTime = Value; break;
	case CRIATOMEXASR_CHORUS_PARAMETER_DEPTH: DynamicSettings.Depth = Value; break;
	case CRIATOMEXASR_CHORUS_PARAMETER_RATE: DynamicSettings.Rate = Value; break;
	case CRIATOMEXASR_CHORUS_PARAMETER_FEEDBACK: DynamicSettings.Feedback = Value; break;
	case CRIATOMEXASR_CHORUS_PARAMETER_DRY_MIX: DynamicSettings.DryMix = Value; break;
	case CRIATOMEXASR_CHORUS_PARAMETER_WET_MIX(0): DynamicSettings.WetMix1 = Value; break;
	case CRIATOMEXASR_CHORUS_PARAMETER_WET_MIX(1): DynamicSettings.WetMix2 = Value; break;
	case CRIATOMEXASR_CHORUS_PARAMETER_WET_MIX(2): DynamicSettings.WetMix3 = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectChorusPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectChorusPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectChorusPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectChorusPreset::SetSettings(const FAtomBusEffectChorusSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectChorusPreset::SetDefaultSettings(const FAtomBusEffectChorusSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectChorusPreset::SetDelayTime(float Time)
{
	DynamicSettings.DelayTime = Time;

	UpdateParameter(CRIATOMEXASR_CHORUS_PARAMETER_DELAY_TIME_MS);
}

void UAtomBusEffectChorusPreset::SetDepth(float Level)
{
	DynamicSettings.Depth = Level;

	UpdateParameter(CRIATOMEXASR_CHORUS_PARAMETER_DEPTH);
}

void UAtomBusEffectChorusPreset::SetRate(float Rate)
{
	DynamicSettings.Rate = Rate;

	UpdateParameter(CRIATOMEXASR_CHORUS_PARAMETER_RATE);
}

void UAtomBusEffectChorusPreset::SetFeedback(float Level)
{
	DynamicSettings.Feedback = Level;

	UpdateParameter(CRIATOMEXASR_CHORUS_PARAMETER_FEEDBACK);
}

void UAtomBusEffectChorusPreset::SetDryMix(float Level)
{
	DynamicSettings.DryMix = Level;

	UpdateParameter(CRIATOMEXASR_CHORUS_PARAMETER_DRY_MIX);
}

void UAtomBusEffectChorusPreset::SetWetMix1(float Level)
{
	DynamicSettings.WetMix1 = Level;

	UpdateParameter(CRIATOMEXASR_CHORUS_PARAMETER_WET_MIX(0));
}

void UAtomBusEffectChorusPreset::SetWetMix2(float Level)
{
	DynamicSettings.WetMix2 = Level;

	UpdateParameter(CRIATOMEXASR_CHORUS_PARAMETER_WET_MIX(1));
}

void UAtomBusEffectChorusPreset::SetWetMix3(float Level)
{
	DynamicSettings.WetMix3 = Level;

	UpdateParameter(CRIATOMEXASR_CHORUS_PARAMETER_WET_MIX(2));
}

#undef LOCTEXT_NAMESPACE
