
#include "Atom/Gameplay/AtomGameplayVolumeMutator.h"

#include "CriWareCorePrivate.h"
#include "Atom/Gameplay/AtomGameplayVolume.h"
#include "Atom/Gameplay/AtomGameplayVolumeListener.h"
#include "Atom/Gameplay/AtomGameplayVolumeSubsystem.h"

#define LOCTEXT_NAMESPACE "AtomGameplayVolumeMutator"

FAtomProxyActiveSoundParams::FAtomProxyActiveSoundParams(const FAtomGameplayActiveSoundInfo& SoundInfo, const FAtomGameplayVolumeListener& InListener)
	: SourceInteriorVolume(SoundInfo.SourceInteriorVolume)
	, SourceInteriorLPF(SoundInfo.SourceInteriorLPF)
	, bUsingWorldSettings(SoundInfo.InteriorSettings.IsWorldSettings())
	, Listener(InListener)
	, Sound(SoundInfo)
	, MultiPositionType(EAtomMultiPositionType::SingleSource)
{
}

void FAtomProxyActiveSoundParams::UpdateInteriorValues()
{
	const FAtomInterpolatedInteriorSettings& ListenerInterior = Listener.GetInteriorSettings();

	// When a listener or a sound changes interior settings, we smoothly transition from the old settings
	// to the new settings. We track both the listener and the sound's interpolation progress.
	// We use the interpolation progress from the object that has _most recently_ changed interior settings.
	// This allows moving sounds to smoothly interpolate across interior boundaries after the listener has completed 
	// transitioning to it's current interior settings (otherwise we'd abruptly slam the moving sound's interior settings to match the listener's).
	float ExtVolumeInterp = ListenerInterior.GetExteriorVolumeInterp();
	float IntVolumeInterp = ListenerInterior.GetInteriorVolumeInterp();
	float ExtLowPassInterp = ListenerInterior.GetExteriorLPFInterp();
	float IntLowPassInterp = ListenerInterior.GetInteriorLPFInterp();

	if (Sound.InteriorSettings.GetInteriorStartTime() > ListenerInterior.GetInteriorStartTime())
	{
		ExtVolumeInterp = Sound.InteriorSettings.GetExteriorVolumeInterp();
		IntVolumeInterp = Sound.InteriorSettings.GetInteriorVolumeInterp();
		ExtLowPassInterp = Sound.InteriorSettings.GetExteriorLPFInterp();
		IntLowPassInterp = Sound.InteriorSettings.GetInteriorLPFInterp();
	}

	// Attenuation
	if (bAffectedByAttenuation || !bAllowSpatialization)
	{
		// Sound and listener are inside
		SourceInteriorVolume = FMath::Lerp(SourceInteriorVolume, 1.0f, IntVolumeInterp);
	}
	else
	{
		if (bUsingWorldSettings)
		{
			// Sound is outside, listener is inside
			SourceInteriorVolume = FMath::Lerp(SourceInteriorVolume, ListenerInterior.GetExteriorVolume(), ExtVolumeInterp);
		}
		else
		{
			// Sound is inside, listener is outside - Use the sound's interior volume multiplied with the listeners exterior volume
			float SoundInteriorValue = FMath::Lerp(SourceInteriorVolume, Sound.InteriorSettings.GetInteriorVolume(), IntVolumeInterp);
			float ListenerInteriorValue = FMath::Lerp(SourceInteriorVolume, ListenerInterior.GetExteriorVolume(), ExtVolumeInterp);
			SourceInteriorVolume = SoundInteriorValue * ListenerInteriorValue;
		}
	}

	// Filter
	if (bAffectedByFilter || !bAllowSpatialization)
	{
		// Sound and listener are inside
		SourceInteriorLPF = FMath::Lerp(SourceInteriorLPF, ATOM_MAX_FILTER_FREQUENCY, IntLowPassInterp);
	}
	else
	{
		if (bUsingWorldSettings)
		{
			// Sound is outside, listener is inside
			SourceInteriorLPF = FMath::Lerp(SourceInteriorLPF, ListenerInterior.GetExteriorLPF(), ExtLowPassInterp);
		}
		else
		{
			// Sound is inside, listener is outside - Current interior LPF is the lesser of the Sound and Listener's LPFs
			float SoundLPFValue = FMath::Lerp(SourceInteriorLPF, Sound.InteriorSettings.GetInteriorLPF(), IntLowPassInterp);
			float ListenerLPFValue = FMath::Lerp(SourceInteriorLPF, ListenerInterior.GetExteriorLPF(), ExtLowPassInterp);
			SourceInteriorLPF = FMath::Min(SoundLPFValue, ListenerLPFValue);
		}
	}
}


