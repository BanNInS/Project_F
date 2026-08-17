
#include "SoundXRAtomRuntimePlugin.h"

#include "CriWareSoundXRApi.h"
#include "CriWareCoreSettings.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"

#include "CriWareSoundXRPrivate.h"

/*
 * FSoundXRAtomRuntimePlugin Implementation
 *****************************************************************************/

bool FSoundXRAtomRuntimePlugin::bIsRegistered = false;

FSoundXRAtomRuntimePlugin::FSoundXRAtomRuntimePlugin(FAtomRuntime* OwningAtomRuntime)
	: IAtomRuntimePlugin()
	, AtomRuntime(OwningAtomRuntime)
{
	OnRuntimeWillActivateHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeWillActivate.AddRaw(this, &FSoundXRAtomRuntimePlugin::OnRuntimeWillActivate);
	OnRuntimeDeactivatedHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeDeactivated.AddRaw(this, &FSoundXRAtomRuntimePlugin::OnRuntimeDeactivated);
}

// this is called before atom SDK is initialized
void FSoundXRAtomRuntimePlugin::OnRuntimeWillActivate(FAtomRuntimeId RuntimeID)
{
	if (AtomRuntime && RuntimeID == AtomRuntime->GetAtomRuntimeID())
	{
		bool bIsSoundXREnabled = false;

		const UAtomConfig* AtomConfig = AtomRuntime->GetAtomConfiguration();
		if (!AtomConfig)
		{
			// try from settings
			auto AtomSettings = GetDefault<UCriWareCoreSettings>();
			AtomConfig = Cast<UAtomConfig>(AtomSettings->AtomConfig.TryLoad());
		}
		
		if (AtomConfig)
		{
			bIsSoundXREnabled = AtomConfig->GetBinauralizerName() == TEXT("2"); // Sound xR is actually "2"
		}

		if (bIsSoundXREnabled)
		{
#if CRIWARE_USE_SOUNDXR
			const void* SoundXRInterface = FCriWareSoundXRApi::criSoundxR_GetInterface();
			if (SoundXRInterface)
			{
				if (FCriWareApi::criAtomExAsr_RegisterSoundxRInterface(SoundXRInterface) == CRI_TRUE)
				{
					bIsRegistered = true;

					UE_LOG(LogCriWareSoundXR, Display, TEXT("Initialize Sound xR for ADX."));
					return;
				}
			}
#endif

			UE_LOG(LogCriWareSoundXR, Display, TEXT("Failed to initialize Sound xR for ADX."));
		}

		UE_LOG(LogCriWareSoundXR, Display, TEXT("Sound xR for ADX is not initialized because current AtomConfig is not supporting it or is not set."));
	}
}

// this is called after atom SDK is finalized
void FSoundXRAtomRuntimePlugin::OnRuntimeDeactivated(FAtomRuntimeId RuntimeID)
{
	if (AtomRuntime && RuntimeID == AtomRuntime->GetAtomRuntimeID())
	{
#if CRIWARE_USE_SOUNDXR
		if (bIsRegistered)
		{
			FCriWareApi::criAtomExAsr_RegisterSoundxRInterface(nullptr);
			bIsRegistered = false;

			UE_LOG(LogCriWareSoundXR, Display, TEXT("Uninitialize Sound xR for ADX."));
		}
#endif
	}
}

FSoundXRAtomRuntimePlugin::~FSoundXRAtomRuntimePlugin()
{
	FAtomRuntimeManagerDelegates::OnAtomRuntimeWillActivate.Remove(OnRuntimeWillActivateHandle);
	FAtomRuntimeManagerDelegates::OnAtomRuntimeDeactivated.Remove(OnRuntimeDeactivatedHandle);

	AtomRuntime = nullptr;
}
