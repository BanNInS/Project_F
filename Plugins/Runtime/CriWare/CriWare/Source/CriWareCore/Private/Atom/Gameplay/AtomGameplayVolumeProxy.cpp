
#include "Atom/Gameplay/AtomGameplayVolumeProxy.h"

#include "Components/BrushComponent.h"
#include "Components/PrimitiveComponent.h"

#include "CriWareCorePrivate.h"
#include "CriWareCore.h" 
//#include "AudioAnalytics.h"
#include "Atom/Gameplay/AtomGameplayVolumeMutator.h"
#include "Atom/Gameplay/AtomGameplayVolumeSubsystem.h"
#include "Atom/Gameplay/AtomGameplayVolumeLogs.h"
#include "Atom/Gameplay/AtomGameplayVolumeComponent.h"
#include "Atom/Gameplay/Interfaces/IAtomGameplayCondition.h"

#define LOCTEXT_NAMESPACE "AtomGameplayVolumeProxy"

UAtomGameplayVolumeProxy::UAtomGameplayVolumeProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

bool UAtomGameplayVolumeProxy::ContainsPosition(const FVector& Position) const
{ 
	return false;
}

void UAtomGameplayVolumeProxy::InitFromComponent(const UAtomGameplayVolumeComponent* Component)
{
	if (!Component || !Component->GetWorld())
	{
		UE_LOG(AtomGameplayVolumeLog, Verbose, TEXT("AtomGameplayVolumeProxy - Attempted Init from invalid volume component!"));
		return;
	}

	VolumeID = Component->GetUniqueID();
	WorldID = Component->GetWorld()->GetUniqueID();

	PayloadType = PayloadFlags::AGCP_None;
	ProxyVolumeMutators.Reset();

	TInlineComponentArray<UAtomGameplayVolumeMutator*> Components(Component->GetOwner());
	for (UAtomGameplayVolumeMutator* Comp : Components)
	{
		if (!Comp || !Comp->IsActive())
		{
			continue;
		}

		TSharedPtr<FAtomProxyVolumeMutator> NewMutator = Comp->CreateMutator();
		if (NewMutator.IsValid())
		{
			NewMutator->VolumeID = VolumeID;
			NewMutator->WorldID = WorldID;

			AddPayloadType(NewMutator->PayloadType);
			ProxyVolumeMutators.Emplace(NewMutator);
		}
	}

	//Audio::Analytics::RecordEvent_Usage(TEXT("AudioGameplayVolume.InitializedFromComponent"));
}

void UAtomGameplayVolumeProxy::FindMutatorPriority(FAtomProxyMutatorPriorities& Priorities) const
{
	check(IsInAtomThread());
	for (const TSharedPtr<FAtomProxyVolumeMutator>& ProxyVolumeMutator : ProxyVolumeMutators)
	{
		if (!ProxyVolumeMutator.IsValid())
		{
			continue;
		}

		ProxyVolumeMutator->UpdatePriority(Priorities);
	}
}

void UAtomGameplayVolumeProxy::GatherMutators(const FAtomProxyMutatorPriorities& Priorities, FAtomProxyMutatorSearchResult& OutResult) const
{
	check(IsInAtomThread());
	for (const TSharedPtr<FAtomProxyVolumeMutator>& ProxyVolumeMutator : ProxyVolumeMutators)
	{
		if (!ProxyVolumeMutator.IsValid())
		{
			continue;
		}

		if (ProxyVolumeMutator->CheckPriority(Priorities))
		{
			ProxyVolumeMutator->Apply(OutResult.InteriorSettings);
			OutResult.MatchingMutators.Add(ProxyVolumeMutator);
		}
	}
}

void UAtomGameplayVolumeProxy::AddPayloadType(PayloadFlags InType)
{
	PayloadType |= InType;
}

bool UAtomGameplayVolumeProxy::HasPayloadType(PayloadFlags InType) const
{
	return (PayloadType & InType) != PayloadFlags::AGCP_None;
}

uint32 UAtomGameplayVolumeProxy::GetVolumeID() const
{ 
	return VolumeID;
}

uint32 UAtomGameplayVolumeProxy::GetWorldID() const
{
	return WorldID;
}

UAtomGameplayVolumePrimitiveComponentProxy::UAtomGameplayVolumePrimitiveComponentProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

bool UAtomGameplayVolumePrimitiveComponentProxy::ContainsPosition(const FVector& Position) const
{
	SCOPED_NAMED_EVENT(UAtomGameplayVolumePrimitiveComponentProxy_ContainsPosition, FColor::Blue);

	FBodyInstance* BodyInstancePointer = nullptr;
	if (UPrimitiveComponent* PrimitiveComponent = WeakPrimitive.Get())
	{
		if (PrimitiveComponent->IsPhysicsStateCreated() && PrimitiveComponent->HasValidPhysicsState())
		{
			BodyInstancePointer = PrimitiveComponent->GetBodyInstance();
		}
	}
	
	if (!BodyInstancePointer)
	{
		return false;
	}

	float DistanceSquared = 0.f;
	FVector PointOnBody = FVector::ZeroVector;
	return BodyInstancePointer->GetSquaredDistanceToBody(Position, DistanceSquared, PointOnBody) && FMath::IsNearlyZero(DistanceSquared);
}

void UAtomGameplayVolumePrimitiveComponentProxy::InitFromComponent(const UAtomGameplayVolumeComponent* Component)
{
	Super::InitFromComponent(Component);

	if (Component)
	{
		TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(Component->GetOwner());
		const int32 PrimitiveComponentCount = PrimitiveComponents.Num();

		if (PrimitiveComponentCount != 1)
		{
			UE_LOG(AtomGameplayVolumeLog, Warning, TEXT("Was expecting exactly one Primitive Component on the owning actor, found %d - this could cause unexpected behavior"), PrimitiveComponentCount);
		}

		if (PrimitiveComponents.Num() > 0)
		{
			WeakPrimitive = PrimitiveComponents[0];
		}
	}
}

UAtomGameplayVolumeConditionProxy::UAtomGameplayVolumeConditionProxy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

bool UAtomGameplayVolumeConditionProxy::ContainsPosition(const FVector& Position) const
{
	SCOPED_NAMED_EVENT(UAtomGameplayVolumeConditionProxy_ContainsPosition, FColor::Blue);

	const UObject* ObjectWithInterface = WeakObject.Get();
	if (ObjectWithInterface && ObjectWithInterface->Implements<UAtomGameplayCondition>())
	{
		return IAtomGameplayCondition::Execute_ConditionMet(ObjectWithInterface)
			|| IAtomGameplayCondition::Execute_ConditionMet_Position(ObjectWithInterface, Position);
	}

	return false;
}

void UAtomGameplayVolumeConditionProxy::InitFromComponent(const UAtomGameplayVolumeComponent* Component)
{
	Super::InitFromComponent(Component);

	AActor* OwnerActor = Component ? Component->GetOwner() : nullptr;
	if (OwnerActor)
	{
		if (OwnerActor->Implements<UAtomGameplayCondition>())
		{
			WeakObject = MakeWeakObjectPtr(OwnerActor);
		}
		else
		{
			TInlineComponentArray<UActorComponent*> AllComponents(OwnerActor);

			for (UActorComponent* ActorComponent : AllComponents)
			{
				if (ActorComponent && ActorComponent->Implements<UAtomGameplayCondition>())
				{
					WeakObject = MakeWeakObjectPtr(ActorComponent);
					break;
				}
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
