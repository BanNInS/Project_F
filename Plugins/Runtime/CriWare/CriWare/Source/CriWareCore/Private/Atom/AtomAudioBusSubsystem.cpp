
#include "Atom/AtomAudioBusSubsystem.h"

#include "UObject/UObjectIterator.h"

#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/Mixer/AtomMixerSourceManager.h"

std::atomic<uint32> Atom::FAudioBusKey::InstanceIdCounter = 0;

UAtomAudioBusSubsystem::UAtomAudioBusSubsystem()
{
}

bool UAtomAudioBusSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer();
}

void UAtomAudioBusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogCriWareAtom, Log, TEXT("Initializing Atom Audio Bus Subsystem for Atom runtime with ID %d"), GetAtomRuntime()->GetAtomRuntimeID());
	InitDefaultAudioBuses();
}

void UAtomAudioBusSubsystem::Deinitialize()
{
	UE_LOG(LogCriWareAtom, Log, TEXT("Deinitializing Atom Audio Bus Subsystem for Atom runtime with ID %d"), GetAtomRuntime()->GetAtomRuntimeID());
	ShutdownDefaultAudioBuses();
}

void UAtomAudioBusSubsystem::StartAudioBus(Atom::FAudioBusKey InAudioBusKey, int32 InNumChannels, bool bInIsAutomatic)
{
	StartAudioBus(InAudioBusKey, FString(), InNumChannels, bInIsAutomatic);
}

void UAtomAudioBusSubsystem::StartAudioBus(Atom::FAudioBusKey InAudioBusKey, const FString& InAudioBusName, int32 InNumChannels, bool bInIsAutomatic)
{
	if (IsInGameThread())
	{
		if (ActiveAudioBuses_GameThread.Contains(InAudioBusKey))
		{
			return;
		}

		FActiveBusData BusData;
		BusData.BusKey = InAudioBusKey;
		BusData.NumChannels = InNumChannels;
		BusData.bIsAutomatic = bInIsAutomatic;

		ActiveAudioBuses_GameThread.Add(InAudioBusKey, BusData);

		FAtomThread::RunCommandOnAtomThread([this, InAudioBusKey, InAudioBusName, InNumChannels, bInIsAutomatic]()
		{
			if (Atom::FMixerSourceManager* MixerSourceManager = GetMutableSourceManager())
			{
				MixerSourceManager->StartAudioBus(InAudioBusKey, InAudioBusName, InNumChannels, bInIsAutomatic);
			}
		});
	}
	else
	{
		// If we're not the game thread, this needs to be on the game thread, so queue up a command to execute it on the game thread
		GetMutableAtomRuntime()->GameThreadMPSCCommand([this, InAudioBusKey, InAudioBusName, InNumChannels, bInIsAutomatic]
		{
			StartAudioBus(InAudioBusKey, InAudioBusName, InNumChannels, bInIsAutomatic);
		});
	}
}

void UAtomAudioBusSubsystem::StopAudioBus(Atom::FAudioBusKey InAudioBusKey)
{
	if (IsInGameThread())
	{
		if (!ActiveAudioBuses_GameThread.Contains(InAudioBusKey))
		{
			return;
		}

		ActiveAudioBuses_GameThread.Remove(InAudioBusKey);

		FAtomThread::RunCommandOnAtomThread([this, InAudioBusKey]()
		{
			GetMutableSourceManager()->StopAudioBus(InAudioBusKey);
		});
	}
	else
	{
		// If we're not the game thread, this needs to be on the game thread, so queue up a command to execute it on the game thread
		GetMutableAtomRuntime()->GameThreadMPSCCommand([this, InAudioBusKey]
		{
			StopAudioBus(InAudioBusKey);
		});
	}
}

bool UAtomAudioBusSubsystem::IsAudioBusActive(Atom::FAudioBusKey InAudioBusKey) const
{
	if (IsInGameThread())
	{
		return ActiveAudioBuses_GameThread.Contains(InAudioBusKey);
	}

	check(IsInAtomThread());

	if (const Atom::FMixerSourceManager* MixerSourceManager = GetSourceManager())
	{
		return MixerSourceManager->IsAudioBusActive(InAudioBusKey);
	}
	return false;
}

