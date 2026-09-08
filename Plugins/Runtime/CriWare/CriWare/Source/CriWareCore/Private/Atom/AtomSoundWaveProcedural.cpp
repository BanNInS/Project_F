/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomSoundWaveProcedural.cpp
 *
 ****************************************************************************/

#include "Atom/AtomSoundWaveProcedural.h"

#include "CriWareCorePrivate.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/AtomSoundBase.h"

#define LOCTEXT_NAMESPACE "AtomSoundWaveProcedural"

 /*
  * UtomSoundWaveProcedural class
  *****************************************************************************/

UAtomSoundWaveProcedural::UAtomSoundWaveProcedural(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bProcedural = true;

	NumSamplesToGeneratePerCallback = ATOM_DEFAULT_PROCEDURAL_SOUNDWAVE_BUFFER_SIZE;
	static_assert(ATOM_DEFAULT_PROCEDURAL_SOUNDWAVE_BUFFER_SIZE >= 512, "Should generate more samples than this per callback.");
	//checkf(NumSamplesToGeneratePerCallback >= NumBufferUnderrunSamples, TEXT("Should generate more samples than this per callback."));

	// If the main audio device has been set up, we can use this to define our callback size.
	// We need to do this for procedural sound waves that we do not process asynchronously,
	// to ensure that we do not underrun.

	if (GCriWare)
	{
		//FAtomRuntime* MainAtomRuntime = GCriWare->GetMainAtomRuntimeRaw();
		//if (MainAtomRuntime)
		//{
		//	NumSamplesToGeneratePerCallback = MainAtomRuntime->GetBufferLength();
		//	NumBufferUnderrunSamples = NumSamplesToGeneratePerCallback / 2;
		//}
	}

	SampleByteSize = 2; // FLOAT32

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

void UAtomSoundWaveProcedural::PostLoad()
{
	// by pass wave post load
	UAtomSoundBase::PostLoad();
}

bool UAtomSoundWaveProcedural::IsPlayable() const
{
	return true;
}

bool UAtomSoundWaveProcedural::IsStreaming(const TCHAR* PlatformName /* = nullptr */) const
{ 
	return false;
}

float UAtomSoundWaveProcedural::GetDuration() const
{
	return 0.0f;
}

IAtomSoundResource* UAtomSoundWaveProcedural::GetSoundResource()
{ 
	return nullptr;
}

void UAtomSoundWaveProcedural::GetAllSoundResources(TArray<IAtomSoundResource*>& Resources)
{
}

#undef LOCTEXT_NAMESPACE
