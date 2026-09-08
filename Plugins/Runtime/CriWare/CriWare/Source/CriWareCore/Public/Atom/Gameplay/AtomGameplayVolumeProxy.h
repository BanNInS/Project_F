
#pragma once

#include "UObject/Object.h"
#include "Templates/SharedPointer.h"

#include "AtomGameplayFlags.h"

#include "AtomGameplayVolumeProxy.generated.h"

// Forward Declarations 
struct FBodyInstance;
struct FAtomProxyMutatorPriorities;
struct FAtomProxyMutatorSearchResult;
class FAtomProxyVolumeMutator;
class UPrimitiveComponent;
class UAtomGameplayVolumeComponent;

/**
 *  UAtomGameplayVolumeProxy - Abstract proxy used on audio thread to represent Atom gameplay volumes.
 */
UCLASS(Abstract, EditInlineNew, HideDropdown)
class CRIWARECORE_API UAtomGameplayVolumeProxy
	: public UObject
{
	GENERATED_BODY()

public:

	using PayloadFlags = AtomGameplay::EComponentPayload;
	using ProxyMutatorList = TArray<TSharedPtr<FAtomProxyVolumeMutator>>;

	UAtomGameplayVolumeProxy(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomGameplayVolumeProxy() = default;

	virtual bool ContainsPosition(const FVector& Position) const;
	virtual void InitFromComponent(const UAtomGameplayVolumeComponent* Component);

	void FindMutatorPriority(FAtomProxyMutatorPriorities& Priorities) const;
	void GatherMutators(const FAtomProxyMutatorPriorities& Priorities, FAtomProxyMutatorSearchResult& OutResult) const;

	void AddPayloadType(PayloadFlags InType);
	bool HasPayloadType(PayloadFlags InType) const;

	uint32 GetVolumeID() const;
	uint32 GetWorldID() const;

protected:

	ProxyMutatorList ProxyVolumeMutators;

	uint32 VolumeID = INDEX_NONE;
	uint32 WorldID = INDEX_NONE;
	PayloadFlags PayloadType = PayloadFlags::AGCP_None;
};

/**
 *  UAGVPrimitiveComponentProxy - Proxy based on a volume's primitive component
 */
UCLASS(meta = (DisplayName = "AGV Primitive Proxy"))
class UAtomGameplayVolumePrimitiveComponentProxy
	: public UAtomGameplayVolumeProxy
{
	GENERATED_BODY()

public:

	UAtomGameplayVolumePrimitiveComponentProxy(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomGameplayVolumePrimitiveComponentProxy() = default;

	virtual bool ContainsPosition(const FVector& Position) const override;
	virtual void InitFromComponent(const UAtomGameplayVolumeComponent* Component) override;

	TWeakObjectPtr<UPrimitiveComponent> GetPrimitiveComponent() const { return WeakPrimitive; }

protected:
	
	TWeakObjectPtr<UPrimitiveComponent> WeakPrimitive;
};

/**
 *  UAtomGameplayVolumeConditionProxy - Proxy for use with the UAtomGameplayCondition interface
 */
UCLASS(meta = (DisplayName = "AGV Condition Proxy"))
class UAtomGameplayVolumeConditionProxy
	: public UAtomGameplayVolumeProxy
{
	GENERATED_BODY()

public:

	UAtomGameplayVolumeConditionProxy(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomGameplayVolumeConditionProxy() = default;

	virtual bool ContainsPosition(const FVector& Position) const override;
	virtual void InitFromComponent(const UAtomGameplayVolumeComponent* Component) override;

protected:

	TWeakObjectPtr<const UObject> WeakObject;
};
