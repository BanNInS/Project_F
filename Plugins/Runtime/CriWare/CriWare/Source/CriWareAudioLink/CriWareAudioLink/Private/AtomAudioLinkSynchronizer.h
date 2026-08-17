#pragma once

#include "IAudioLink.h"
#include "Misc/ScopeRWLock.h"
#include "Templates/SharedPointer.h"

#include "AudioMixer.h"
#include "AudioDevice.h"

// Forward Definitions
class FAtomRuntimeHandle;
class FAtomRuntimeManager;

struct FAtomAudioLinkSynchronizer 
	: IAudioLinkSynchronizer
	, TSharedFromThis<FAtomAudioLinkSynchronizer, ESPMode::ThreadSafe>
{
public:

	// AudioLink Delegates
	IAudioLinkSynchronizer::FOnSuspend      OnSuspend;
	IAudioLinkSynchronizer::FOnResume       OnResume;
	IAudioLinkSynchronizer::FOnOpenStream   OnOpenStream;
	IAudioLinkSynchronizer::FOnCloseStream  OnCloseStream;
	IAudioLinkSynchronizer::FOnBeginRender  OnBeginRender;
	IAudioLinkSynchronizer::FOnEndRender    OnEndRender;

	// Lock object
	FRWLock RwLock;
	FCriticalSection CallbackLock;
	FAtomRuntimeManager* RuntimeManager;
	bool bIsBound = false;
	unsigned int DspBufferSize;
	FDelegateHandle OnAtomRuntimeActivatedHandle;
	FDelegateHandle OnAtomRuntimeDeactivatedHandle;

	FAtomAudioLinkSynchronizer();
	~FAtomAudioLinkSynchronizer() override;

	void ExecuteOpenStream(FAtomRuntimeHandle& AtomRuntime);
	void ExecuteCloseStream();
	void ExecuteSuspend();
	void ExecuteResume();
	//void ExecuteBeginRender(FAtomRuntimeHandle& AtomRuntime);
	void ExecuteEndRender(FAtomRuntimeHandle& AtomRuntime);
	void BindAtomRuntime();
	void UnbindAtomRuntime();

	// test
	/*void Process()
	{
		FAudioDeviceHandle AudioDevice = GEngine->GetActiveAudioDevice();

		if (AudioDevice->bAudioMixerModuleLoaded)
		{
			
		}

		if (AudioDevice->GetNumBuffers() > 0)
		{
			
		}

		if (AudioDevice->GetBufferLength() > 0)
		{
			
		}
	}*/

#define MAKE_DELEGATE_FUNC(X)\
    FDelegateHandle Register##X##Delegate(const FOn##X::FDelegate& InDelegate) override\
    {\
        FWriteScopeLock WriteLock(RwLock);\
        return On##X.Add(InDelegate);\
    }\
    bool Remove##X##Delegate(const FDelegateHandle& InHandle) override\
    {\
        FWriteScopeLock WriteLock(RwLock);\
        return On##X.Remove(InHandle);\
    }

	MAKE_DELEGATE_FUNC(Suspend)
	MAKE_DELEGATE_FUNC(Resume)
	MAKE_DELEGATE_FUNC(OpenStream)
	MAKE_DELEGATE_FUNC(CloseStream)
	MAKE_DELEGATE_FUNC(BeginRender)
	MAKE_DELEGATE_FUNC(EndRender)

#undef MAKE_DELEGATE_FUNC
};

using FAtomAudioLinkSynchronizerPtr = TSharedPtr<FAtomAudioLinkSynchronizer>;
