#pragma once

#include "CoreMinimal.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"

// Forward Declarations
class UAtomRackBase;
class ICriWarePlatformAtom;

class FAtomRuntimeAndroid
	: public FAtomRuntime
{
public:

	FAtomRuntimeAndroid(ICriWarePlatformAtom* InPlatformAtom);
	virtual ~FAtomRuntimeAndroid();

	virtual void SuspendContext() override;
	virtual void ResumeContext() override;

	//~ FAtomRuntime interface 
	virtual void InitializePlatformAtomLibrary() override;
	virtual void FinalizePlatformAtomLibrary() override;

	FORCEINLINE virtual CriAtomSoundRendererType GetPlatformAtomExSoundRendererType(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererTypeForAndroid(RendererType);
	}

	FORCEINLINE virtual int32 GetPlatformSoundRendererNumChannels(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererNumChannelsForAndroid(RendererType);
	}

	FORCEINLINE virtual int32 GetPlatformSoundRendererNumPorts(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererNumPortsForAndroid(RendererType);
	}

private:

	static CriAtomSoundRendererType GetAtomExSoundRendererTypeForAndroid(EAtomSoundRendererType RendererType);
	static int32 GetAtomExSoundRendererNumChannelsForAndroid(EAtomSoundRendererType RendererType);
	static int32 GetAtomExSoundRendererNumPortsForAndroid(EAtomSoundRendererType RendererType);

	FCriticalSection SuspendedCriticalSection;
	bool bSuspended;
};