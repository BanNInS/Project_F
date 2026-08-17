/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomSoundWaveProcedural.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ScriptMacros.h"
#include "Runtime/Launch/Resources/Version.h"

#include "AtomSoundBase.h"
#include "AtomSoundGenerator.h"

#include "AtomSoundWaveProcedural.generated.h"

#define ATOM_DEFAULT_PROCEDURAL_SOUNDWAVE_BUFFER_SIZE 1024

UCLASS()
class CRIWARECORE_API UAtomSoundWaveProcedural
	: public UAtomSoundBase
{
	GENERATED_BODY()

public:

	UAtomSoundWaveProcedural(const FObjectInitializer& ObjectInitializer);

	/** Creates a sound generator instance from this sound base. Return true if this is being implemented by a subclass. Sound generators procedurally generate audio in the Atom render thread. */
	virtual IAtomSoundGeneratorPtr CreateSoundGenerator(const FAtomSoundGeneratorInitParams& InParams) { return nullptr; }

	/** Called when the procedural sound wave is done generating on the render thread.Only used in the Atom mixer and when bProcedural is true. */
	virtual void OnEndGenerate(IAtomSoundGeneratorPtr Generator) {};

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	//virtual void Serialize(FArchive& Ar) override;
    //#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
    //virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
    //UE_DEPRECATED(5.4, "Implement the version that takes FAssetRegistryTagsContext instead.")
    //#endif
    // virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	//~ End UObject Interface

	//~ Begin UAtomSoundBase interface.
	virtual bool IsPlayable() const override;
	virtual bool IsStreaming(const TCHAR* PlatformName = nullptr) const override;
	virtual float GetDuration() const override;
	virtual IAtomSoundResource* GetSoundResource() override;
	virtual void GetAllSoundResources(TArray<IAtomSoundResource*>& Resources) override;
	//~ End UAtomSoundBase interface.

protected:

	// Number of samples to pad with 0 if there isn't enough audio available
	int32 NumBufferUnderrunSamples;

	// The number of PCM samples we want to generate. This can't be larger than SamplesNeeded in GeneratePCMData callback, but can be less.
	int32 NumSamplesToGeneratePerCallback;

	/** Size in bytes of a single sample of audio in the procedural audio buffer. */
	int32 SampleByteSize;
};
