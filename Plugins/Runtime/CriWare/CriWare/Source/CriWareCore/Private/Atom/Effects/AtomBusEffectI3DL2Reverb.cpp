/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectI3DL2Reverb.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectI3DL2Reverb.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectI3DL2Reverb"

 /*
  * FAtomBusEffectI3DL2ReverbSettings Implementation
  *****************************************************************************/

FAtomBusEffectI3DL2ReverbSettings::FAtomBusEffectI3DL2ReverbSettings()
	: Room(-10.0f)
	, RoomHF(-1.0f)
	, DecayTime(1.49f)
	, DecayHFRatio(0.83f)
	, Reflections(-26.02f)
	, ReflectionsDelay(0.007f)
	, Reverb(2.0f)
	, ReverbDelay(0.011f)
	, Diffusion(100.0f)
	, Density(100.0f)
	, HFReference(5000.0f)
	, FrontInput(0.0f)
	, RearInput(0.0f)
	, CenterInput(-100.0f)
	, FrontOutput(0.0f)
	, RearOutput(0.0f)
	, CenterOutput(-100.0f)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif

	// todo: use presets UI in editor
	// CRIATOMEXASR_I3DL2_REVERB_PRESET_GENERIC
	//	CRIATOMEXASR_I3DL2_REVERB_PRESET_BATHROOM ...
}

/*
 * UAtomBusEffectI3DL2ReverbPreset Implementation
 *****************************************************************************/

UAtomBusEffectI3DL2ReverbPreset::UAtomBusEffectI3DL2ReverbPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_I3DL2_REVERB_NAME);
	NumParameters = CRIATOMEXASR_I3DL2_REVERB_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectI3DL2ReverbPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_ROOM: return DynamicSettings.Room * 100.0f; // decibels to milibels
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_ROOM_HF: return DynamicSettings.RoomHF * 100.0f;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DECAY_TIME: return DynamicSettings.DecayTime;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DECAY_HF_RATIO: return DynamicSettings.DecayHFRatio;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REFLECTIONS: return DynamicSettings.Reflections * 100.0f;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REFLECTIONS_DELAY: return DynamicSettings.ReflectionsDelay;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REVERB: return DynamicSettings.Reverb * 100.0f;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REVERB_DELAY: return DynamicSettings.ReverbDelay;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DIFFUSION: return DynamicSettings.Diffusion;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DENSITY: return DynamicSettings.Density;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_HF_REFERENCE: return DynamicSettings.HFReference;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_FRONT_INPUT: return DynamicSettings.FrontInput * 100.0f;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REAR_INPUT: return DynamicSettings.RearInput * 100.0f;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_CENTER_INPUT: return DynamicSettings.CenterInput * 100.0f;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_FRONT_OUTPUT: return DynamicSettings.FrontOutput * 100.0f;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REAR_OUTPUT: return DynamicSettings.RearOutput * 100.0f;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_CENTER_OUTPUT: return DynamicSettings.CenterOutput * 100.0f;
	}

	return 0.0f;
}

void UAtomBusEffectI3DL2ReverbPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_ROOM: DynamicSettings.Room = Value * 0.01f; break; // milibels to decibels 
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_ROOM_HF: DynamicSettings.RoomHF = Value * 0.01f; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DECAY_TIME: DynamicSettings.DecayTime = Value; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DECAY_HF_RATIO: DynamicSettings.DecayHFRatio = Value; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REFLECTIONS: DynamicSettings.Reflections = Value * 0.01f; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REFLECTIONS_DELAY: DynamicSettings.ReflectionsDelay = Value; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REVERB: DynamicSettings.Reverb = Value * 0.01f; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REVERB_DELAY: DynamicSettings.ReverbDelay = Value; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DIFFUSION: DynamicSettings.Diffusion = Value; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DENSITY: DynamicSettings.Density = Value; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_HF_REFERENCE: DynamicSettings.HFReference = Value; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_FRONT_INPUT: DynamicSettings.FrontInput = Value * 0.01f; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REAR_INPUT: DynamicSettings.RearInput = Value * 0.01f; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_CENTER_INPUT: DynamicSettings.CenterInput = Value * 0.01f; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_FRONT_OUTPUT: DynamicSettings.FrontOutput = Value * 0.01f; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REAR_OUTPUT: DynamicSettings.RearOutput = Value * 0.01f; break;
	case CRIATOMEXASR_I3DL2_REVERB_PARAMETER_CENTER_OUTPUT: DynamicSettings.CenterOutput = Value * 0.01f; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectI3DL2ReverbPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectI3DL2ReverbPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectI3DL2ReverbPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectI3DL2ReverbPreset::SetSettings(const FAtomBusEffectI3DL2ReverbSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectI3DL2ReverbPreset::SetDefaultSettings(const FAtomBusEffectI3DL2ReverbSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectI3DL2ReverbPreset::SetRoom(float Gain)
{
	DynamicSettings.Room = Gain;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_ROOM);
}

void UAtomBusEffectI3DL2ReverbPreset::SetRoomHF(float Gain)
{
	DynamicSettings.RoomHF = Gain;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_ROOM_HF);
}

