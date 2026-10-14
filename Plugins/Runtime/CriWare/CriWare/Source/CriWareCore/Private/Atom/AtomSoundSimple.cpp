/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomSoundSimple.cpp
 *
 ****************************************************************************/

#include "Atom/AtomSoundSimple.h"

#include "CriWareCorePrivate.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"

#define LOCTEXT_NAMESPACE "AtomSoundSimple"

UAtomSoundSimple::UAtomSoundSimple(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

void UAtomSoundSimple::CacheValues()
{
	MaxDistance = 0.0f;
	Duration = 0.0f;

	for (int32 i = 0; i < Variations.Num(); ++i)
	{
		UAtomSoundWave* SoundWaveVariation = Variations[i].SoundWave;
		if (SoundWaveVariation)
		{
			float SoundWaveMaxAudibleDistance = SoundWaveVariation->GetMaxDistance();
			if (SoundWaveMaxAudibleDistance > MaxDistance)
			{
				MaxDistance = SoundWaveMaxAudibleDistance;
			}

			float SoundWaveMaxDuration = SoundWaveVariation->GetDuration();
			if (SoundWaveMaxDuration > Duration)
			{
				Duration = SoundWaveMaxDuration;
			}
		}
	}
}

void UAtomSoundSimple::PostLoad()
{
	Super::PostLoad();

	CacheValues();
}

void UAtomSoundSimple::Serialize(FArchive& Ar)
{
	// Always force the duration to be updated when we are saving or cooking
	if (Ar.IsSaving() || Ar.IsCooking())
	{
		CacheValues();
	}

	Super::Serialize(Ar);
}

bool UAtomSoundSimple::IsPlayable() const
{
	return true;
}

void UAtomSoundSimple::Parse(FAtomRuntime* AtomRuntime, const UPTRINT PlaybackInstanceHash, FAtomActiveSound& ActiveSound, const FAtomSoundParseParameters& ParseParams, TArray<FAtomPlaybackInstance*>& PlaybackInstances)
{
	FAtomPlaybackInstance* PlaybackInstance = ActiveSound.FindPlaybackInstance(PlaybackInstanceHash);
	if (!PlaybackInstance)
	{
		ChooseSoundWave();
	}

	// Forward the parse to the chosen sound wave
	check(SoundWave);
	SoundWave->Parse(AtomRuntime, PlaybackInstanceHash, ActiveSound, ParseParams, PlaybackInstances);
}

void UAtomSoundSimple::ChooseSoundWave()
{
	float ProbabilitySum = 0.0f;
	for (int32 i = 0; i < Variations.Num(); ++i)
	{
		if (Variations[i].SoundWave)
		{
			ProbabilitySum += Variations[i].ProbabilityWeight;
		}
	}

	float Choice = FMath::FRandRange(0.0f, ProbabilitySum);

	// Find the index chosen
	ProbabilitySum = 0.0f;
	int32 ChosenIndex = 0;
	for (int32 i = 0; i < Variations.Num(); ++i)
	{
		if (Variations[i].SoundWave)
		{
			float NextSum = ProbabilitySum + Variations[i].ProbabilityWeight;

			if (Choice >= ProbabilitySum && Choice < NextSum)
			{
				ChosenIndex = i;
				break;
			}

			ProbabilitySum = NextSum;
		}
	}

	check(ChosenIndex < Variations.Num());
	FAtomSoundVariation& SoundVariation = Variations[ChosenIndex];

	check(SoundVariation.SoundWave);

	// Now choise the volume and pitch to use based on prob ranges
	float Volume = FMath::FRandRange(SoundVariation.VolumeRange[0], SoundVariation.VolumeRange[1]);
	float Pitch = FMath::FRandRange(SoundVariation.PitchRange[0], SoundVariation.PitchRange[1]);

	// Assign the sound wave value to the transient sound wave ptr
	SoundWave = SoundVariation.SoundWave;
	SoundWave->Volume = Volume;
	SoundWave->Pitch = Pitch;
}

float UAtomSoundSimple::GetMaxDistance() const
{
	return MaxDistance;
}

float UAtomSoundSimple::GetDuration() const
{
	return Duration;
}

#undef LOCTEXT_NAMESPACE