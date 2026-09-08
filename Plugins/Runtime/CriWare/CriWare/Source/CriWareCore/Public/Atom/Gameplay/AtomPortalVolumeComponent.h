
#pragma once

#include "CoreMinimal.h"
#include "Atom/Gameplay/AtomGameplayVolumeMutator.h"

#include "AtomPortalVolumeComponent.generated.h"

/**
 *  FAtomProxyMutator_Portal - An Atom thread representation of Portal settings
 */
class FAtomProxyMutator_Portal
	: public FAtomProxyVolumeMutator
{
public:

	FAtomProxyMutator_Portal();
	virtual ~FAtomProxyMutator_Portal() = default;

	virtual void Apply(FAtomProxyActiveSoundParams& Params) const override;

protected:

	constexpr static const TCHAR MutatorPortalName[] = TEXT("Portal");
};

/**
 * 
 */
UCLASS(Blueprintable, Config = Game, ClassGroup = ("AtomGameplayVolume"), meta = (BlueprintSpawnableComponent, DisplayName = "Portal"))
class CRIWARECORE_API UAtomPortalVolumeComponent
	: public UAtomGameplayVolumeMutator
{
	GENERATED_BODY()

public:

	UAtomPortalVolumeComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomPortalVolumeComponent() = default;

private:

	//~ Begin UAtomGameplayVolumeMutator interface
	virtual TSharedPtr<FAtomProxyVolumeMutator> FactoryMutator() const override;
	virtual void CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const override;
	//~ End UAtomGameplayVolumeMutator interface
};