Audio::FPatchInput UAtomAudioBusSubsystem::AddPatchInputForAudioBus(Atom::FAudioBusKey InAudioBusKey, int32 InFrames, int32 InChannels, float InGain)
{
	Atom::FMixerSourceManager* SourceManager = GetMutableSourceManager();
	check(SourceManager);

	FAtomRuntime* AtomRuntime = GetMutableAtomRuntime();
	if (!AtomRuntime)
	{
		return Audio::FPatchInput();
	}

	Audio::FPatchInput PatchInput = AtomRuntime->MakePatch(InFrames, InChannels, InGain);
	SourceManager->AddPendingAudioBusConnection(InAudioBusKey, InChannels, false, PatchInput);
	return PatchInput;
}

Audio::FPatchOutputStrongPtr UAtomAudioBusSubsystem::AddPatchOutputForAudioBus(Atom::FAudioBusKey InAudioBusKey, int32 InFrames, int32 InChannels, float InGain)
{
	Atom::FMixerSourceManager* SourceManager = GetMutableSourceManager();
	check(SourceManager);

	FAtomRuntime* AtomRuntime = GetMutableAtomRuntime();
	if (!AtomRuntime)
	{
		return nullptr;
	}

	Audio::FPatchOutputStrongPtr PatchOutput = AtomRuntime->MakePatch(InFrames, InChannels, InGain);
	SourceManager->AddPendingAudioBusConnection(InAudioBusKey, InChannels, false, PatchOutput);
	return PatchOutput;
}

Audio::FPatchInput UAtomAudioBusSubsystem::AddPatchInputForSoundAndAudioBus(uint64 SoundInstanceID, Atom::FAudioBusKey AudioBusKey, int32 InFrames, int32 NumChannels, float InGain)
{
	FAtomRuntime* AtomRuntime = GetMutableAtomRuntime();
	if (!AtomRuntime)
	{
		return {};
	}

	if (Audio::FPatchOutputStrongPtr PatchOutput = AtomRuntime->MakePatch(InFrames, NumChannels, InGain))
	{
		Audio::FPatchInput PatchInput = MoveTemp(PatchOutput);
		AddPendingConnection(SoundInstanceID, FPendingConnection{ FPendingConnection::FPatchVariant(TInPlaceType<Audio::FPatchInput>(), PatchInput), MoveTemp(AudioBusKey), InFrames, NumChannels });
		return PatchInput;
	}

	return {};
}

Audio::FPatchOutputStrongPtr UAtomAudioBusSubsystem::AddPatchOutputForSoundAndAudioBus(uint64 SoundInstanceID, Atom::FAudioBusKey AudioBusKey, int32 InFrames, int32 NumChannels, float InGain)
{
	FAtomRuntime* AtomRuntime = GetMutableAtomRuntime();
	if (!AtomRuntime)
	{
		return {};
	}

	if (Audio::FPatchOutputStrongPtr PatchOutput = AtomRuntime->MakePatch(InFrames, NumChannels, InGain))
	{
		AddPendingConnection(SoundInstanceID, FPendingConnection{ FPendingConnection::FPatchVariant(TInPlaceType<Audio::FPatchOutputStrongPtr>(), PatchOutput), MoveTemp(AudioBusKey), InFrames, NumChannels });
		return PatchOutput;
	}

	return {};
}

void UAtomAudioBusSubsystem::AddPendingConnection(uint64 SoundInstanceID, FPendingConnection&& PendingConnection)
{
	FScopeLock ScopeLock(&Mutex);
	FSoundInstanceConnections& SoundInstanceConnections = SoundInstanceConnectionMap.FindOrAdd(SoundInstanceID);
	SoundInstanceConnections.PendingConnections.Add(MoveTemp(PendingConnection));
}

