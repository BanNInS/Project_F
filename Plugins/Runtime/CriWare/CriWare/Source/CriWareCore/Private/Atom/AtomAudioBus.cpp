
#include "Atom/AtomAudioBus.h"

#include "Atom/AtomAudioBusSubsystem.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomAudioBus)

FAtomAudioBusProxy::FAtomAudioBusProxy(UAtomAudioBus* InAudioBus)
{
	if (InAudioBus)
	{
		AudioBusID = InAudioBus->GetUniqueID();
		NumChannels = InAudioBus->GetNumChannels();
	}
}

UAtomAudioBus::UAtomAudioBus(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAtomAudioBus::BeginDestroy()
{
	Super::BeginDestroy();

	if (!GCriWare)
	{
		return;
	}

	// Make sure we stop all audio bus instances on all devices if this object is getting destroyed
	uint32 AudioBusId = GetUniqueID();

	FAtomRuntimeManager* AtomRuntimeManager = GCriWare->GetAtomRuntimeManager();
	if (AtomRuntimeManager)
	{
		TArray<FAtomRuntime*> AtomRuntimes = AtomRuntimeManager->GetAtomRuntimes();
		for (FAtomRuntime* AtomRuntime : AtomRuntimes)
		{
			UAtomAudioBusSubsystem* AudioBusSubsystem = AtomRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
			check(AudioBusSubsystem);
			AudioBusSubsystem->StopAudioBus(Atom::FAudioBusKey(AudioBusId));
		}
	}
}

#if WITH_EDITOR
void UAtomAudioBus::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!PropertyChangedEvent.Property)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomAudioBus, AudioBusChannels))
	{
		if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
		{
			RuntimeManager->IterateOverAllRuntimes([BusID = GetUniqueID(), Name = GetPathName(), NumChannels = AudioBusChannels](FAtomRuntimeId, FAtomRuntime* InRuntime)
			{
				UAtomAudioBusSubsystem* AudioBusSubsystem = InRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
				check(AudioBusSubsystem);
				Atom::FAudioBusKey AudioBusKey = Atom::FAudioBusKey(BusID);
				AudioBusSubsystem->StopAudioBus(AudioBusKey);
				AudioBusSubsystem->StartAudioBus(AudioBusKey, Name, (int32)NumChannels + 1, false /* bInIsAutomatic */);
			});
		}
	}
}
#endif // WITH_EDITOR

TSharedPtr<Atom::IProxyData> UAtomAudioBus::CreateProxyData(const Atom::FProxyDataInitParams& InitParams)
{
	return MakeShared<FAtomAudioBusProxy>(this);
}

