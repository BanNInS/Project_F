/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectMatrix.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectMatrix.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectMatrix"

 /*
  * FAtomBusEffectMatrixSettings Implementation
  *****************************************************************************/

FAtomBusEffectMatrixSettings::FAtomBusEffectMatrixSettings()
	: LevelMatrix(CRIATOMEXASR_MATRIX_MAX_NUM_CHANNELS, CRIATOMEXASR_MATRIX_MAX_NUM_CHANNELS)
	, bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectMatrixPreset Implementation
 *****************************************************************************/

UAtomBusEffectMatrixPreset::UAtomBusEffectMatrixPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_MATRIX_NAME);
	NumParameters = CRIATOMEXASR_MATRIX_DEFAULT_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectMatrixPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	const EAtomSpeaker InputChannel = (EAtomSpeaker)(ParameterIndex / CRIATOMEXASR_MATRIX_MAX_NUM_CHANNELS);
	const EAtomSpeaker OutputChannel = (EAtomSpeaker)(ParameterIndex % CRIATOMEXASR_MATRIX_MAX_NUM_CHANNELS);

	return DynamicSettings.LevelMatrix.GetChannelLevel(InputChannel, OutputChannel);
}

void UAtomBusEffectMatrixPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	const EAtomSpeaker InputChannel = (EAtomSpeaker)(ParameterIndex / CRIATOMEXASR_MATRIX_MAX_NUM_CHANNELS);
	const EAtomSpeaker OutputChannel = (EAtomSpeaker)(ParameterIndex % CRIATOMEXASR_MATRIX_MAX_NUM_CHANNELS);

	DynamicSettings.LevelMatrix.SetChannelLevel(InputChannel, OutputChannel, Value);
}

#if WITH_EDITOR
void UAtomBusEffectMatrixPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectMatrixPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectMatrixPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectMatrixPreset::SetSettings(const FAtomBusEffectMatrixSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectMatrixPreset::SetDefaultSettings(const FAtomBusEffectMatrixSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectMatrixPreset::SetLevelMatrix(const FAtomChannelLevelMatrix& LevelMatrix)
{
	DynamicSettings.LevelMatrix = LevelMatrix;

	for (int Index = 0; Index < NumParameters; Index++)
	{
		UpdateParameter(Index);
	}
}

void UAtomBusEffectMatrixPreset::SetChannelLevel(EAtomSpeaker InChannel, EAtomSpeaker OutChannel, float Level)
{
	DynamicSettings.LevelMatrix.SetChannelLevel(InChannel, OutChannel, Level);

	UpdateParameter(CRIATOMEXASR_MATRIX_PARAMETER_LEVELS((int)InChannel, (int)OutChannel));
}

#undef LOCTEXT_NAMESPACE
