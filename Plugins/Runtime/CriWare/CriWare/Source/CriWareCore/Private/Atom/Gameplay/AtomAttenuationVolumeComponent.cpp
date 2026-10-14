
#include "Atom/Gameplay/AtomAttenuationVolumeComponent.h"

#include "CriWareCorePrivate.h"
#include "Atom/AtomVolume.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/Gameplay/AtomGameplayFlags.h"

#define LOCTEXT_NAMESPACE "AtomAttenuationVolumeComponent"

constexpr TCHAR FAtomProxyMutator_Attenuation::MutatorAttenuationName[];

FAtomProxyMutator_Attenuation::FAtomProxyMutator_Attenuation()
{
	MutatorName = MutatorAttenuationName;
}

void FAtomProxyMutator_Attenuation::Apply(FAtomInteriorSettings& InteriorSettings) const
{
	FAtomProxyVolumeMutator::Apply(InteriorSettings);

	InteriorSettings.ExteriorVolume = ExteriorVolume;
	InteriorSettings.ExteriorTime = ExteriorTime;
	InteriorSettings.InteriorVolume = InteriorVolume;
	InteriorSettings.InteriorTime = InteriorTime;
}

void FAtomProxyMutator_Attenuation::Apply(FAtomProxyActiveSoundParams& Params) const
{
	Params.bAffectedByAttenuation |= Params.bListenerInVolume;
}

UAtomAttenuationVolumeComponent::UAtomAttenuationVolumeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PayloadType = AtomGameplay::EComponentPayload::AGCP_ActiveSound | AtomGameplay::EComponentPayload::AGCP_Listener;
	bAutoActivate = true;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

void UAtomAttenuationVolumeComponent::SetExteriorVolume(float Volume, float InterpolateTime)
{
	ExteriorVolume = Volume;
	ExteriorTime = InterpolateTime;

	// Let the parent volume know we've changed
	NotifyDataChanged();
}

void UAtomAttenuationVolumeComponent::SetInteriorVolume(float Volume, float InterpolateTime)
{
	InteriorVolume = Volume;
	InteriorTime = InterpolateTime;

	// Let the parent volume know we've changed
	NotifyDataChanged();
}

TSharedPtr<FAtomProxyVolumeMutator> UAtomAttenuationVolumeComponent::FactoryMutator() const
{
	return MakeShared<FAtomProxyMutator_Attenuation>();
}

void UAtomAttenuationVolumeComponent::CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const
{
	TSharedPtr<FAtomProxyMutator_Attenuation> AttenuationMutator = StaticCastSharedPtr<FAtomProxyMutator_Attenuation>(Mutator);
	AttenuationMutator->ExteriorVolume = ExteriorVolume;
	AttenuationMutator->ExteriorTime = ExteriorTime;
	AttenuationMutator->InteriorVolume = InteriorVolume;
	AttenuationMutator->InteriorTime = InteriorTime;
}

#undef LOCTEXT_NAMESPACE
