
#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"

#include "AtomMapAnyKey.h"

#include "AtomGameplayTagCacheSubsystem.generated.h"

#define CRI_API CRIWARECORE_API

using FAtomGameplayTagMap = Atom::TMapAnyKey<FGameplayTag>;

/**
 * UAtomGameplayTagCacheSubsystem - Per world subsystem used to persist gameplay tags that are expensive to construct dynamically from parts
 */
UCLASS(MinimalAPI)
class UAtomGameplayTagCacheSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public: 

	UAtomGameplayTagCacheSubsystem() = default;
	virtual ~UAtomGameplayTagCacheSubsystem() = default;

	//~ Begin USubsystem interface
	CRI_API virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	CRI_API virtual void Deinitialize() override;
	//~ End USubsystem interface

	static CRI_API UAtomGameplayTagCacheSubsystem* Get(const UWorld* WorldContext);

	FAtomGameplayTagMap& GetTagCache()
	{
		return GameplayTagCache;
	}

protected:

	FAtomGameplayTagMap GameplayTagCache;
};

#undef CRI_API
