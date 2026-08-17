/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundSimpleFactory.cpp
 *
 ****************************************************************************/

#include "AtomSoundSimpleFactory.h"

#include "CriWareCoreEditorPrivate.h"
#include "Atom/AtomSoundSimple.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

UAtomSoundSimpleFactory::UAtomSoundSimpleFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomSoundSimple::StaticClass();

	bCreateNew = false;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomSoundSimpleFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomSoundSimple* SoundSimple = NewObject<UAtomSoundSimple>(InParent, Name, Flags);

	float Duration = 0.0f;

	// Add the sound waves with default variation entries
	for (int32 i = 0; i < SoundWaves.Num(); ++i)
	{
		FAtomSoundVariation NewVariation;
		NewVariation.SoundWave = SoundWaves[i];

		if (SoundWaves[i]->Duration > Duration)
		{
			Duration = SoundWaves[i]->Duration;
		}

		SoundSimple->Variations.Add(NewVariation);
	}

	// Write out the duration to be the longest duration
	SoundSimple->Duration = Duration;

	//Atom::Analytics::RecordEvent_Usage("SoundUtilities.SimpleSoundCreated");

	return SoundSimple;
}

#undef LOCTEXT_NAMESPACE
