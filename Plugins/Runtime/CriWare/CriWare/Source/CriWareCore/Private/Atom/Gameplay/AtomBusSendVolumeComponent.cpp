
#include "Atom/Gameplay/AtomBusSendVolumeComponent.h"

#include "CriWareCorePrivate.h"
#include "CriWare.h"
#include "Atom/AtomVolume.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/Gameplay/AtomGameplayFlags.h"

#define LOCTEXT_NAMESPACE "AtomBusSendVolumeComponent"

constexpr TCHAR FAtomProxyMutator_BusSend::MutatorBusSendName[];

FAtomProxyMutator_BusSend::FAtomProxyMutator_BusSend()
{
	MutatorName = MutatorBusSendName;
}

void FAtomProxyMutator_BusSend::Apply(FAtomProxyActiveSoundParams& Params) const
{
	check(IsInAtomThread());

	// Determine location state.  Inside if any of the following conditions are met:
	// spatialization disabled on the active sound, we're in the same volume as the listener,
	// or if the active sound's interior settings are still 'default' (bUsingWorldSettings)
	EAtomVolumeLocationState LocationState = EAtomVolumeLocationState::OutsideTheVolume;
	if (Params.bListenerInVolume || !Params.bAllowSpatialization || Params.bUsingWorldSettings)
	{
		LocationState = EAtomVolumeLocationState::InsideTheVolume;
	}

	if (BusSendSettings.Num() > 0)
	{
		for (const FAtomVolumeBusSendSettings& SendSetting : BusSendSettings)
		{
			if (SendSetting.ListenerLocationState == LocationState)
			{
				for (const FAtomSoundToBusSend& SendInfo : SendSetting.BusSends)
				{
					Params.SoundBusSends.Add(SendInfo);
				}
			}
		}
	}
}

UAtomBusSendVolumeComponent::UAtomBusSendVolumeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PayloadType = AtomGameplay::EComponentPayload::AGCP_ActiveSound;
	bAutoActivate = true;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

void UAtomBusSendVolumeComponent::SetBusSendSettings(const TArray<FAtomVolumeBusSendSettings>& NewBusSendSettings)
{
	BusSendSettings = NewBusSendSettings;

	// Let the parent volume know we've changed
	NotifyDataChanged();
}

TSharedPtr<FAtomProxyVolumeMutator> UAtomBusSendVolumeComponent::FactoryMutator() const
{
	return MakeShared<FAtomProxyMutator_BusSend>();
}

void UAtomBusSendVolumeComponent::CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const
{
	TSharedPtr<FAtomProxyMutator_BusSend> SubmixMutator = StaticCastSharedPtr<FAtomProxyMutator_BusSend>(Mutator);
	SubmixMutator->BusSendSettings = BusSendSettings;
}

#undef LOCTEXT_NAMESPACE
