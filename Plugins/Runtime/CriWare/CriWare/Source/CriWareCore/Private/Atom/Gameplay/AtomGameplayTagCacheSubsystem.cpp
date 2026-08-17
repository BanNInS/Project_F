
#include "Atom/Gameplay/AtomGameplayTagCacheSubsystem.h"

#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomGameplayTagCacheSubsystem)

namespace AtomGameplayTagCacheConsoleVariables
{
	int32 bEnabled = 1;
	FAutoConsoleVariableRef CVarEnabled(
		TEXT("au.GameplayTagCache.Enabled"),
		bEnabled,
		TEXT("Cache any string concatenations used to build Gameplay Tags at runtime.\n0: Disable, 1: Enable (default)"),
		ECVF_Default);

} // namespace AtomGameplayTagCacheConsoleVariables

bool UAtomGameplayTagCacheSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (AtomGameplayTagCacheConsoleVariables::bEnabled == 0)
	{
		return false;
	}

	return Super::ShouldCreateSubsystem(Outer);
}

void UAtomGameplayTagCacheSubsystem::Deinitialize()
{
	GameplayTagCache.Empty();

	Super::Deinitialize();
}

UAtomGameplayTagCacheSubsystem* UAtomGameplayTagCacheSubsystem::Get(const UWorld* WorldContext)
{
	if (WorldContext)
	{
		return WorldContext->GetSubsystem<UAtomGameplayTagCacheSubsystem>();
	}

	return nullptr;
}
