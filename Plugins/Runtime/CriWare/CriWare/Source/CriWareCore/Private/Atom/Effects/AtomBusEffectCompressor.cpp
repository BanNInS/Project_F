/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectCompressor.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectCompressor.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectCompressor"

 /*
  * FAtomBusEffectCompressorSettings Implementation
  *****************************************************************************/

FAtomBusEffectCompressorSettings::FAtomBusEffectCompressorSettings()
	: Threshold(-12.5f)
	, Ratio(1.5f)
	, AttackTime(10.0f)
	, ReleaseTime(48.0f)
	, Gain(4.0f)
	, SurroundLink(100.0f)
	, SplitEQ({ EAtomSplitEQFilterType::None, 8000.0f, 0.6f })
	//, Type(EAtomCompressorType::Peak)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectCompressorPreset Implementation
 *****************************************************************************/

UAtomBusEffectCompressorPreset::UAtomBusEffectCompressorPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_COMPRESSOR_NAME);
	NumParameters = CRIATOMEXASR_COMPRESSOR_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectCompressorPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_THRESHOLD: return Atom::ConvertToLinear(DynamicSettings.Threshold); // dB to presure
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_RATIO: return DynamicSettings.Ratio;
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_ATTACK_TIME_MS: return DynamicSettings.AttackTime;
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_RELEASE_TIME_MS: return DynamicSettings.ReleaseTime;
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_OUTPUT_GAIN: return Atom::ConvertToLinear(DynamicSettings.Gain); // dB to presure
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_SURROUND_LINK: return DynamicSettings.SurroundLink * 0.01; // % to level
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_SPLIT_EQ_TYPE: return (float)DynamicSettings.SplitEQ.FilterType;
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_SPLIT_EQ_FREQUENCY: return DynamicSettings.SplitEQ.Frequency;
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_SPLIT_EQ_QUALITY_FACTOR: return DynamicSettings.SplitEQ.Q;
	//case CRIATOMEXASR_COMPRESSOR_PARAMETER_TYPE: return (float)DynamicSettings.Type;
	}

	return 0.0f;
}

void UAtomBusEffectCompressorPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_THRESHOLD: DynamicSettings.Threshold = Atom::ConvertToDecibels(Value); break; // presure to dB
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_RATIO: DynamicSettings.Ratio = Value; break;
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_ATTACK_TIME_MS: DynamicSettings.AttackTime = Value; break;
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_RELEASE_TIME_MS: DynamicSettings.ReleaseTime = Value; break;
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_OUTPUT_GAIN: DynamicSettings.Gain = Atom::ConvertToDecibels(Value); break; // presure to dB
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_SURROUND_LINK: DynamicSettings.SurroundLink = Value * 100.0f; break; // level to %
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_SPLIT_EQ_TYPE: DynamicSettings.SplitEQ.FilterType = (EAtomSplitEQFilterType)Value; break;
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_SPLIT_EQ_FREQUENCY: DynamicSettings.SplitEQ.Frequency = Value; break;
	case CRIATOMEXASR_COMPRESSOR_PARAMETER_SPLIT_EQ_QUALITY_FACTOR: DynamicSettings.SplitEQ.Q = Value; break;
	//case CRIATOMEXASR_COMPRESSOR_PARAMETER_TYPE: DynamicSettings.Type = (EAtomCompressorType)Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectCompressorPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectCompressorPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectCompressorPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectCompressorPreset::SetSettings(const FAtomBusEffectCompressorSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectCompressorPreset::SetDefaultSettings(const FAtomBusEffectCompressorSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectCompressorPreset::SetThreshold(float Gain)
{
	DynamicSettings.Threshold = Gain;

	UpdateParameter(CRIATOMEXASR_COMPRESSOR_PARAMETER_THRESHOLD);
}

void UAtomBusEffectCompressorPreset::SetRatio(float Ratio)
{
	DynamicSettings.Ratio = Ratio;

	UpdateParameter(CRIATOMEXASR_COMPRESSOR_PARAMETER_RATIO);
}

void UAtomBusEffectCompressorPreset::SetAttackTime(float Time)
{
	DynamicSettings.AttackTime = Time;

	UpdateParameter(CRIATOMEXASR_COMPRESSOR_PARAMETER_ATTACK_TIME_MS);
}

void UAtomBusEffectCompressorPreset::SetReleaseTime(float Time)
{
	DynamicSettings.ReleaseTime = Time;

	UpdateParameter(CRIATOMEXASR_COMPRESSOR_PARAMETER_RELEASE_TIME_MS);
}

void UAtomBusEffectCompressorPreset::SetGain(float Gain)
{
	DynamicSettings.Gain = Gain;

	UpdateParameter(CRIATOMEXASR_COMPRESSOR_PARAMETER_OUTPUT_GAIN);
}

void UAtomBusEffectCompressorPreset::SetSurroundLink(float Percent)
{
	DynamicSettings.SurroundLink = Percent;

	UpdateParameter(CRIATOMEXASR_COMPRESSOR_PARAMETER_SURROUND_LINK);
}

void UAtomBusEffectCompressorPreset::SetSplitEQFilterType(EAtomSplitEQFilterType Type)
{
	DynamicSettings.SplitEQ.FilterType = Type;

	UpdateParameter(CRIATOMEXASR_COMPRESSOR_PARAMETER_SPLIT_EQ_TYPE);
}

void UAtomBusEffectCompressorPreset::SetSplitEQFrequency(float Frequency)
{
	DynamicSettings.SplitEQ.Frequency = Frequency;

	UpdateParameter(CRIATOMEXASR_COMPRESSOR_PARAMETER_SPLIT_EQ_FREQUENCY);
}

void UAtomBusEffectCompressorPreset::SetSplitEQQ(float Factor)
{
	DynamicSettings.SplitEQ.Q = Factor;

	UpdateParameter(CRIATOMEXASR_COMPRESSOR_PARAMETER_SPLIT_EQ_QUALITY_FACTOR);
}

/*void UAtomBusEffectCompressorPreset::SetType(EAtomCompressorType Type)
{
	DynamicSettings.Type = Type;

	UpdateParameter(CRIATOMEXASR_COMPRESSOR_PARAMETER_TYPE);
}*/

#undef LOCTEXT_NAMESPACE
