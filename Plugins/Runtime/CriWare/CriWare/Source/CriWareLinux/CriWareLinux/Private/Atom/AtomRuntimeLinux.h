
#pragma once

#include "CoreMinimal.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"

// Forward Declarations
class UAtomRackBase;
class ICriWarePlatformAtom;

class FAtomRuntimeLinux
	: public FAtomRuntime
{
public:

	FAtomRuntimeLinux(ICriWarePlatformAtom* InPlatformAtom);
	virtual ~FAtomRuntimeLinux();

	//~ Begin FAtomRuntime Interface
	virtual void InitializePlatformAtomLibrary() override;
	virtual void FinalizePlatformAtomLibrary() override;

	FORCEINLINE virtual CriAtomSoundRendererType GetPlatformAtomExSoundRendererType(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererTypeForLinux(RendererType);
	}

	FORCEINLINE virtual int32 GetPlatformSoundRendererNumChannels(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererNumChannelsForLinux(RendererType);
	}

	FORCEINLINE virtual int32 GetPlatformSoundRendererNumPorts(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererNumPortsForLinux(RendererType);
	}

	//~ End FAtomRuntime Interface

private:

	static CriAtomSoundRendererType GetAtomExSoundRendererTypeForLinux(EAtomSoundRendererType RendererType);
	static int32 GetAtomExSoundRendererNumChannelsForLinux(EAtomSoundRendererType RendererType);
	static int32 GetAtomExSoundRendererNumPortsForLinux(EAtomSoundRendererType RendererType);
};