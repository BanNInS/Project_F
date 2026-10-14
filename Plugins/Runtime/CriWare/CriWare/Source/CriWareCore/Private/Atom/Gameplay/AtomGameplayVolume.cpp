
#include "Atom/Gameplay/AtomGameplayVolume.h"

#include "Components/BrushComponent.h"
#include "Engine/CollisionProfile.h"
#include "Net/UnrealNetwork.h"

#include "CriWareCorePrivate.h"
#include "Atom/Gameplay/AtomGameplayVolumeLogs.h"
#include "Atom/Gameplay/AtomGameplayVolumeProxy.h"
#include "Atom/Gameplay/AtomGameplayVolumeComponent.h"

#define LOCTEXT_NAMESPACE "AtomGameplayVolume"

DEFINE_LOG_CATEGORY(AtomGameplayVolumeLog);

AAtomGameplayVolume::AAtomGameplayVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UBrushComponent* BrushComp = GetBrushComponent())
	{
		BrushComp->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		BrushComp->SetGenerateOverlapEvents(false);
		BrushComp->bAlwaysCreatePhysicsState = true;
	}

	AGVComponent = CreateDefaultSubobject<UAtomGameplayVolumeComponent>(TEXT("AGVComponent"));

#if WITH_EDITOR
	bColored = true;
	BrushColor = FColor(255, 255, 0, 255);

	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif // WITH_EDITOR
}

void AAtomGameplayVolume::SetEnabled(bool bEnable)
{
	if (bEnable != bEnabled)
	{
		bEnabled = bEnable;
		if (CanSupportProxy())
		{
			AddProxy();
		}
		else
		{
			RemoveProxy();
		}
	}
}

void AAtomGameplayVolume::OnListenerEnter_Implementation()
{
	OnListenerEnterEvent.Broadcast();
}

void AAtomGameplayVolume::OnListenerExit_Implementation()
{
	OnListenerExitEvent.Broadcast();
}

#if WITH_EDITOR
void AAtomGameplayVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AAtomGameplayVolume, bEnabled))
	{
		if (CanSupportProxy())
		{
			AddProxy();
		}
		else
		{
			RemoveProxy();
		}
	}
}
#endif // WITH_EDITOR

void AAtomGameplayVolume::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AAtomGameplayVolume, bEnabled);
}

void AAtomGameplayVolume::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (CanSupportProxy())
	{
		AddProxy();
	}
}

void AAtomGameplayVolume::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	if (USceneComponent* SC = GetRootComponent())
	{
		SC->TransformUpdated.AddUObject(this, &AAtomGameplayVolume::TransformUpdated);
	}

	if (AGVComponent)
	{
		UAtomGameplayVolumePrimitiveComponentProxy* PrimitiveComponentProxy = Cast<UAtomGameplayVolumePrimitiveComponentProxy>(AGVComponent->GetProxy());
		if (!PrimitiveComponentProxy)
		{
			AGVComponent->SetProxy(NewObject<UAtomGameplayVolumePrimitiveComponentProxy>(AGVComponent));
		}

		AGVComponent->bAutoActivate = false;
	}
}

void AAtomGameplayVolume::PostUnregisterAllComponents()
{
	RemoveProxy();

	// Component can be nulled due to GC at this point
	if (USceneComponent* SC = GetRootComponent())
	{
		SC->TransformUpdated.RemoveAll(this);
	}

	Super::PostUnregisterAllComponents();
}

void AAtomGameplayVolume::OnComponentDataChanged()
{
	if (CanSupportProxy())
	{
		UpdateProxy();
	}
}

bool AAtomGameplayVolume::CanSupportProxy() const
{
	if (!bEnabled || !AGVComponent || !AGVComponent->GetProxy())
	{
		return false;
	}

	return true;
}

void AAtomGameplayVolume::OnRep_bEnabled()
{
	if (CanSupportProxy())
	{
		AddProxy();
	}
	else
	{
		RemoveProxy();
	}
}

void AAtomGameplayVolume::TransformUpdated(USceneComponent* InRootComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport)
{
	UpdateProxy();
}

void AAtomGameplayVolume::AddProxy() const
{
	if (AGVComponent)
	{
		AGVComponent->OnProxyEnter.AddUniqueDynamic(this, &AAtomGameplayVolume::OnListenerEnter);
		AGVComponent->OnProxyExit.AddUniqueDynamic(this, &AAtomGameplayVolume::OnListenerExit);
		AGVComponent->Activate();
	}
}

void AAtomGameplayVolume::RemoveProxy() const
{
	if (AGVComponent)
	{
		AGVComponent->Deactivate();
		AGVComponent->OnProxyEnter.RemoveAll(this);
		AGVComponent->OnProxyExit.RemoveAll(this);
	}
}

void AAtomGameplayVolume::UpdateProxy() const
{
	if (AGVComponent)
	{
		AGVComponent->OnComponentDataChanged();
	}
}

#undef LOCTEXT_NAMESPACE
