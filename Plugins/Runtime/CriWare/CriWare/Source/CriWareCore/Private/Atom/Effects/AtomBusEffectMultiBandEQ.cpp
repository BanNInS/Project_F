/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffectMultiBandEQ.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffectMultiBandEQ.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffectMultiBandEQ"

 /*
  * FAtomBusEffectMultiBandEQSettings Implementation
  *****************************************************************************/

FAtomBusEffectMultiBandEQSettings::FAtomBusEffectMultiBandEQSettings()
	: bBypass(false)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

/*
 * UAtomBusEffectMultiBandEQPreset Implementation
 *****************************************************************************/

UAtomBusEffectMultiBandEQPreset::UAtomBusEffectMultiBandEQPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

void UAtomBusEffectMultiBandEQPreset::PostInitProperties()
{
	Super::PostInitProperties();

	if (GetClass() != UAtomBusEffectMultiBandEQPreset::StaticClass())
	{
		const int NumBands = GetEqualizerNumBands();
		for (int BandIndex = 0; BandIndex < NumBands; BandIndex++)
		{
			const EAtomBandEQFilterType BandFilterType = BandIndex == 0 ? EAtomBandEQFilterType::LowShelf : BandIndex == (NumBands - 1) ? EAtomBandEQFilterType::HighShelf : EAtomBandEQFilterType::Peaking;
			const float BandFrequency = FMath::Floor(Atom::GetLogFrequencyClamped((1.0f / (NumBands + 1)) * (BandIndex + 1), { 0.0f, 1.0f }, { ATOM_MIN_FILTER_FREQUENCY, ATOM_MAX_FILTER_FREQUENCY }));
			Settings.BandFilters.Add({ BandFilterType, BandFrequency, 1.0f, 0.0f });
			DynamicSettings.BandFilters.Add({ BandFilterType, BandFrequency, 1.0f, 0.0f });
		}

		//BandFilters.Add({ EAtomBandEQFilterType::LowShelf, 52.0f, 1.0f, 0.0f });
		//BandFilters.Add({ EAtomBandEQFilterType::Peaking, 940.0f, 1.0f, 0.0f });
		//BandFilters.Add({ EAtomBandEQFilterType::HighShelf, 9800.0f, 1.0f, 0.0f });
	}
}

float UAtomBusEffectMultiBandEQPreset::GetParameterValue(int ParameterIndex) const
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

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

	return 0.0f;
}

void UAtomBusEffectMultiBandEQPreset::SetParameterValue(int ParameterIndex, float Value)
{
	check(ParameterIndex >= 0);
	check(ParameterIndex < NumParameters);

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

#if WITH_EDITOR
void UAtomBusEffectMultiBandEQPreset::PreEditChange(FProperty* PropertyAboutToChange)
{
	Super::PreEditChange(PropertyAboutToChange);
}

void UAtomBusEffectMultiBandEQPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomBusEffectMultiBandEQPreset, Settings))
	{
		SetSettings(Settings);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomBusEffectMultiBandEQPreset::SetSettings(const FAtomBusEffectMultiBandEQSettings& InSettings)
{
	UpdateSettings(InSettings);
}

void UAtomBusEffectMultiBandEQPreset::SetDefaultSettings(const FAtomBusEffectMultiBandEQSettings& InSettings)
{
	Settings = InSettings;
	UpdateSettings(InSettings);

	MarkPackageDirty();
}

void UAtomBusEffectMultiBandEQPreset::SetBandFilterType(int BandIndex, EAtomBandEQFilterType Type)
{
	if (BandIndex < 0 || BandIndex >= GetEqualizerNumBands())
	{
		return;
	}

	DynamicSettings.GetBandFilter(BandIndex).FilterType = Type;

	UpdateParameter(CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_TYPE(BandIndex));
}

void UAtomBusEffectMultiBandEQPreset::SetBandFrequency(int BandIndex, float Frequency)
{
	if (BandIndex < 0 || BandIndex >= GetEqualizerNumBands())
	{
		return;
	}

	DynamicSettings.GetBandFilter(BandIndex).Frequency = Frequency;

	UpdateParameter(CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_FREQUENCY(BandIndex));
}

void UAtomBusEffectMultiBandEQPreset::SetBandQ(int BandIndex, float Factor)
{
	if (BandIndex < 0 || BandIndex >= GetEqualizerNumBands())
	{
		return;
	}

	DynamicSettings.GetBandFilter(BandIndex).Q = Factor;

	UpdateParameter(CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_QUALITY_FACTOR(BandIndex));
}

void UAtomBusEffectMultiBandEQPreset::SetBandGain(int BandIndex, float Level)
{
	if (BandIndex < 0 || BandIndex >= GetEqualizerNumBands())
	{
		return;
	}

	DynamicSettings.GetBandFilter(BandIndex).Gain = Level;

	UpdateParameter(CRIATOMEXASR_MULTIBANDS_EQ_PARAMETER_GAIN(BandIndex));
}

#undef LOCTEXT_NAMESPACE
