/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectMultiTapDelay.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectMultiTapDelay.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectMultiTapDelay"

 /*
  * FAtomBusEffectMultiTapDelaySettings Implementation
  *****************************************************************************/

FAtomBusEffectMultiTapDelaySettings::FAtomBusEffectMultiTapDelaySettings()
	: MaximumDelayTime(CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_MAX_DELAY_TIME_MS)
	, bBypass(false)
{
	TapDelays.Add({ 20.0f, 1.0f, -30.0f, 0.0f });
	TapDelays.Add({ 40.0f, 1.0f, 30.0f, 0.0f });
	TapDelays.Add({ 60.0f, 1.0f, -120.0f, 0.0f });
	TapDelays.Add({ 80.0f, 1.0f, 120.0f, 0.0f });

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectMultiTapDelayPreset Implementation
 *****************************************************************************/

UAtomBusEffectMultiTapDelayPreset::UAtomBusEffectMultiTapDelayPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_MULTITAP_DELAY_NAME);
	NumParameters = CRIATOMEXASR_MULTITAP_DELAY_NUM_PARAMETERS;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectMultiTapDelayPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	const int TapIndex = ParameterIndex / CRIATOMEXASR_MULTITAP_DELAY_NUM_PARAMETER_PAR_TAP;
	if (DynamicSettings.TapDelays.IsValidIndex(TapIndex))
	{
		if (ParameterIndex == CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_DELAY_TIME_MS(TapIndex))
		{
			return DynamicSettings.GetTapDelay(TapIndex).DelayTime;
		}
		else if (ParameterIndex == CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_LEVEL(TapIndex))
		{
			return DynamicSettings.GetTapDelay(TapIndex).Level;
		}
		else if (ParameterIndex == CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_AZIMUTH(TapIndex))
		{
			return DynamicSettings.GetTapDelay(TapIndex).Azimuth;
		}
		else if (ParameterIndex == CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_FEEDBACK(TapIndex))
		{
			return DynamicSettings.GetTapDelay(TapIndex).Feedback;
		}
	}

	return 0.0f;
}

void UAtomBusEffectMultiTapDelayPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	const int TapIndex = ParameterIndex / CRIATOMEXASR_MULTITAP_DELAY_NUM_PARAMETER_PAR_TAP;
	if (DynamicSettings.TapDelays.IsValidIndex(TapIndex))
	{
		if (ParameterIndex == CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_DELAY_TIME_MS(TapIndex))
		{
			DynamicSettings.GetTapDelay(TapIndex).DelayTime = Value;
		}
		else if (ParameterIndex == CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_LEVEL(TapIndex))
		{
			DynamicSettings.GetTapDelay(TapIndex).Level = Value;
		}
		else if (ParameterIndex == CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_AZIMUTH(TapIndex))
		{
			DynamicSettings.GetTapDelay(TapIndex).Azimuth = Value;
		}
		else if (ParameterIndex == CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_FEEDBACK(TapIndex))
		{
			DynamicSettings.GetTapDelay(TapIndex).Feedback = Value;
		}
	}
}

#if WITH_EDITOR
void UAtomBusEffectMultiTapDelayPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectMultiTapDelayPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectMultiTapDelayPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectMultiTapDelayPreset::SetSettings(const FAtomBusEffectMultiTapDelaySettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectMultiTapDelayPreset::SetDefaultSettings(const FAtomBusEffectMultiTapDelaySettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectMultiTapDelayPreset::SetTapDelayTime(int TapIndex, float DelayTime)
{
	if (TapIndex < 0 || TapIndex > 3)
	{
		return;
	}

	DynamicSettings.GetTapDelay(TapIndex).DelayTime = DelayTime;

	UpdateParameter(CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_DELAY_TIME_MS(TapIndex));
}

void UAtomBusEffectMultiTapDelayPreset::SetTapLevel(int TapIndex, float Level)
{
	if (TapIndex < 0 || TapIndex > 3)
	{
		return;
	}

	DynamicSettings.GetTapDelay(TapIndex).Level = Level;

	UpdateParameter(CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_LEVEL(TapIndex));
}

void UAtomBusEffectMultiTapDelayPreset::SetTapAzimuth(int TapIndex, float Angle)
{
	if (TapIndex < 0 || TapIndex > 3)
	{
		return;
	}

	DynamicSettings.GetTapDelay(TapIndex).Azimuth = Angle;

	UpdateParameter(CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_AZIMUTH(TapIndex));
}

void UAtomBusEffectMultiTapDelayPreset::SetTapFeedback(int TapIndex, float Level)
{
	if (TapIndex < 0 || TapIndex > 3)
	{
		return;
	}

	DynamicSettings.GetTapDelay(TapIndex).Feedback = Level;

	UpdateParameter(CRIATOMEXASR_MULTITAP_DELAY_PARAMETER_FEEDBACK(TapIndex));
}

#undef LOCTEXT_NAMESPACE
