/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectLimiter.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectLimiter.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectLimiter"

 /*
  * FAtomBusEffectLimiterSettings Implementation
  *****************************************************************************/

FAtomBusEffectLimiterSettings::FAtomBusEffectLimiterSettings()
	: Threshold(-12.5f)
	, AttackTime(10.0f)
	, ReleaseTime(48.0f)
	, Gain(4.0f)
	, SurroundLink(100.0f)
	, Type(EAtomLimiterType::Peak)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectLimiterPreset Implementation
 *****************************************************************************/

UAtomBusEffectLimiterPreset::UAtomBusEffectLimiterPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_LIMITER_NAME);
	NumParameters = CRIATOMEXASR_LIMITER_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectLimiterPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_LIMITER_PARAMETER_THRESHOLD: return Atom::ConvertToLinear(DynamicSettings.Threshold); // dB to presure
	case CRIATOMEXASR_LIMITER_PARAMETER_ATTACK_TIME_MS: return DynamicSettings.AttackTime;
	case CRIATOMEXASR_LIMITER_PARAMETER_RELEASE_TIME_MS: return DynamicSettings.ReleaseTime;
	case CRIATOMEXASR_LIMITER_PARAMETER_OUTPUT_GAIN: return Atom::ConvertToLinear(DynamicSettings.Gain); // dB to presure
	case CRIATOMEXASR_LIMITER_PARAMETER_SURROUND_LINK: return DynamicSettings.SurroundLink * 0.01; // % to level
	case CRIATOMEXASR_LIMITER_PARAMETER_TYPE: return (float)DynamicSettings.Type;
	}

	return 0.0f;
}

void UAtomBusEffectLimiterPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_LIMITER_PARAMETER_THRESHOLD: DynamicSettings.Threshold = Atom::ConvertToDecibels(Value); break; // presure to dB
	case CRIATOMEXASR_LIMITER_PARAMETER_ATTACK_TIME_MS: DynamicSettings.AttackTime = Value; break;
	case CRIATOMEXASR_LIMITER_PARAMETER_RELEASE_TIME_MS: DynamicSettings.ReleaseTime = Value; break;
	case CRIATOMEXASR_LIMITER_PARAMETER_OUTPUT_GAIN: DynamicSettings.Gain = Atom::ConvertToDecibels(Value); break; // presure to dB
	case CRIATOMEXASR_LIMITER_PARAMETER_SURROUND_LINK: DynamicSettings.SurroundLink = Value * 100.0f; break; // level to %
	case CRIATOMEXASR_LIMITER_PARAMETER_TYPE: DynamicSettings.Type = (EAtomLimiterType)Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectLimiterPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectLimiterPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectLimiterPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectLimiterPreset::SetSettings(const FAtomBusEffectLimiterSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectLimiterPreset::SetDefaultSettings(const FAtomBusEffectLimiterSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectLimiterPreset::SetThreshold(float Gain)
{
	DynamicSettings.Threshold = Gain;

	UpdateParameter(CRIATOMEXASR_LIMITER_PARAMETER_THRESHOLD);
}

void UAtomBusEffectLimiterPreset::SetAttackTime(float Time)
{
	DynamicSettings.AttackTime = Time;

	UpdateParameter(CRIATOMEXASR_LIMITER_PARAMETER_ATTACK_TIME_MS);
}

void UAtomBusEffectLimiterPreset::SetReleaseTime(float Time)
{
	DynamicSettings.ReleaseTime = Time;

	UpdateParameter(CRIATOMEXASR_LIMITER_PARAMETER_RELEASE_TIME_MS);
}

void UAtomBusEffectLimiterPreset::SetGain(float Gain)
{
	DynamicSettings.Gain = Gain;

	UpdateParameter(CRIATOMEXASR_LIMITER_PARAMETER_OUTPUT_GAIN);
}

void UAtomBusEffectLimiterPreset::SetSurroundLink(float Percent)
{
	DynamicSettings.SurroundLink = Percent;

	UpdateParameter(CRIATOMEXASR_LIMITER_PARAMETER_SURROUND_LINK);
}

void UAtomBusEffectLimiterPreset::SetType(EAtomLimiterType Type)
{
	DynamicSettings.Type = Type;

	UpdateParameter(CRIATOMEXASR_LIMITER_PARAMETER_TYPE);
}

#undef LOCTEXT_NAMESPACE
