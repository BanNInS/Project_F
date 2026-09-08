#pragma once

#include "CoreMinimal.h" 

#include "CriWareApi.h"

struct IMMDevice;

class FAtomWasapi
{
public:
	FAtomWasapi();

	int RefreshWasapiEndpoints();
	bool SetWasapiEndpoint(CriAtomSoundRendererType SoundRendererType, const FName& DeviceId);
	void UnsetWasapiEndpoint(CriAtomSoundRendererType SoundRendererType);
	void SetWasapiSpatialAudio(CriAtomSoundRendererType SoundRendererType, bool bEnable);

	TMap<FName, FName> WasapiEndpoints; // <DeviceId, DeviceName>

public:
	void HandleNativeOnWasapiEndpoint(IMMDevice* Device);
};
