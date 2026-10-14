/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectDelay.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectDelay.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectDelay"

 /*
  * FAtomBusEffectDelaySettings Implementation
  *****************************************************************************/

FAtomBusEffectDelaySettings::FAtomBusEffectDelaySettings()
	: DelayTime(50.0f)
	, MaximumDelayTime(CRIATOMEXASR_DELAY_PARAMETER_MAX_DELAY_TIME_MS)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectDelayPreset Implementation
 *****************************************************************************/

UAtomBusEffectDelayPreset::UAtomBusEffectDelayPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_DELAY_NAME);
	NumParameters = CRIATOMEXASR_DELAY_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectDelayPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_DELAY_PARAMETER_DELAY_TIME_MS: return DynamicSettings.DelayTime;
	}

	return 0.0f;
}

void UAtomBusEffectDelayPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_DELAY_PARAMETER_DELAY_TIME_MS: DynamicSettings.DelayTime = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectDelayPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectDelayPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectDelayPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectDelayPreset::SetSettings(const FAtomBusEffectDelaySettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectDelayPreset::SetDefaultSettings(const FAtomBusEffectDelaySettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectDelayPreset::SetDelayTime(float Time)
{
	DynamicSettings.DelayTime = Time;

	UpdateParameter(CRIATOMEXASR_DELAY_PARAMETER_DELAY_TIME_MS);
}

#undef LOCTEXT_NAMESPACE
