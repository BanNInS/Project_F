#pragma once

#include "CoreMinimal.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"

// Forward Declarations
class UAtomRackBase;
class ICriWarePlatformAtom;

class FAtomRuntimeUnrealAudio
	: public FAtomRuntime
{
public:

	FAtomRuntimeUnrealAudio(ICriWarePlatformAtom* InPlatformAtom);
	virtual ~FAtomRuntimeUnrealAudio();

	//~ FAtomRuntime interface 
	virtual void InitializePlatformAtomLibrary() override;
	virtual void FinalizePlatformAtomLibrary() override;
	virtual FCriAtomExAsrRackId LoadPlatformAtomRack(const FAtomRackInitParams& InitParams) override;

	FORCEINLINE virtual CriAtomSoundRendererType GetPlatformAtomExSoundRendererType(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererTypeForUnrealAudio(RendererType);
	}

	FORCEINLINE virtual int32 GetPlatformSoundRendererNumChannels(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererNumChannelsForUnrealAudio(RendererType);
	}

	FORCEINLINE virtual int32 GetPlatformSoundRendererNumPorts(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererNumPortsForUnrealAudio(RendererType);
	}

	FORCEINLINE virtual CriAtomSoundRendererType GetPlatformSoundfieldAtomExSoundRendererType(EAtomSoundfieldRendererType SoundfieldRendererType)
	{
		return GetSoundfieldAtomExSoundRendererType(EAtomSoundfieldRendererType::Default);
	}

	FORCEINLINE virtual int32 GetPlatformSoundfieldRendererNumChannels(EAtomSoundfieldRendererType SoundfieldRendererType)
	{
		return GetSoundfieldAtomExSoundRendererNumChannels(EAtomSoundfieldRendererType::Default);
	}

	FORCEINLINE virtual CriAtomSpeakerMapping GetPlatformSoundfieldRendererSpeakerMapping(EAtomSoundfieldRendererType SoundfieldRendererType)
	{
		return GetSoundfieldAtomExSoundRendererSpeakerMapping(EAtomSoundfieldRendererType::Default);
	}

	virtual bool IsUsingUnrealSoundRenderer() const override { return true; }

private:

	static CriAtomSoundRendererType GetAtomExSoundRendererTypeForUnrealAudio(EAtomSoundRendererType RendererType);
	static int32 GetAtomExSoundRendererNumChannelsForUnrealAudio(EAtomSoundRendererType RendererType);
	static int32 GetAtomExSoundRendererNumPortsForUnrealAudio(EAtomSoundRendererType RendererType);
};