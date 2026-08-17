#pragma once

#include "CoreMinimal.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomWindowsEndpointSettings.h"

#include "AtomWasapi.h"

// Forward Declarations
class UAtomRackBase;
class ICriWarePlatformAtom;

class FAtomRuntimeWindows
	: public FAtomRuntime
{
public:

	FAtomRuntimeWindows(ICriWarePlatformAtom* InPlatformAtom);
	virtual ~FAtomRuntimeWindows();

	//~ FAtomRuntime interface 
	virtual void InitializePlatformAtomLibrary() override;
	virtual void FinalizePlatformAtomLibrary() override;
	virtual FCriAtomExAsrRackId LoadPlatformAtomRack(const FAtomRackInitParams& InitParams) override;
	virtual bool SetPlatformAtomDSPBusOutput(CriAtomExAsrRackId RackID, const FString& BusName, const FAtomBusOutputParams& OutputParams) override;

	FORCEINLINE virtual TSubclassOf<UAtomEndpointSettingsBase> GetPlatformEndpointSettingsClass() const
	{
		return UAtomWindowsEndpointSettings::StaticClass();
	}

	FORCEINLINE virtual CriAtomSoundRendererType GetPlatformAtomExSoundRendererType(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererTypeForWindows(RendererType);
	}

	FORCEINLINE virtual int32 GetPlatformSoundRendererNumChannels(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererNumChannelsForWindows(RendererType);
	}

	FORCEINLINE virtual int32 GetPlatformSoundRendererNumPorts(EAtomSoundRendererType RendererType) override
	{
		return GetAtomExSoundRendererNumPortsForWindows(RendererType);
	}

public:
	
	//~ Platform specific interface 
	CRIWAREWINDOWS_API TMap<FName, FName>& GetAvailableAudioDevices(bool bRefresh = false) const;
	CRIWAREWINDOWS_API bool AssignAudioDeviceToEndpoint(int HardwareId, const FName& DeviceId);

private:

	TUniquePtr<FAtomWasapi> Wasapi;

private:

	static CriAtomSoundRendererType GetAtomExSoundRendererTypeForWindows(EAtomSoundRendererType RendererType);
	static int32 GetAtomExSoundRendererNumChannelsForWindows(EAtomSoundRendererType RendererType);
	static int32 GetAtomExSoundRendererNumPortsForWindows(EAtomSoundRendererType RendererType);

	static CriAtomSoundRendererType EndpointMappings[(uint32)EAtomSoundRendererType::Num];
};