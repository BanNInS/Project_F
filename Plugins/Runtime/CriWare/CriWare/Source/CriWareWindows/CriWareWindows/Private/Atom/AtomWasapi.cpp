
#include "AtomWasapi.h"

#include "CriWareAtomWindows.h"
#include "Atom/Atom.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"

THIRD_PARTY_INCLUDES_START
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <functiondiscoverykeys.h>
#include <propvarutil.h>
#ifdef CRIWARE_UE_LE
#include <cri_le_atom_wasapi.h>
#else
#include <cri_atom_wasapi.h>
#endif
THIRD_PARTY_INCLUDES_END

// todo: move this to Core
#define ATOM_PLATFORM_ERROR(Err) UE_LOG(LogCriWareAtom, Error, TEXT("Windows: %s"), Err)

// See MSDN documentation for what these error codes mean in the context of the API call
static const TCHAR* GetWasapiError(HRESULT Result)
{
	switch (Result)
	{
	case AUDCLNT_E_NOT_INITIALIZED:					return TEXT("AUDCLNT_E_NOT_INITIALIZED");
	case AUDCLNT_E_ALREADY_INITIALIZED:				return TEXT("AUDCLNT_E_ALREADY_INITIALIZED");
	case AUDCLNT_E_WRONG_ENDPOINT_TYPE:				return TEXT("AUDCLNT_E_WRONG_ENDPOINT_TYPE");
	case AUDCLNT_E_DEVICE_INVALIDATED:				return TEXT("AUDCLNT_E_DEVICE_INVALIDATED");
	case AUDCLNT_E_NOT_STOPPED:						return TEXT("AUDCLNT_E_NOT_STOPPED");
	case AUDCLNT_E_BUFFER_TOO_LARGE:				return TEXT("AUDCLNT_E_BUFFER_TOO_LARGE");
	case AUDCLNT_E_OUT_OF_ORDER:					return TEXT("AUDCLNT_E_OUT_OF_ORDER");
	case AUDCLNT_E_UNSUPPORTED_FORMAT:				return TEXT("AUDCLNT_E_UNSUPPORTED_FORMAT");
	case AUDCLNT_E_INVALID_SIZE:					return TEXT("AUDCLNT_E_INVALID_SIZE");
	case AUDCLNT_E_DEVICE_IN_USE:					return TEXT("AUDCLNT_E_DEVICE_IN_USE");
	case AUDCLNT_E_BUFFER_OPERATION_PENDING:		return TEXT("AUDCLNT_E_BUFFER_OPERATION_PENDING");
	case AUDCLNT_E_THREAD_NOT_REGISTERED:			return TEXT("AUDCLNT_E_THREAD_NOT_REGISTERED");
	case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:		return TEXT("AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED");
	case AUDCLNT_E_ENDPOINT_CREATE_FAILED:			return TEXT("AUDCLNT_E_ENDPOINT_CREATE_FAILED");
	case AUDCLNT_E_SERVICE_NOT_RUNNING:				return TEXT("AUDCLNT_E_SERVICE_NOT_RUNNING");
	case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED:		return TEXT("AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED");
	case AUDCLNT_E_EXCLUSIVE_MODE_ONLY:				return TEXT("AUDCLNT_E_EXCLUSIVE_MODE_ONLY");
	case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:	return TEXT("AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL");
	case AUDCLNT_E_EVENTHANDLE_NOT_SET:				return TEXT("AUDCLNT_E_EVENTHANDLE_NOT_SET");
	case AUDCLNT_E_INCORRECT_BUFFER_SIZE:			return TEXT("AUDCLNT_E_INCORRECT_BUFFER_SIZE");
	case AUDCLNT_E_BUFFER_SIZE_ERROR:				return TEXT("AUDCLNT_E_BUFFER_SIZE_ERROR");
	case AUDCLNT_E_CPUUSAGE_EXCEEDED:				return TEXT("AUDCLNT_E_CPUUSAGE_EXCEEDED");
	case AUDCLNT_E_BUFFER_ERROR:					return TEXT("AUDCLNT_E_BUFFER_ERROR");
	case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:			return TEXT("AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED");
	case AUDCLNT_E_INVALID_DEVICE_PERIOD:			return TEXT("AUDCLNT_E_INVALID_DEVICE_PERIOD");
	case AUDCLNT_E_INVALID_STREAM_FLAG:				return TEXT("AUDCLNT_E_INVALID_STREAM_FLAG");
	case AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE:	return TEXT("AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE");
	case AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES:		return TEXT("AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES");
	case AUDCLNT_E_OFFLOAD_MODE_ONLY:				return TEXT("AUDCLNT_E_OFFLOAD_MODE_ONLY");
	case AUDCLNT_E_NONOFFLOAD_MODE_ONLY:			return TEXT("AUDCLNT_E_NONOFFLOAD_MODE_ONLY");
	case AUDCLNT_E_RESOURCES_INVALIDATED:			return TEXT("AUDCLNT_E_RESOURCES_INVALIDATED");
	case AUDCLNT_E_RAW_MODE_UNSUPPORTED:			return TEXT("AUDCLNT_E_RAW_MODE_UNSUPPORTED");
	case REGDB_E_CLASSNOTREG:						return TEXT("REGDB_E_CLASSNOTREG");
	case CLASS_E_NOAGGREGATION:						return TEXT("CLASS_E_NOAGGREGATION");
	case E_NOINTERFACE:								return TEXT("E_NOINTERFACE");
	case E_POINTER:									return TEXT("E_POINTER");
	case E_INVALIDARG:								return TEXT("E_INVALIDARG");
	case E_OUTOFMEMORY:								return TEXT("E_OUTOFMEMORY");
	default: return TEXT("UKNOWN");
	}
}

