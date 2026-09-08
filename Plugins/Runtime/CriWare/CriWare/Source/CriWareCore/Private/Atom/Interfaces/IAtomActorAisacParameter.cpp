
#include "Atom/Interfaces/IAtomActorAisacParameter.h"

#include "HAL/IConsoleManager.h"
#include "GameFramework/Actor.h"

#include "Atom/AtomAisacPatch.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(IAtomActorAisacParameter)

namespace AtomActorAisacParameterConsoleVariables
{
	bool bGatherImplementers = false;
	FAutoConsoleVariableRef CVarGatherImplementers(
		TEXT("atom.ActorAisacParameter.GatherImplementers"),
		bGatherImplementers,
		TEXT("When true, allows the interface to search for attached components and actors that implement the interface."),
		ECVF_Default);
} // namespace

void UAtomActorAisacParameter::Fill(const AActor* OwningActor, TArray<FAtomAisacParameter>& OutParams)
{
	QUICK_SCOPE_CYCLE_COUNTER(UAtomActorAisacParameter_Fill);

	TArray<const AActor*> Actors;
	TArray<const UActorComponent*> Components;

	if (AtomActorAisacParameterConsoleVariables::bGatherImplementers)
	{
		// This is prohibitively expensive, as it goes through our owning actor and all attached actors, looking for 
		// components that implement the IAtomActorAisacParameter
		GetImplementers(OwningActor, Actors, Components);
	}
	else
	{
		// Prior to the GetImplementers change, we only considered the owning actor, and no attached actors or components.
		if (OwningActor && OwningActor->Implements<UAtomActorAisacParameter>())
		{
			Actors.Add(OwningActor);
		}
	}

	TArray<FAtomAisacParameter> TempParams;

	for (const AActor* Actor : Actors)
	{
		IAtomActorAisacParameter::Execute_GetActorAisacParams(Actor, TempParams);
		OutParams.Append(MoveTemp(TempParams));
	}

	for (const UActorComponent* Component : Components)
	{
		IAtomActorAisacParameter::Execute_GetActorAisacParams(Component, TempParams);
		OutParams.Append(MoveTemp(TempParams));
	}
}

void UAtomActorAisacParameter::GetImplementers(const AActor* InActor, TArray<const AActor*>& OutActors, TArray<const UActorComponent*>& OutComponents)
{
	QUICK_SCOPE_CYCLE_COUNTER(UAtomActorAisacParameter_GetImplementers);

	if (!InActor)
	{
		return;
	}

	// Helper to collect objects that implement this interface from an actor (and its components)
	auto CollectFromActor = [&OutActors, &OutComponents](const AActor* InActor)
	{
		if (InActor)
		{
			if (InActor->Implements<UAtomActorAisacParameter>())
			{
				OutActors.Add(InActor);
			}

			TArray<UActorComponent*> Components = InActor->GetComponentsByInterface(UAtomActorAisacParameter::StaticClass());
			OutComponents.Append(Components);
		}
	};

	// Collect Actors/Components that implement this interface
	const AActor* RootActor = InActor;
	if (USceneComponent* RootComp = RootActor->GetRootComponent())
	{
		// Walk up to the top-most attach actor in the hierarchy (will just be the RootActor if no attachment)
		RootActor = RootComp->GetAttachmentRootActor();
	}

	CollectFromActor(RootActor);

	// Grab all attached actors (recursive)
	TArray<AActor*> AttachedActors;
	constexpr bool bResetArray = false;
	constexpr bool bRecursivelyIncludeAttachedActors = true;
	RootActor->GetAttachedActors(AttachedActors, bResetArray, bRecursivelyIncludeAttachedActors);

	for (const AActor* Actor : AttachedActors)
	{
		CollectFromActor(Actor);
	}
}

