
#include "Atom/Gameplay/AtomRoomVolumeComponent.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/Gameplay/AtomGameplayVolumeSubsystem.h"
#include "Atom/Gameplay/AtomGameplayVolumeProxy.h"
#include "Atom/Gameplay/AtomPortalVolumeComponent.h"

FAtomProxyMutator_Room::FAtomProxyMutator_Room()
{
	MutatorName = MutatorRoomName;
}

void FAtomProxyMutator_Room::Apply(FAtomProxyActiveSoundParams& Params) const
{
	Params.bSoundMovable = Params.bAllowSpatialization;
	if (!Params.bSoundMovable)
	{
		return;
	}

	if (Params.bListenerInVolume)
	{
		// when with listener, sound acts normally in the room
		
		//UE_LOG(LogTemp, Display, TEXT("IN VOLUME %s"), *Params.Listener.GetPosition().ToString());
	}
	else
	{
		// when listener is ouside the room, the sound inside the room is moved to the closest portal

		FAtomRuntimeId RuntimeID = Params.Listener.GetOwningRuntimeID();
		FAtomRuntimeHandle Runtime = FAtomRuntimeManager::Get()->GetAtomRuntime(RuntimeID);

		if (Runtime.IsValid())
		{
			if (auto AGVS = FAtomRuntime::GetSubsystem<UAtomGameplayVolumeSubsystem>(Runtime))
			{
				const UAtomGameplayVolumeComponent* RoomVolume = AGVS->GetVolumeComponent(VolumeID);
				TArray<const UPrimitiveComponent*> Portals;

				if (!ComponentTag.IsNone())
				{
					TArray<UActorComponent*> Components = RoomVolume->GetOwner()->GetComponentsByTag(UAtomGameplayVolumeComponent::StaticClass(), ComponentTag);
					for (auto Component : Components)
					{
						if (auto AGVComponent = Cast<UAtomGameplayVolumeComponent>(Component))
						{
							if (const UPrimitiveComponent* OtherPrimitive = AGVS->GetPrimitiveComponent(AGVComponent))
							{
								if (!Portals.Contains(OtherPrimitive))
								{
									Portals.Add(OtherPrimitive);
								}
							}
						}
					}
				}

				if (bUseOvelappingPortals)
				{
					// search for portals that overlaps this room
					if (const UPrimitiveComponent* Primitive = AGVS->GetPrimitiveComponent(RoomVolume))
					{
						AGVS->IterateOverVolumeComponents([this, AGVS, Primitive, &Portals](uint32 OtherID, const UAtomGameplayVolumeComponent* OtherVolume)
						{
							FAtomProxyMutatorPriorities Priorities;
							Priorities.bFilterPayload = true;
							Priorities.PayloadType = PayloadFlags::AGCP_ActiveSound;
							Priorities.PriorityMap.Add(TEXT("Portal"), 0);
							FAtomProxyMutatorSearchResult Results;
							OtherVolume->GetProxy()->GatherMutators(Priorities, Results);

							for (auto& Mutator : Results.MatchingMutators)
							{
								if (Mutator.IsValid())
								{
									if (const UPrimitiveComponent* OtherPrimitive = AGVS->GetPrimitiveComponent(OtherVolume))
									{
										if (!Portals.Contains(OtherPrimitive))
										{
											if (Primitive->OverlapComponent(OtherPrimitive->GetComponentLocation(), OtherPrimitive->GetComponentQuat(), OtherPrimitive->GetCollisionShape()))
											{
												Portals.Add(OtherPrimitive);
											}
										}
									}
								}
							}
						});
					}
				}

				// user child component that use portal mutator - USER settings
				
				if (PortailingMethod == EAtomPortailingMethod::ClosestPortal)
				{
					float Distance = UE_MAX_FLT;
					FVector NewActiveSoundPosition = Params.Transform.GetLocation();
					for (auto Portal : Portals)
					{
						FVector Position;
						float Squared;
						Portal->GetSquaredDistanceToCollision(Params.Listener.GetPosition(), Squared, Position);
				
						if (Squared < Distance)
						{
							Distance = Squared;
							NewActiveSoundPosition = Position;
						}

						// normallly add a multi postion for each portals with attenuation mutiplied with the distance and user factor
					}

					if (Distance < UE_KINDA_SMALL_NUMBER)
					{
						// We are inside the portal
					}
					else
					{
						Params.Transform.SetLocation(NewActiveSoundPosition);
					}
					//UE_LOG(LogTemp, Display, TEXT("NOT IN VOLUME %s"), *Params.Transform.ToString());
				}
				else if (PortailingMethod == EAtomPortailingMethod::MultiplePortals)
				{
					Params.MultiPositionType = EAtomMultiPositionType::MultiSources;

					if (Params.MultiPositions.Num() != Portals.Num())
					{
						Params.MultiPositions.AddDefaulted(Portals.Num());
					}

					for (int Index = 0; Index < Portals.Num(); ++Index)
					{
						if (const UPrimitiveComponent* Portal = Portals[Index])
						{
							Params.MultiPositions[Index] = Portal->GetComponentTransform();
						}
					}
				}
			}
		}
	}
}

UAtomRoomVolumeComponent::UAtomRoomVolumeComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PayloadType = AtomGameplay::EComponentPayload::AGCP_ActiveSound | AtomGameplay::EComponentPayload::AGCP_Listener;
	bAutoActivate = true;
}

void UAtomRoomVolumeComponent::SetPortailingMethod(EAtomPortailingMethod InPortailingMethod)
{
	PortailingMethod = InPortailingMethod;

	// Let the parent volume know we've changed
	NotifyDataChanged();
}

void UAtomRoomVolumeComponent::UseOvelappingPortals(bool bInUseOvelappingPortals)
{
	bUseOvelappingPortals = bInUseOvelappingPortals;

	// Let the parent volume know we've changed
	NotifyDataChanged();
}

void UAtomRoomVolumeComponent::SetComponenetTag(FName InComponentTag)
{
	ComponentTag = InComponentTag;

	// Let the parent volume know we've changed
	NotifyDataChanged();
}

TSharedPtr<FAtomProxyVolumeMutator> UAtomRoomVolumeComponent::FactoryMutator() const
{
	return MakeShared<FAtomProxyMutator_Room>();
}

void UAtomRoomVolumeComponent::CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const
{
	TSharedPtr<FAtomProxyMutator_Room> RoomMutator = StaticCastSharedPtr<FAtomProxyMutator_Room>(Mutator);
	RoomMutator->PortailingMethod = PortailingMethod;
	RoomMutator->bUseOvelappingPortals = bUseOvelappingPortals;
	RoomMutator->ComponentTag = ComponentTag;
}
