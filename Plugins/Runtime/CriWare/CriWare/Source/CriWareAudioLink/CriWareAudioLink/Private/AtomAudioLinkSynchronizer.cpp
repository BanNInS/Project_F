
#include "AtomAudioLinkSynchronizer.h"

#include "CriWare.h"
#include "CriWareCoreModule.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRack.h"

#include "AtomAudioLink.h"

FAtomAudioLinkSynchronizer::FAtomAudioLinkSynchronizer()
{
	BindAtomRuntime();
}

FAtomAudioLinkSynchronizer::~FAtomAudioLinkSynchronizer()
{
	UnbindAtomRuntime();
}

void FAtomAudioLinkSynchronizer::ExecuteEndRender(FAtomRuntimeHandle& AtomRuntime)
{
	FScopeLock Lock(&CallbackLock);
	FOnRenderParams Params;
	Params.BufferTickID = 0; // -> dspclockid ?
	Params.NumFrames = DspBufferSize;
	OnEndRender.Broadcast(Params);
}

void FAtomAudioLinkSynchronizer::ExecuteOpenStream(FAtomRuntimeHandle& AtomRuntime)
{
	UE_LOG(LogAtomAudioLink, Verbose, TEXT("FAtomAudioLinkSynchronizer::ExecuteOpenStream: Opening stream between Unreal and Atom."));

	if (!AtomRuntime.IsValid())
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkSynchronizer::ExecuteOpenStream: Atom Runtime is not initialized."));
		return;
	}

	UAtomRack* MasterRack = AtomRuntime->GetMasterRack();
	check(MasterRack);

	FOnOpenStreamParams Params;
	Params.Name = TEXT("Atom AudioLink Stream");
	Params.NumChannels = MasterRack->NumChannels;
	Params.SampleRate = MasterRack->SamplingRate;
	Params.NumFrames = DspBufferSize;

	OnOpenStream.Broadcast(Params);
}

void FAtomAudioLinkSynchronizer::ExecuteCloseStream()
{
	UE_LOG(LogAtomAudioLink, Verbose, TEXT("FAtomAudioLinkSynchronizer::ExecuteCloseStream: Closing stream between Unreal and Atom."));

	OnCloseStream.Broadcast();
}

void FAtomAudioLinkSynchronizer::ExecuteSuspend()
{
	OnSuspend.Broadcast();
}

void FAtomAudioLinkSynchronizer::ExecuteResume()
{
	OnResume.Broadcast();
}

void FAtomAudioLinkSynchronizer::BindAtomRuntime()
{
	if (UNLIKELY(bIsBound))
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkSynchronizer::Bind: Runtime is already bound."));
		return;
	}

	if (UNLIKELY(ICriWareCoreModule::IsAvailable()))
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkSynchronizer::Bind: CriWareCoreModule not available."));
		return;
	}

	auto CoreSystem = ICriWareCoreModule::Get().GetCriWareCore();
	if (UNLIKELY(!CoreSystem.IsValid()))
	{
		UE_LOG(LogAtomAudioLink, Error, TEXT("FAtomAudioLinkSynchronizer::ExecuteOpenStream: No Criware Core."));
		return;
	}

	RuntimeManager = CoreSystem->GetAtomRuntimeManager();

	UE_LOG(LogAtomAudioLink, Verbose, TEXT("FAtomAudioLinkSynchronizer::Bind: Binding System Callbacks."));
	//UE_LOG(LogAtomAudioLink, Verbose, TEXT("Attaching Atom runtime to AudioLink mixer."));

	// if runtime exist
	if (RuntimeManager)
	{
		FAtomRuntimeHandle Runtime = RuntimeManager->GetMainAtomRuntimeHandle();
		
		if (Runtime.IsValid())
		{
			ExecuteOpenStream(Runtime);
		}
	}
	else
	{
		// attach delegates when runtime are activated or removed
		OnAtomRuntimeActivatedHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeActivated.AddLambda([WeakThis = AsWeak()](FAtomRuntimeId RuntimeID)
		{
			auto ThisSync = WeakThis.Pin();
			if (!ThisSync.IsValid() || !ThisSync->bIsBound)
			{
				return;
			}

			auto Runtime = FAtomRuntimeManager::Get()->GetAtomRuntime(RuntimeID);
			ThisSync->ExecuteOpenStream(Runtime);
		});
	}

	OnAtomRuntimeDeactivatedHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeDeactivated.AddLambda([WeakThis = AsWeak()](FAtomRuntimeId RuntimeID)
	{
		auto ThisSync = WeakThis.Pin();
		if (!ThisSync.IsValid() || !ThisSync->bIsBound)
		{
			return;
		}

		auto Runtime = FAtomRuntimeManager::Get()->GetAtomRuntime(RuntimeID);
		ThisSync->ExecuteEndRender(Runtime);
	});

	bIsBound = true;
}

void FAtomAudioLinkSynchronizer::UnbindAtomRuntime()
{
	if (UNLIKELY(!bIsBound))
	{
		return;
	}

	if (UNLIKELY(!RuntimeManager))
	{
		return;
	}

	UE_LOG(LogAtomAudioLink, Verbose, TEXT("Detaching Atom runtime from AudioLink mixer."));
	FAtomRuntimeManagerDelegates::OnAtomRuntimeActivated.Remove(OnAtomRuntimeActivatedHandle);
	FAtomRuntimeManagerDelegates::OnAtomRuntimeDeactivated.Remove(OnAtomRuntimeDeactivatedHandle);
	bIsBound = false;

	ExecuteCloseStream();
}
