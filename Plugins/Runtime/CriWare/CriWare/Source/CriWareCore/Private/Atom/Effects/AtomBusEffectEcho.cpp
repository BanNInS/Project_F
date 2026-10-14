/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectEcho.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectEcho.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectEcho"

 /*
  * FAtomBusEffectEchoSettings Implementation
  *****************************************************************************/

FAtomBusEffectEchoSettings::FAtomBusEffectEchoSettings()
	: DelayTime(50.0f)
	, Feedback(0.3f)
	, MaximumDelayTime(CRIATOMEXASR_ECHO_PARAMETER_MAX_DELAY_TIME_MS)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectEchoPreset Implementation
 *****************************************************************************/

UAtomBusEffectEchoPreset::UAtomBusEffectEchoPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_ECHO_NAME);
	NumParameters = CRIATOMEXASR_ECHO_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectEchoPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_ECHO_PARAMETER_DELAY_TIME_MS: return DynamicSettings.DelayTime;
	case CRIATOMEXASR_ECHO_PARAMETER_FB_GAIN: return DynamicSettings.Feedback;
	}

	return 0.0f;
}

void UAtomBusEffectEchoPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_ECHO_PARAMETER_DELAY_TIME_MS: DynamicSettings.DelayTime = Value; break;
	case CRIATOMEXASR_ECHO_PARAMETER_FB_GAIN: DynamicSettings.Feedback = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectEchoPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectEchoPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectEchoPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectEchoPreset::SetSettings(const FAtomBusEffectEchoSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectEchoPreset::SetDefaultSettings(const FAtomBusEffectEchoSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectEchoPreset::SetDelayTime(float Time)
{
	DynamicSettings.DelayTime = Time;

	UpdateParameter(CRIATOMEXASR_ECHO_PARAMETER_DELAY_TIME_MS);
}

void UAtomBusEffectEchoPreset::SetFeedback(float Level)
{
	DynamicSettings.Feedback = Level;

	UpdateParameter(CRIATOMEXASR_ECHO_PARAMETER_FB_GAIN);
}

#undef LOCTEXT_NAMESPACE
