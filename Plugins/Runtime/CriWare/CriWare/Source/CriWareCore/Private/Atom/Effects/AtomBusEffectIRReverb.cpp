/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectIRReverb.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectIRReverb.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectIRReverb"

 /*
  * FAtomBusEffectIRReverbSettings Implementation
  *****************************************************************************/

FAtomBusEffectIRReverbSettings::FAtomBusEffectIRReverbSettings()
	: DryGain(1.0f)
	, WetGain(1.0f)
	, ReverbGain(0.1f)
	, InputSeparationRatio(0.0f)
	, FrontInputGain(0.7f)
	, CenterInputGain(0.0f)
	, SurroundInputGain(0.7f)
	, SurroundBackInputGain(0.7f)
	, FrontOutputGain(0.7f)
	, CenterOutputGain(0.0f)
	, SurroundOutputGain(0.0f)
	, SurroundBackOutputGain(0.0f)
	, IRIndex(0)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectIRReverbPreset Implementation
 *****************************************************************************/

UAtomBusEffectIRReverbPreset::UAtomBusEffectIRReverbPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_IR_REVERB_NAME);
	NumParameters = CRIATOMEXASR_IR_REVERB_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectIRReverbPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_IR_REVERB_PARAMETER_DRY_GAIN: return DynamicSettings.DryGain;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_WET_GAIN: return DynamicSettings.WetGain;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_REVERB_GAIN: return DynamicSettings.ReverbGain;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_INPUT_SEPARATION_RATIO: return DynamicSettings.InputSeparationRatio;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_FRONT_INPUT_GAIN: return DynamicSettings.FrontInputGain;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_CENTER_INPUT_GAIN: return DynamicSettings.CenterInputGain;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_INPUT_GAIN: return DynamicSettings.SurroundInputGain;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_BACK_INPUT_GAIN: return DynamicSettings.SurroundBackInputGain;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_FRONT_OUTPUT_GAIN: return DynamicSettings.FrontOutputGain;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_CENTER_OUTPUT_GAIN: return DynamicSettings.CenterOutputGain;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_OUTPUT_GAIN: return DynamicSettings.SurroundOutputGain;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_BACK_OUTPUT_GAIN: return DynamicSettings.SurroundBackOutputGain;
	}

	return 0.0f;
}

void UAtomBusEffectIRReverbPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_IR_REVERB_PARAMETER_DRY_GAIN: DynamicSettings.DryGain = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_WET_GAIN: DynamicSettings.WetGain = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_REVERB_GAIN: DynamicSettings.ReverbGain = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_INPUT_SEPARATION_RATIO: DynamicSettings.InputSeparationRatio = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_FRONT_INPUT_GAIN: DynamicSettings.FrontInputGain = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_CENTER_INPUT_GAIN: DynamicSettings.CenterInputGain = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_INPUT_GAIN: DynamicSettings.SurroundInputGain = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_BACK_INPUT_GAIN: DynamicSettings.SurroundBackInputGain = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_FRONT_OUTPUT_GAIN: DynamicSettings.FrontOutputGain = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_CENTER_OUTPUT_GAIN: DynamicSettings.CenterOutputGain = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_OUTPUT_GAIN: DynamicSettings.SurroundOutputGain = Value; break;
	case CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_BACK_OUTPUT_GAIN: DynamicSettings.SurroundBackOutputGain = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectIRReverbPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectIRReverbPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectIRReverbPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectIRReverbPreset::SetSettings(const FAtomBusEffectIRReverbSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectIRReverbPreset::SetDefaultSettings(const FAtomBusEffectIRReverbSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectIRReverbPreset::SetDryGain(float Gain)
{
	DynamicSettings.DryGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_DRY_GAIN);
}

void UAtomBusEffectIRReverbPreset::SetWetGain(float Gain)
{
	DynamicSettings.WetGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_WET_GAIN);
}

void UAtomBusEffectIRReverbPreset::SetReverbGain(float Gain)
{
	DynamicSettings.ReverbGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_REVERB_GAIN);
}

void UAtomBusEffectIRReverbPreset::SetInputSeparationRatio(float Ratio)
{
	DynamicSettings.InputSeparationRatio = Ratio;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_INPUT_SEPARATION_RATIO);
}

void UAtomBusEffectIRReverbPreset::SetFrontInputGain(float Gain)
{
	DynamicSettings.FrontInputGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_FRONT_INPUT_GAIN);
}

void UAtomBusEffectIRReverbPreset::SetCenterInputGain(float Gain)
{
	DynamicSettings.CenterInputGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_CENTER_INPUT_GAIN);
}

void UAtomBusEffectIRReverbPreset::SetSurroundInputGain(float Gain)
{
	DynamicSettings.SurroundInputGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_INPUT_GAIN);
}

void UAtomBusEffectIRReverbPreset::SetSurroundBackInputGain(float Gain)
{
	DynamicSettings.SurroundBackInputGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_BACK_INPUT_GAIN);
}

void UAtomBusEffectIRReverbPreset::SetFrontOutputGain(float Gain)
{
	DynamicSettings.FrontOutputGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_FRONT_OUTPUT_GAIN);
}

void UAtomBusEffectIRReverbPreset::SetCenterOutputGain(float Gain)
{
	DynamicSettings.CenterOutputGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_CENTER_OUTPUT_GAIN);
}

void UAtomBusEffectIRReverbPreset::SetSurroundOutputGain(float Gain)
{
	DynamicSettings.SurroundOutputGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_OUTPUT_GAIN);
}

void UAtomBusEffectIRReverbPreset::SetSurroundBackOutputGain(float Gain)
{
	DynamicSettings.SurroundBackOutputGain = Gain;

	UpdateParameter(CRIATOMEXASR_IR_REVERB_PARAMETER_SURROUND_BACK_OUTPUT_GAIN);
}

#undef LOCTEXT_NAMESPACE
