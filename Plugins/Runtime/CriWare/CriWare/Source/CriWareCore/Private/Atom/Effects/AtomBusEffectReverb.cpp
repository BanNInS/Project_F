/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectReverb.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectReverb.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectReverb"

 /*
  * FAtomBusEffectReverbSettings Implementation
  *****************************************************************************/

FAtomBusEffectReverbSettings::FAtomBusEffectReverbSettings()
	: ReverbTime(3000.0f)
	, RoomSize(25.0f)
	, PreDelayTime(10.0f)
	, LowCutoff(0.0f)
	, HighCutoff(8000.0f)
	, MaximumRoomSize(50.0f)
	, MaximumPreDelayTime(1000.0f)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectReverbPreset Implementation
 *****************************************************************************/

UAtomBusEffectReverbPreset::UAtomBusEffectReverbPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_REVERB_NAME);
	NumParameters = CRIATOMEXASR_REVERB_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectReverbPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_REVERB_PARAMETER_REVERB_TIME_MS: return DynamicSettings.ReverbTime;
	case CRIATOMEXASR_REVERB_PARAMETER_ROOM_SIZE_M: return DynamicSettings.RoomSize;
	case CRIATOMEXASR_REVERB_PARAMETER_PRE_DELAY_TIME_MS: return DynamicSettings.PreDelayTime;
	case CRIATOMEXASR_REVERB_PARAMETER_LOW_COF: return DynamicSettings.LowCutoff;
	case CRIATOMEXASR_REVERB_PARAMETER_HIGH_COF: return DynamicSettings.HighCutoff;
	}

	return 0.0f;
}

void UAtomBusEffectReverbPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_REVERB_PARAMETER_REVERB_TIME_MS: DynamicSettings.ReverbTime = Value; break;
	case CRIATOMEXASR_REVERB_PARAMETER_ROOM_SIZE_M: DynamicSettings.RoomSize = Value; break;
	case CRIATOMEXASR_REVERB_PARAMETER_PRE_DELAY_TIME_MS: DynamicSettings.PreDelayTime = Value; break;
	case CRIATOMEXASR_REVERB_PARAMETER_LOW_COF: DynamicSettings.LowCutoff = Value; break;
	case CRIATOMEXASR_REVERB_PARAMETER_HIGH_COF: DynamicSettings.HighCutoff = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectReverbPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectReverbPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectReverbPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectReverbPreset::SetSettings(const FAtomBusEffectReverbSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectReverbPreset::SetDefaultSettings(const FAtomBusEffectReverbSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectReverbPreset::SetReverbTime(float Time)
{
	DynamicSettings.ReverbTime = Time;

	UpdateParameter(CRIATOMEXASR_REVERB_PARAMETER_REVERB_TIME_MS);
}

void UAtomBusEffectReverbPreset::SetRoomSize(float Meters)
{
	DynamicSettings.RoomSize = Meters;

	UpdateParameter(CRIATOMEXASR_REVERB_PARAMETER_ROOM_SIZE_M);
}

void UAtomBusEffectReverbPreset::SetPreDelayTime(float Time)
{
	DynamicSettings.PreDelayTime = Time;

	UpdateParameter(CRIATOMEXASR_REVERB_PARAMETER_PRE_DELAY_TIME_MS);
}

void UAtomBusEffectReverbPreset::SetLowCutoff(float Frenquency)
{
	DynamicSettings.LowCutoff = Frenquency;

	UpdateParameter(CRIATOMEXASR_REVERB_PARAMETER_LOW_COF);
}

void UAtomBusEffectReverbPreset::SetHighCutoff(float Frenquency)
{
	DynamicSettings.HighCutoff = Frenquency;

	UpdateParameter(CRIATOMEXASR_REVERB_PARAMETER_HIGH_COF);
}

#undef LOCTEXT_NAMESPACE
