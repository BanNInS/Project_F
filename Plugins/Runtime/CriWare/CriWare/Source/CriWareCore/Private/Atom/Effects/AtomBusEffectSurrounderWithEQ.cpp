/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectSurrounderWithEQ.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectSurrounderWithEQ.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectSurrounderWithEQ"

 /*
  * FAtomBusEffectSurrounderWithEQSettings Implementation
  *****************************************************************************/

FAtomBusEffectSurrounderWithEQSettings::FAtomBusEffectSurrounderWithEQSettings()
	: Mode(EAtomSurrounderMode::Straight)
	, DelayTime(50.0f)
	, Gain(-6.0f)
	, MaximumDelayTime(CRIATOMEXASR_DELAY_PARAMETER_MAX_DELAY_TIME_MS)
	, bBypass(false)
{
	// Front
	BandFilters.Add({ EAtomBandEQFilterType::Peaking, 135.0f, 1.0f, 0.0f });
	BandFilters.Add({ EAtomBandEQFilterType::Peaking, 759.0f, 1.0f, 0.0f });
	BandFilters.Add({ EAtomBandEQFilterType::Peaking, 4268.0f, 1.0f, 0.0f });
	// Back
	BandFilters.Add({ EAtomBandEQFilterType::Peaking, 208.0f, 1.0f, 0.0f });
	BandFilters.Add({ EAtomBandEQFilterType::Peaking, 1169.0f, 1.0f, 0.0f });
	BandFilters.Add({ EAtomBandEQFilterType::Peaking, 6572.0f, 1.0f, 0.0f });

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectSurrounderWithEQPreset Implementation
 *****************************************************************************/

UAtomBusEffectSurrounderWithEQPreset::UAtomBusEffectSurrounderWithEQPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_SURROUNDER_WITH_EQUALIZER_NAME);
	NumParameters = CRIATOMEXASR_SURROUNDER_NUM_PARAMETERS + CRIATOMEXASR_MULTIBANDS_EQ_NUM_PARAMETERS(6);

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

float UAtomBusEffectSurrounderWithEQPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	if (ParameterIndex < CRIATOMEXASR_SURROUNDER_NUM_PARAMETERS)
	{
		switch (ParameterIndex)
		{
		case CRIATOMEXASR_SURROUNDER_PARAMETER_MODE: return (float)DynamicSettings.Mode;
		case CRIATOMEXASR_SURROUNDER_PARAMETER_DELAY_TIME_MS: return DynamicSettings.DelayTime;
		case CRIATOMEXASR_SURROUNDER_PARAMETER_GAIN: return Atom::ConvertToLinear(DynamicSettings.Gain); // dB to presure
		}
	}
	else
	{
		ParameterIndex -= CRIATOMEXASR_SURROUNDER_NUM_PARAMETERS;

		const int BandIndex = ParameterIndex / CRIATOMEXASR_MULTIBANDS_EQ_NUM_PARAMETER_PAR_BAND;
		if (DynamicSettings.BandFilters.IsValidIndex(BandIndex))
		{
			if (ParameterIndex == CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_TYPE(BandIndex))
			{
				return (float)DynamicSettings.GetBandFilter(BandIndex).FilterType;
			}
			else if (ParameterIndex == CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_FREQUENCY(BandIndex))
			{
				return DynamicSettings.GetBandFilter(BandIndex).Frequency;
			}
			else if (ParameterIndex == CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_QUALITY_FACTOR(BandIndex))
			{
				return DynamicSettings.GetBandFilter(BandIndex).Q;
			}
			else if (ParameterIndex == CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_GAIN(BandIndex))
			{
				return DynamicSettings.GetBandFilter(BandIndex).Gain;
			}
		}
	}

	return 0.0f;
}

void UAtomBusEffectSurrounderWithEQPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

	if (ParameterIndex < CRIATOMEXASR_SURROUNDER_NUM_PARAMETERS)
	{
		switch (ParameterIndex)
		{
		case CRIATOMEXASR_SURROUNDER_PARAMETER_MODE: DynamicSettings.Mode = (EAtomSurrounderMode)Value; break;
		case CRIATOMEXASR_SURROUNDER_PARAMETER_DELAY_TIME_MS: DynamicSettings.DelayTime = Value; break;
		case CRIATOMEXASR_SURROUNDER_PARAMETER_GAIN: DynamicSettings.Gain = Atom::ConvertToDecibels(Value); break; // presure to dB
		}
	}
	else
	{
		ParameterIndex -= CRIATOMEXASR_SURROUNDER_NUM_PARAMETERS;

		const int BandIndex = ParameterIndex / CRIATOMEXASR_MULTIBANDS_EQ_NUM_PARAMETER_PAR_BAND;
		if (DynamicSettings.BandFilters.IsValidIndex(BandIndex))
		{
			if (ParameterIndex == CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_TYPE(BandIndex))
			{
				DynamicSettings.GetBandFilter(BandIndex).FilterType = (EAtomBandEQFilterType)Value;
			}
			else if (ParameterIndex == CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_FREQUENCY(BandIndex))
			{
				DynamicSettings.GetBandFilter(BandIndex).Frequency = Value;
			}
			else if (ParameterIndex == CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_QUALITY_FACTOR(BandIndex))
			{
				DynamicSettings.GetBandFilter(BandIndex).Q = Value;
			}
			else if (ParameterIndex == CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_GAIN(BandIndex))
			{
				DynamicSettings.GetBandFilter(BandIndex).Gain = Value;
			}
		}
	}
}

#if WITH_EDITOR
void UAtomBusEffectSurrounderWithEQPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectSurrounderWithEQPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectSurrounderWithEQPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectSurrounderWithEQPreset::SetSettings(const FAtomBusEffectSurrounderWithEQSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectSurrounderWithEQPreset::SetDefaultSettings(const FAtomBusEffectSurrounderWithEQSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectSurrounderWithEQPreset::SetMode(EAtomSurrounderMode Mode)
{
	DynamicSettings.Mode = Mode;

	UpdateParameter(CRIATOMEXASR_SURROUNDER_PARAMETER_MODE);
}

void UAtomBusEffectSurrounderWithEQPreset::SetDelayTime(float Time)
{
	DynamicSettings.DelayTime = Time;

	UpdateParameter(CRIATOMEXASR_SURROUNDER_PARAMETER_DELAY_TIME_MS);
}

void UAtomBusEffectSurrounderWithEQPreset::SetGain(float Gain)
{
	DynamicSettings.Gain = Gain;

	UpdateParameter(CRIATOMEXASR_SURROUNDER_PARAMETER_GAIN);
}

void UAtomBusEffectSurrounderWithEQPreset::SetBandFilterType(int BandIndex, EAtomBandEQFilterType Type)
{
	if (BandIndex < 0 || BandIndex > 6)
	{
		return;
	}

	DynamicSettings.GetBandFilter(BandIndex).FilterType = Type;

	UpdateParameter(CRIATOMEXASR_SURROUNDER_NUM_PARAMETERS + CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_TYPE(BandIndex));
}

void UAtomBusEffectSurrounderWithEQPreset::SetBandFrequency(int BandIndex, float Frequency)
{
	if (BandIndex < 0 || BandIndex > 6)
	{
		return;
	}

	DynamicSettings.GetBandFilter(BandIndex).Frequency = Frequency;

	UpdateParameter(CRIATOMEXASR_SURROUNDER_NUM_PARAMETERS + CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_FREQUENCY(BandIndex));
}

void UAtomBusEffectSurrounderWithEQPreset::SetBandQ(int BandIndex, float Factor)
{
	if (BandIndex < 0 || BandIndex > 6)
	{
		return;
	}

	DynamicSettings.GetBandFilter(BandIndex).Q = Factor;

	UpdateParameter(CRIATOMEXASR_SURROUNDER_NUM_PARAMETERS + CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_QUALITY_FACTOR(BandIndex));
}

void UAtomBusEffectSurrounderWithEQPreset::SetBandGain(int BandIndex, float Level)
{
	if (BandIndex < 0 || BandIndex > 6)
	{
		return;
	}

	DynamicSettings.GetBandFilter(BandIndex).Gain = Level;

	UpdateParameter(CRIATOMEXASR_SURROUNDER_NUM_PARAMETERS + CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_GAIN(BandIndex));
}

#undef LOCTEXT_NAMESPACE
