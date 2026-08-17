/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectPhaser.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectPhaser.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectPhaser"

 /*
  * FAtomBusEffectPhaserSettings Implementation
  *****************************************************************************/

FAtomBusEffectPhaserSettings::FAtomBusEffectPhaserSettings()
	: Stages(8)
	, Depth(0.5f)
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
 * UAtomBusEffectPhaserPreset Implementation
 *****************************************************************************/

UAtomBusEffectPhaserPreset::UAtomBusEffectPhaserPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_PHASER_NAME);
	NumParameters = CRIATOMEXASR_PHASER_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectPhaserPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_PHASER_PARAMETER_STAGES: return (float)DynamicSettings.Stages;
	case CRIATOMEXASR_PHASER_PARAMETER_DEPTH: return DynamicSettings.Depth;
	case CRIATOMEXASR_PHASER_PARAMETER_RATE: return DynamicSettings.Rate;
	case CRIATOMEXASR_PHASER_PARAMETER_FEEDBACK: return DynamicSettings.Feedback;
	case CRIATOMEXASR_PHASER_PARAMETER_DRY_MIX: return DynamicSettings.DryMix;
	case CRIATOMEXASR_PHASER_PARAMETER_WET_MIX: return DynamicSettings.WetMix;
	}

	return 0.0f;
}

void UAtomBusEffectPhaserPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	switch (ParameterIndex)
	{
	case CRIATOMEXASR_PHASER_PARAMETER_STAGES: DynamicSettings.Stages = (int)Value; break;
	case CRIATOMEXASR_PHASER_PARAMETER_DEPTH: DynamicSettings.Depth = Value; break;
	case CRIATOMEXASR_PHASER_PARAMETER_RATE: DynamicSettings.Rate = Value; break;
	case CRIATOMEXASR_PHASER_PARAMETER_FEEDBACK: DynamicSettings.Feedback = Value; break;
	case CRIATOMEXASR_PHASER_PARAMETER_DRY_MIX: DynamicSettings.DryMix = Value; break;
	case CRIATOMEXASR_PHASER_PARAMETER_WET_MIX: DynamicSettings.WetMix = Value; break;
	}
}

#if WITH_EDITOR
void UAtomBusEffectPhaserPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectPhaserPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectPhaserPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectPhaserPreset::SetSettings(const FAtomBusEffectPhaserSettings& InSettings)
{
	DynamicSettings = InSettings;
	UpdateSettings(InSettings);
}

void UAtomBusEffectPhaserPreset::SetDefaultSettings(const FAtomBusEffectPhaserSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectPhaserPreset::SetStages(int Number)
{
	DynamicSettings.Stages = Number;

	UpdateParameter(CRIATOMEXASR_PHASER_PARAMETER_STAGES);
}

void UAtomBusEffectPhaserPreset::SetDepth(float Level)
{
	DynamicSettings.Depth = Level;

	UpdateParameter(CRIATOMEXASR_PHASER_PARAMETER_DEPTH);
}

void UAtomBusEffectPhaserPreset::SetRate(float Rate)
{
	DynamicSettings.Rate = Rate;

	UpdateParameter(CRIATOMEXASR_PHASER_PARAMETER_RATE);
}

void UAtomBusEffectPhaserPreset::SetFeedback(float Level)
{
	DynamicSettings.Feedback = Level;

	UpdateParameter(CRIATOMEXASR_PHASER_PARAMETER_FEEDBACK);
}

void UAtomBusEffectPhaserPreset::SetDryMix(float Level)
{
	DynamicSettings.DryMix = Level;

	UpdateParameter(CRIATOMEXASR_PHASER_PARAMETER_DRY_MIX);
}

void UAtomBusEffectPhaserPreset::SetWetMix(float Level)
{
	DynamicSettings.WetMix = Level;

	UpdateParameter(CRIATOMEXASR_PHASER_PARAMETER_WET_MIX);
}

#undef LOCTEXT_NAMESPACE