void UAtomBusEffectI3DL2ReverbPreset::SetDecayTime(float Time)
{
	DynamicSettings.DecayTime = Time;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DECAY_TIME);
}

void UAtomBusEffectI3DL2ReverbPreset::SetDecayHFRatio(float Ratio)
{
	DynamicSettings.DecayHFRatio = Ratio;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DECAY_HF_RATIO);
}

void UAtomBusEffectI3DL2ReverbPreset::SetReflections(float Gain)
{
	DynamicSettings.Reflections = Gain;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REFLECTIONS);
}

void UAtomBusEffectI3DL2ReverbPreset::SetReflectionsDelay(float Time)
{
	DynamicSettings.ReflectionsDelay = Time;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REFLECTIONS_DELAY);
}

void UAtomBusEffectI3DL2ReverbPreset::SetReverb(float Gain)
{
	DynamicSettings.Reverb = Gain;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REVERB);
}

void UAtomBusEffectI3DL2ReverbPreset::SetReverbDelay(float Time)
{
	DynamicSettings.ReverbDelay = Time;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REVERB_DELAY);
}

void UAtomBusEffectI3DL2ReverbPreset::SetDiffusion(float Percent)
{
	DynamicSettings.Diffusion = Percent;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DIFFUSION);
}

void UAtomBusEffectI3DL2ReverbPreset::SetDensity(float Percent)
{
	DynamicSettings.Density = Percent;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_DENSITY);
}

void UAtomBusEffectI3DL2ReverbPreset::SetHFReference(float Frequency)
{
	DynamicSettings.HFReference = Frequency;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_HF_REFERENCE);
}

void UAtomBusEffectI3DL2ReverbPreset::SetFrontInput(float Gain)
{
	DynamicSettings.FrontInput = Gain;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_FRONT_INPUT);
}

void UAtomBusEffectI3DL2ReverbPreset::SetRearInput(float Gain)
{
	DynamicSettings.RearInput = Gain;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REAR_INPUT);
}

void UAtomBusEffectI3DL2ReverbPreset::SetCenterInput(float Gain)
{
	DynamicSettings.CenterInput = Gain;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_CENTER_INPUT);
}

void UAtomBusEffectI3DL2ReverbPreset::SetFrontOutput(float Gain)
{
	DynamicSettings.FrontOutput = Gain;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_FRONT_OUTPUT);
}

void UAtomBusEffectI3DL2ReverbPreset::SetRearOutput(float Gain)
{
	DynamicSettings.RearOutput = Gain;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_REAR_OUTPUT);
}

void UAtomBusEffectI3DL2ReverbPreset::SetCenterOutput(float Gain)
{
	DynamicSettings.CenterOutput = Gain;

	UpdateParameter(CRIATOMEXASR_I3DL2_REVERB_PARAMETER_CENTER_OUTPUT);
}

#if WITH_EDITOR
bool UAtomBusEffectI3DL2ReverbPreset::IsAtomAsrPreset(float AsrPreset[11])
{
	for (int i = 0; i < 11; i++) {
		if (!FMath::IsNearlyEqual(AsrPreset[i], GetParameterValue(i))) {
			return false;
		}
	}
	return true;
}
#endif

/*
 * UAtomBusEffectI3DL2Reverb2Preset Implementation
 *****************************************************************************/

UAtomBusEffectI3DL2Reverb2Preset::UAtomBusEffectI3DL2Reverb2Preset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_I3DL2_REVERB2_NAME);
	check(CRIATOMEXASR_I3DL2_REVERB_NUM_PARAMETERS == CRIATOMEXASR_I3DL2_REVERB2_NUM_PARAMETERS);

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

#undef LOCTEXT_NAMESPACE