#define CLEANUP_ON_FAIL(Result)						\
	if (FAILED(Result))								\
	{												\
		const TCHAR* Err = GetWasapiError(Result);	\
		ATOM_PLATFORM_ERROR(Err);				\
		goto Cleanup;								\
	}

#define RETURN_FALSE_ON_FAIL(Result)				\
	if (FAILED(Result))								\
	{												\
		const TCHAR* Err = GetWasapiError(Result);	\
		ATOM_PLATFORM_ERROR(Err);				\
		return false;								\
	}

/*
 * Criware Native Callbacks
 *****************************************************************************/

namespace FAtomWasapi_NativeCallbacks
{
	extern "C" void OnAudioEndpoint_WASAPI(void* Obj, IMMDevice * Device)
	{
		FAtomWasapi* Wasapi = static_cast<FAtomWasapi*>(Obj);
		if (Wasapi)
		{
			Wasapi->HandleNativeOnWasapiEndpoint(Device);
		}
	}
}

/*
 * FAtomWasapi Implementation
 *****************************************************************************/

FAtomWasapi::FAtomWasapi()
{
	// populate wasapi device table
	RefreshWasapiEndpoints();
}

int FAtomWasapi::RefreshWasapiEndpoints()
{
	return FCriWareApi::criAtom_EnumAudioEndpoints_WASAPI(&FAtomWasapi_NativeCallbacks::OnAudioEndpoint_WASAPI, this);
}

bool FAtomWasapi::SetWasapiEndpoint(CriAtomSoundRendererType SoundRendererType, const FName& DeviceId)
{
	int HardwareIndex = 0;

	if (SoundRendererType & CRIATOM_SOUND_RENDERER_NATIVE)
	{
		HardwareIndex = ((SoundRendererType & ~CRIATOM_SOUND_RENDERER_NATIVE) >> 16) + 1;
	}

	FName* DeviceName = WasapiEndpoints.Find(DeviceId);
	if (DeviceName)
	{
		if (HardwareIndex < 1 || HardwareIndex > 4)
		{
			UE_LOG(LogCriWareAtom, Error, TEXT("'%s' cannot be assigned. Invalid Sound Renderer specified."), *(*DeviceName).ToString());
			return false;
		}

		UE_LOG(LogCriWareAtom, Log, TEXT("'%s' is assigned to Hardware %d."), *(*DeviceName).ToString(), HardwareIndex);
		FCriWareApi::criAtom_SetDeviceId_WASAPI(SoundRendererType, *DeviceId.ToString());
		return true;
	}

	return false;
}

void FAtomWasapi::UnsetWasapiEndpoint(CriAtomSoundRendererType SoundRendererType)
{
	int HardwareIndex = 0;

	if (SoundRendererType & CRIATOM_SOUND_RENDERER_NATIVE)
	{
		HardwareIndex = ((SoundRendererType & ~CRIATOM_SOUND_RENDERER_NATIVE) >> 16) + 1;
	}

	if (HardwareIndex < 1 || HardwareIndex > 4)
	{
		UE_LOG(LogCriWareAtom, Error, TEXT("Hardware cannot be assigned. Invalid Sound Renderer specified."));
		return;
	}

	UE_LOG(LogCriWareAtom, Log, TEXT("Hardware %d is unassigned."), HardwareIndex);
	FCriWareApi::criAtom_SetDeviceId_WASAPI(SoundRendererType, nullptr);
}

void FAtomWasapi::SetWasapiSpatialAudio(CriAtomSoundRendererType SoundRendererType, bool bEnable)
{
	int HardwareIndex = 0;

	if (SoundRendererType & CRIATOM_SOUND_RENDERER_NATIVE)
	{
		HardwareIndex = ((SoundRendererType & ~CRIATOM_SOUND_RENDERER_NATIVE) >> 16) + 1;
	}

	if (HardwareIndex < 1 || HardwareIndex > 4)
	{
		UE_LOG(LogCriWareAtom, Error, TEXT("Spatial audio cannot be changed. Invalid Sound Renderer specified."));
		return;
	}

	FCriWareApi::criAtom_SetSpatialAudioEnabled_WASAPI(SoundRendererType, bEnable ? CRI_TRUE : CRI_FALSE);
}

void FAtomWasapi::HandleNativeOnWasapiEndpoint(IMMDevice* Device)
{
	// Get device store
	IPropertyStore* PropertyStore = nullptr;

	PROPVARIANT DeviceNameProperty;
	PropVariantInit(&DeviceNameProperty);

	LPWSTR DeviceId = nullptr;
	HRESULT Result = S_OK;
	FString FriendlyName;
	FString DeviceIdentifier;

	Result = Device->OpenPropertyStore(STGM_READ, &PropertyStore);
	CLEANUP_ON_FAIL(Result);

	Result = Device->GetId(&DeviceId);
	CLEANUP_ON_FAIL(Result);

	Result = PropertyStore->GetValue(PKEY_Device_FriendlyName, &DeviceNameProperty);
	CLEANUP_ON_FAIL(Result);

	FriendlyName = FString(DeviceNameProperty.pwszVal);
	DeviceIdentifier = FString(DeviceId);
	UE_LOG(LogCriWareAtom, Display, TEXT("Audio Device '%s':id(%s) found."), *FriendlyName, *DeviceIdentifier);

	WasapiEndpoints.Add(FName(DeviceIdentifier), FName(FriendlyName));

Cleanup:
	// Release all our resources that we used above
	SAFE_RELEASE(PropertyStore);

	PropVariantClear(&DeviceNameProperty);

	CoTaskMemFree(DeviceId);
}

#include "Windows/HideWindowsPlatformTypes.h"
#include "Windows/HideWindowsPlatformAtomics.h"