void UAtomAudioBusSubsystem::ConnectPatches(uint64 SoundInstanceID)
{
	TArray<FPendingConnection> PendingConnections = ExtractPendingConnectionsIfReady(SoundInstanceID);
	if (!PendingConnections.IsEmpty())
	{
		Atom::FMixerSourceManager* SourceManager = GetMutableSourceManager();
		check(SourceManager);
		for (FPendingConnection& PendingConnection : PendingConnections)
		{
			switch (PendingConnection.PatchVariant.GetIndex())
			{
			case FPendingConnection::FPatchVariant::IndexOfType<Audio::FPatchInput>():
				SourceManager->AddPendingAudioBusConnection(MoveTemp(PendingConnection.AudioBusKey), PendingConnection.NumChannels, PendingConnection.bIsAutomatic, MoveTemp(PendingConnection.PatchVariant.Get<Audio::FPatchInput>()));
				break;
			case FPendingConnection::FPatchVariant::IndexOfType<Audio::FPatchOutputStrongPtr>():
				SourceManager->AddPendingAudioBusConnection(MoveTemp(PendingConnection.AudioBusKey), PendingConnection.NumChannels, PendingConnection.bIsAutomatic, MoveTemp(PendingConnection.PatchVariant.Get<Audio::FPatchOutputStrongPtr>()));
				break;
			}
		}
	}
}

void UAtomAudioBusSubsystem::RemoveSound(uint64 SoundInstanceID)
{
	FScopeLock ScopeLock(&Mutex);
	SoundInstanceConnectionMap.Remove(SoundInstanceID);
}

TArray<UAtomAudioBusSubsystem::FPendingConnection> UAtomAudioBusSubsystem::ExtractPendingConnectionsIfReady(uint64 SoundInstanceID)
{
	FScopeLock ScopeLock(&Mutex);
	if (FSoundInstanceConnections* SoundInstanceConnections = SoundInstanceConnectionMap.Find(SoundInstanceID))
	{
		TArray<FPendingConnection> PendingConnections = MoveTemp(SoundInstanceConnections->PendingConnections);
		SoundInstanceConnections->PendingConnections.Empty();
		return PendingConnections;
	}
	return {};
}

void UAtomAudioBusSubsystem::InitDefaultAudioBuses()
{
	if (!ensure(IsInGameThread()))
	{
		return;
	}

	if (const UCriWareCoreSettings* AtomSettings = GetDefault<UCriWareCoreSettings>())
	{
		TArray<TStrongObjectPtr<UAtomAudioBus>> StaleBuses = DefaultAudioBuses;
		DefaultAudioBuses.Reset();

		for (const FDefaultAtomAudioBusSettings& BusSettings : AtomSettings->DefaultAudioBuses)
		{
			if (UObject* BusObject = BusSettings.AudioBus.TryLoad())
			{
				if (UAtomAudioBus* AudioBus = Cast<UAtomAudioBus>(BusObject))
				{
					const int32 NumChannels = static_cast<int32>(AudioBus->AudioBusChannels) + 1;
					StartAudioBus(Atom::FAudioBusKey(AudioBus->GetUniqueID()), AudioBus->GetPathName(), NumChannels, false /* bInIsAutomatic */);
		
					TStrongObjectPtr<UAtomAudioBus>AddedBus(AudioBus);
					DefaultAudioBuses.AddUnique(AddedBus);
					StaleBuses.Remove(AddedBus);
				}
			}
		}

		for (TStrongObjectPtr<UAtomAudioBus>& Bus : StaleBuses)
		{
			if (Bus.IsValid())
			{
				StopAudioBus(Atom::FAudioBusKey(Bus->GetUniqueID()));
			}
		}
	}
	else
	{
		UE_LOG(LogCriWareAtom, Error, TEXT("Failed to initialize Default Atom Audio Buses. Atom Settings not found."));
	}
}

void UAtomAudioBusSubsystem::ShutdownDefaultAudioBuses()
{
	if (!ensure(IsInGameThread()))
	{
		return;
	}

	for (TObjectIterator<UAtomAudioBus> It; It; ++It)
	{
		UAtomAudioBus* AudioBus = *It;
		if (AudioBus)
		{
			StopAudioBus(Atom::FAudioBusKey(AudioBus->GetUniqueID()));
		}
	}

	DefaultAudioBuses.Reset();
}
