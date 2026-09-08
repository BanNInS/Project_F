#pragma once

#include "CoreMinimal.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"

// Forward Declarations
class UAtomRackBase;

class FAtomRuntimeApple
	: public FAtomRuntime
{
public:

	FAtomRuntimeApple(ICriWarePlatformAtom* InPlatformAtom);
	virtual ~FAtomRuntimeApple();

	//~ FAtomRuntime interface 
	virtual void InitializePlatformAtomLibrary() override;
	virtual void FinalizePlatformAtomLibrary() override;

	FORCEINLINE virtual CriAtomSoundRendererType GetPlatformAtomExSoundRendererType(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererTypeForApple(RendererType);
	}

	FORCEINLINE virtual int32 GetPlatformSoundRendererNumChannels(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererNumChannelsForApple(RendererType);
	}

	FORCEINLINE virtual int32 GetPlatformSoundRendererNumPorts(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererNumPortsForApple(RendererType);
	}

private:

	static CriAtomSoundRendererType GetAtomExSoundRendererTypeForApple(EAtomSoundRendererType RendererType);
	static int32 GetAtomExSoundRendererNumChannelsForApple(EAtomSoundRendererType RendererType);
	static int32 GetAtomExSoundRendererNumPortsForApple(EAtomSoundRendererType RendererType);
};