constexpr TCHAR FAtomProxyVolumeMutator::MutatorBaseName[];

FAtomProxyVolumeMutator::FAtomProxyVolumeMutator()
{
	MutatorName = MutatorBaseName;
}

void FAtomProxyVolumeMutator::UpdatePriority(FAtomProxyMutatorPriorities& Priorities) const
{
	if (!Priorities.bFilterPayload || HasPayloadType(Priorities.PayloadType))
	{
		int32 DefaultValue = INDEX_NONE;
		int32& CurrentPriority = Priorities.PriorityMap.FindOrAdd(MutatorName, DefaultValue);
		if (Priority > CurrentPriority)
		{
			CurrentPriority = Priority;
		}
	}
}

bool FAtomProxyVolumeMutator::CheckPriority(const FAtomProxyMutatorPriorities& Priorities) const
{
	if (!Priorities.bFilterPayload || HasPayloadType(Priorities.PayloadType))
	{
		if (const int32* HighestPriority = Priorities.PriorityMap.Find(MutatorName))
		{
			if (Priority == *HighestPriority)
			{
				return true;
			}
		}
	}

	return false;
}

void FAtomProxyVolumeMutator::Apply(FAtomInteriorSettings& InteriorSettings) const
{
	InteriorSettings.bIsWorldSettings = false;
}

bool FAtomProxyVolumeMutator::HasPayloadType(PayloadFlags InType) const
{
	return (PayloadType & InType) != PayloadFlags::AGCP_None;
}

UAtomGameplayVolumeMutator::UAtomGameplayVolumeMutator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

void UAtomGameplayVolumeMutator::SetPriority(int32 InPriority)
{
	if (Priority != InPriority)
	{
		Priority = InPriority;
		NotifyDataChanged();
	}
}

TSharedPtr<FAtomProxyVolumeMutator> UAtomGameplayVolumeMutator::CreateMutator() const
{
	TSharedPtr<FAtomProxyVolumeMutator> ProxyMutator = FactoryMutator();
	if (ProxyMutator.IsValid())
	{
		CopyAtomDataToMutatorBase(ProxyMutator);
	}

	return ProxyMutator;
}

void UAtomGameplayVolumeMutator::Enable()
{
	Super::Enable();
	NotifyDataChanged();
}

TSharedPtr<FAtomProxyVolumeMutator> UAtomGameplayVolumeMutator::FactoryMutator() const
{
	return TSharedPtr<FAtomProxyVolumeMutator>();
}

void UAtomGameplayVolumeMutator::NotifyDataChanged() const
{
	if (IsActive())
	{
		TInlineComponentArray<UAtomGameplayVolumeComponent*> VolumeComponents(GetOwner());
		for (UAtomGameplayVolumeComponent* VolumeComponent : VolumeComponents)
		{
			if (VolumeComponent != nullptr)
			{
				VolumeComponent->OnComponentDataChanged();
			}
		}
	}
}

void UAtomGameplayVolumeMutator::CopyAtomDataToMutatorBase(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const
{
	check(Mutator.IsValid());
	Mutator->PayloadType = PayloadType;
	Mutator->Priority = Priority;

	CopyAtomDataToMutator(Mutator);
}

#undef LOCTEXT_NAMESPACE
