
#pragma once

#include "AtomGameplayVolumeMutator.h"

#include "AtomFilterVolumeComponent.generated.h"

// Forward Declarations
struct FAtomInteriorSettings;

/**
 *  FAtomProxyMutator_Filter - An Atom thread representation of occlusion settings (volume filter)
 */
class FAtomProxyMutator_Filter
	: public FAtomProxyVolumeMutator
{
public:

	FAtomProxyMutator_Filter();
	virtual ~FAtomProxyMutator_Filter() = default;

	float ExteriorLPF = MAX_FILTER_FREQUENCY;
	float ExteriorLPFTime = 0.5f;
	float InteriorLPF = MAX_FILTER_FREQUENCY;
	float InteriorLPFTime = 0.5f;

	virtual void Apply(FAtomInteriorSettings& InteriorSettings) const override;
	virtual void Apply(FAtomProxyActiveSoundParams& Params) const override;

protected:

	constexpr static const TCHAR MutatorFilterName[] = TEXT("Filter");
};

/**
 *  UAtomFilterVolumeComponent - Atom Gameplay Volume component for occlusion settings (volume filter)
 */
UCLASS(Blueprintable, Config = Game, ClassGroup = ("AtomGameplayVolume"), meta = (BlueprintSpawnableComponent, DisplayName = "Filter"))
class CRIWARECORE_API UAtomFilterVolumeComponent
	: public UAtomGameplayVolumeMutator
{
	GENERATED_BODY()

public:

	UAtomFilterVolumeComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomFilterVolumeComponent() = default;

	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void SetExteriorLPF(float Volume, float InterpolateTime);

	float GetExteriorLPF() const { return ExteriorLPF; }
	float GetExteriorLPFTime() const { return ExteriorLPFTime; }

	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void SetInteriorLPF(float Volume, float InterpolateTime);

	float GetInteriorLPF() const { return InteriorLPF; }
	float GetInteriorLPFTime() const { return InteriorLPFTime; }

private:

	//~ Begin UAtomGameplayVolumeMutator interface
	virtual TSharedPtr<FAtomProxyVolumeMutator> FactoryMutator() const override;
	virtual void CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const override;
	//~ End UAtomGameplayVolumeMutator interface

	// The desired LPF frequency cutoff (in hertz) of sounds outside the volume when the player is inside the volume
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VolumeFilter", meta = (AllowPrivateAccess = "true", MinClamp = "24", MaxClamp = "24000", MinUI = "24", MaxUI = "24000", Units = Hz))
	float ExteriorLPF = MAX_FILTER_FREQUENCY;

	// The time over which to interpolate from the current LPF to the desired LPF of sounds outside the volume when the player enters the volume
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VolumeFilter", meta = (AllowPrivateAccess = "true", Units = s))
	float ExteriorLPFTime = 0.5f;

	// The desired LPF frequency cutoff (in hertz) of sounds inside the volume when the player is outside the volume
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VolumeFilter", meta = (AllowPrivateAccess = "true", MinClamp = "24", MaxClamp = "24000", MinUI = "24", MaxUI = "24000", Units = Hz))
	float InteriorLPF = MAX_FILTER_FREQUENCY;

	// The time over which to interpolate from the current LPF to the desired LPF of sounds inside the volume when the player enters the volume
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VolumeFilter", meta = (AllowPrivateAccess = "true", Units = s))
	float InteriorLPFTime = 0.5f;
};
