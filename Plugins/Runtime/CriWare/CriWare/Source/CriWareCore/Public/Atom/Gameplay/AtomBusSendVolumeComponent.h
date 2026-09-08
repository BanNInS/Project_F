
#pragma once

#include "AtomGameplayVolumeMutator.h"

#include "AtomBusSendVolumeComponent.generated.h"

/**
 *  FAtomProxyMutator_BusSend - An Atom thread representation of Bus Sends
 */
class FAtomProxyMutator_BusSend
	: public FAtomProxyVolumeMutator
{
public:

	FAtomProxyMutator_BusSend();
	virtual ~FAtomProxyMutator_BusSend() = default;

	TArray<FAtomVolumeBusSendSettings> BusSendSettings;

	virtual void Apply(FAtomProxyActiveSoundParams& Params) const override;

protected:

	constexpr static const TCHAR MutatorBusSendName[] = TEXT("BusSend");
};

/**
 *  UAtomBusSendVolumeComponent - Atom Gameplay Volume component for bus sends
 */
UCLASS(Blueprintable, Config = Game, ClassGroup = ("AtomGameplayVolume"), meta = (BlueprintSpawnableComponent, DisplayName = "Bus Send"))
class CRIWARECORE_API UAtomBusSendVolumeComponent
	: public UAtomGameplayVolumeMutator
{
	GENERATED_BODY()

public:

	UAtomBusSendVolumeComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomBusSendVolumeComponent() = default;

	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void SetBusSendSettings(const TArray<FAtomVolumeBusSendSettings>& NewBusSendSettings);

	const TArray<FAtomVolumeBusSendSettings>& GetBusSendSettings() const { return BusSendSettings; }

private:

	//~ Begin UAtomGameplayVolumeMutator Interface
	virtual TSharedPtr<FAtomProxyVolumeMutator> FactoryMutator() const override;
	virtual void CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const override;
	//~ End UAtomGameplayVolumeMutator Interface

	/** Bus send settings to use for this component. Allows audio to dynamically send to buses based on source and listener locations (relative to parent volume.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Submixes, meta = (AllowPrivateAccess = "true"))
	TArray<FAtomVolumeBusSendSettings> BusSendSettings;
};
