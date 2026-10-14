
#pragma once

#include "AtomGameplayVolumeMutator.h"

#include "AtomAttenuationVolumeComponent.generated.h"

// Forward Declarations
struct FAtomInteriorSettings;

/**
 *  FAtomProxyMutator_Attenuation - An Atom thread representation of occlusion settings (volume attenuation)
 */
class FAtomProxyMutator_Attenuation
	: public FAtomProxyVolumeMutator
{
public:

	FAtomProxyMutator_Attenuation();
	virtual ~FAtomProxyMutator_Attenuation() = default;

	float ExteriorVolume = 1.0f;
	float ExteriorTime = 0.5f;
	float InteriorVolume = 1.0f;
	float InteriorTime = 0.5f;

	virtual void Apply(FAtomInteriorSettings& InteriorSettings) const override;
	virtual void Apply(FAtomProxyActiveSoundParams& Params) const override;

protected:

	constexpr static const TCHAR MutatorAttenuationName[] = TEXT("Attenuation");
};

/**
 *  UAtomAttenuationVolumeComponent - Atom Gameplay Volume component for occlusion settings (volume attenuation)
 */
UCLASS(Blueprintable, Config = Game, ClassGroup = ("AtomGameplayVolume"), meta = (BlueprintSpawnableComponent, DisplayName = "Attenuation"))
class CRIWARECORE_API UAtomAttenuationVolumeComponent
	: public UAtomGameplayVolumeMutator
{
	GENERATED_BODY()

public:

	UAtomAttenuationVolumeComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomAttenuationVolumeComponent() = default;

	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void SetExteriorVolume(float Volume, float InterpolateTime);

	float GetExteriorVolume() const { return ExteriorVolume; }
	float GetExteriorTime() const { return ExteriorTime; }

	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void SetInteriorVolume(float Volume, float InterpolateTime);

	float GetInteriorVolume() const { return InteriorVolume; }
	float GetInteriorTime() const { return InteriorTime; }

private:

	//~ Begin UAtomGameplayVolumeMutator interface
	virtual TSharedPtr<FAtomProxyVolumeMutator> FactoryMutator() const override;
	virtual void CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const override;
	//~ End UAtomGameplayVolumeMutator interface

	// The desired volume of sounds outside the volume when the player is inside the volume
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VolumeAttenuation", meta = (AllowPrivateAccess = "true"))
	float ExteriorVolume = 1.0f;

	// The time over which to interpolate from the current volume to the desired volume of sounds outside the volume when the player enters the volume
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VolumeAttenuation", meta = (AllowPrivateAccess = "true", Units = s))
	float ExteriorTime = 0.5f;

	// The desired volume of sounds inside the volume when the player is outside the volume
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VolumeAttenuation", meta = (AllowPrivateAccess = "true"))
	float InteriorVolume = 1.0f;

	// The time over which to interpolate from the current volume to the desired volume of sounds inside the volume when the player enters the volume
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VolumeAttenuation", meta = (AllowPrivateAccess = "true", Units = s))
	float InteriorTime = 0.5f;
};
