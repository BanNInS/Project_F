
#include "Atom/Gameplay/AtomPortalVolumeComponent.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/Gameplay/AtomGameplayVolumeSubsystem.h"
#include "Atom/Gameplay/AtomGameplayVolumeProxy.h"

FAtomProxyMutator_Portal::FAtomProxyMutator_Portal()
{
	MutatorName = MutatorPortalName;
}

void FAtomProxyMutator_Portal::Apply(FAtomProxyActiveSoundParams& Params) const
{
	Params.bSoundMovable = Params.bAllowSpatialization;
	if (!Params.bSoundMovable)
	{
		return;
	}

	if (Params.bListenerInVolume)
	{
		FAtomRuntimeId RuntimeID = Params.Listener.GetOwningRuntimeID();
		FAtomRuntimeHandle Runtime = FAtomRuntimeManager::Get()->GetAtomRuntime(RuntimeID);

		if (Runtime.IsValid())
		{
			float UnusedDistSq;
			const int32 ListenerIndex = Runtime->FindClosestListenerIndex(Params.Listener.GetPosition(), UnusedDistSq, true);
			FTransform Transform;
			Runtime->GetListenerTransform(ListenerIndex, Transform);
			const FVector Forward = Transform.GetUnitAxis(EAxis::X);
			Params.Transform.SetLocation(Params.Listener.GetPosition() + Forward);
		}
	}
	else
	{
		FAtomRuntimeId RuntimeID = Params.Listener.GetOwningRuntimeID();
		FAtomRuntimeHandle Runtime = FAtomRuntimeManager::Get()->GetAtomRuntime(RuntimeID);

		if (Runtime.IsValid())
		{
			if (auto AGVS = FAtomRuntime::GetSubsystem<UAtomGameplayVolumeSubsystem>(Runtime))
			{

				const UAtomGameplayVolumeComponent* Volume = AGVS->GetVolumeComponent(VolumeID);
				if (UPrimitiveComponent* Primitive = AGVS->GetPrimitiveComponent(Volume))
				{
					FVector NewActiveSoundPosition;
					Primitive->GetClosestPointOnCollision(Params.Listener.GetPosition(), NewActiveSoundPosition);
					Params.Transform.SetLocation(NewActiveSoundPosition);
				}
			}
		}
	}
}

UAtomPortalVolumeComponent::UAtomPortalVolumeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PayloadType = AtomGameplay::EComponentPayload::AGCP_ActiveSound | AtomGameplay::EComponentPayload::AGCP_Listener;
	bAutoActivate = true;
}

TSharedPtr<FAtomProxyVolumeMutator> UAtomPortalVolumeComponent::FactoryMutator() const
{
	return MakeShared<FAtomProxyMutator_Portal>();
}

void UAtomPortalVolumeComponent::CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const
{
}
