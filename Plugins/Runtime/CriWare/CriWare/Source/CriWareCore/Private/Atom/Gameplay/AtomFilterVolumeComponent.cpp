
#include "Atom/Gameplay/AtomFilterVolumeComponent.h"

#include "CriWareCorePrivate.h"
#include "Atom/AtomVolume.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/Gameplay/AtomGameplayFlags.h"

#define LOCTEXT_NAMESPACE "AtomFilterVolumeComponent"

constexpr TCHAR FAtomProxyMutator_Filter::MutatorFilterName[];

FAtomProxyMutator_Filter::FAtomProxyMutator_Filter()
{
	MutatorName = MutatorFilterName;
}

void FAtomProxyMutator_Filter::Apply(FAtomInteriorSettings& InteriorSettings) const
{
	FAtomProxyVolumeMutator::Apply(InteriorSettings);

	InteriorSettings.ExteriorLPF = ExteriorLPF;
	InteriorSettings.ExteriorLPFTime = ExteriorLPFTime;
	InteriorSettings.InteriorLPF = InteriorLPF;
	InteriorSettings.InteriorLPFTime = InteriorLPFTime;
}

void FAtomProxyMutator_Filter::Apply(FAtomProxyActiveSoundParams& Params) const
{
	Params.bAffectedByFilter |= Params.bListenerInVolume;
}

UAtomFilterVolumeComponent::UAtomFilterVolumeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PayloadType = AtomGameplay::EComponentPayload::AGCP_ActiveSound | AtomGameplay::EComponentPayload::AGCP_Listener;
	bAutoActivate = true;

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

void UAtomFilterVolumeComponent::SetExteriorLPF(float Volume, float InterpolateTime)
{
	ExteriorLPF = Volume;
	ExteriorLPFTime = InterpolateTime;

	// Let the parent volume know we've changed
	NotifyDataChanged();
}

void UAtomFilterVolumeComponent::SetInteriorLPF(float Volume, float InterpolateTime)
{
	InteriorLPF = Volume;
	InteriorLPFTime = InterpolateTime;

	// Let the parent volume know we've changed
	NotifyDataChanged();
}

TSharedPtr<FAtomProxyVolumeMutator> UAtomFilterVolumeComponent::FactoryMutator() const
{
	return MakeShared<FAtomProxyMutator_Filter>();
}

void UAtomFilterVolumeComponent::CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const
{
	TSharedPtr<FAtomProxyMutator_Filter> FilterMutator = StaticCastSharedPtr<FAtomProxyMutator_Filter>(Mutator);
	FilterMutator->ExteriorLPF = ExteriorLPF;
	FilterMutator->ExteriorLPFTime = ExteriorLPFTime;
	FilterMutator->InteriorLPF = InteriorLPF;
	FilterMutator->InteriorLPFTime = InteriorLPFTime;
}

#undef LOCTEXT_NAMESPACE
