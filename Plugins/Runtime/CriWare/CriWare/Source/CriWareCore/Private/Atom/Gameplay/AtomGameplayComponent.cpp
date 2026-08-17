
#include "Atom/Gameplay/AtomGameplayComponent.h"

#include "Atom/Gameplay/AtomGameplayLogs.h"

DEFINE_LOG_CATEGORY(AtomGameplayLog);

UAtomGameplayComponent::UAtomGameplayComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);
}

void UAtomGameplayComponent::Activate(bool bReset)
{
	const bool bWasActive = IsActive();
	Super::Activate(bReset);

	if (!bWasActive && IsActive() && GetNetMode() != NM_DedicatedServer)
	{
		Enable();
	}
}

void UAtomGameplayComponent::Deactivate()
{
	const bool bWasActive = IsActive();
	Super::Deactivate();

	if (bWasActive && !IsActive() && GetNetMode() != NM_DedicatedServer)
	{
		Disable();
	}
}

void UAtomGameplayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Deactivate();
	Super::EndPlay(EndPlayReason);
}

bool UAtomGameplayComponent::HasPayloadType(PayloadFlags InType) const
{
	return (PayloadType & InType) != PayloadFlags::AGCP_None;
}

void UAtomGameplayComponent::Enable()
{
	UE_LOG(AtomGameplayLog, Verbose, TEXT("AtomGameplayComponent Enabled (%s)."), *GetFullName());
}

void UAtomGameplayComponent::Disable()
{
	UE_LOG(AtomGameplayLog, Verbose, TEXT("AtomGameplayComponent Disabled (%s)."), *GetFullName());
}
