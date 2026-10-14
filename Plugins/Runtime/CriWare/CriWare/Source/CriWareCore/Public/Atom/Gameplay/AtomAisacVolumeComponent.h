
#pragma once

#include "AtomGameplayVolumeMutator.h"
#include "Atom/AtomAisacPatch.h"
#include "Atom/AtomVolume.h"

#include "AtomAisacVolumeComponent.generated.h"

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomVolumeAisacControlSetting
{
	GENERATED_BODY()

public:

	/** The AISAC value to target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISAC")
	FAtomAisacControl Control;

	/** The AISAC value to target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AISAC", meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float Value = 0.0f;
};

/** Struct to determine dynamic Aisac Control data for use with Atom gameplay volumes. */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomVolumeAisacControlSettings
{
	GENERATED_BODY()

public:

	/** The state the listener needs to be in, relative to the Atom volume, for this Aisac control list to be used for a given sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atom Volume AISAC Controls")
	EAtomVolumeLocationState ListenerLocationState = EAtomVolumeLocationState::InsideTheVolume;

	/** AISAC control array for sounds that are in the AtomListenerLocationState at the same time as the listener. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atom Volume AISAC Controls")
	TArray<FAtomVolumeAisacControlSetting> AisacControls;

	/** Time to fade from the current AISAC controls setting into this setting, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atom Volume AISAC Controls", meta = (UIMin = "0.0", UIMax = "10.0", ClampMin = "0.0", Units = s))
	float FadeTime = 0.0f;
};

/**
 *  FAtomProxyMutator_Aisac - An Atom thread representation of Aisac.
 */
class FAtomProxyMutator_Aisac
	: public FAtomProxyVolumeMutator
{
public:

	FAtomProxyMutator_Aisac();
	virtual ~FAtomProxyMutator_Aisac() = default;

	TArray<FAtomVolumeAisacControlSettings> AisacControlSettings;

	virtual void Apply(FAtomProxyActiveSoundParams& Params) const override;

protected:

	float Interpolate(double CurrentTime, double EndTime) const;

	mutable double StartTime = -1.0;
	mutable EAtomVolumeLocationState PrevLocationState = EAtomVolumeLocationState::OutsideTheVolume;
	mutable TMap<FAtomAisacControl, TPair<float, float>> InterpValues;

	constexpr static const TCHAR MutatorAisacName[] = TEXT("AISAC");
};

/**
 *  UAtomAisacVolumeComponent - Atom Gameplay Volume component for bus sends.
 */
UCLASS(Blueprintable, Config = Game, ClassGroup = ("AtomGameplayVolume"), meta = (BlueprintSpawnableComponent, DisplayName = "AISAC"))
class CRIWARECORE_API UAtomAisacVolumeComponent
	: public UAtomGameplayVolumeMutator
{
	GENERATED_BODY()

public:

	UAtomAisacVolumeComponent(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomAisacVolumeComponent() = default;

	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void SetAisacControlSettings(const TArray<FAtomVolumeAisacControlSettings>& NewAisacControlSettings);

	const TArray<FAtomVolumeAisacControlSettings>& GetAisacControlSettings() const { return AisacControlSettings; }

private:

	//~ Begin UAtomGameplayVolumeMutator Interface
	virtual TSharedPtr<FAtomProxyVolumeMutator> FactoryMutator() const override;
	virtual void CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const override;
	//~ End UAtomGameplayVolumeMutator Interface

	/** AISAC Controls to use for this component. Allows to apply AISAC controls dynamically to ausio on source and listener locations (relative to parent volume.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Volume AISAC Controls", meta = (AllowPrivateAccess = "true"))
	TArray<FAtomVolumeAisacControlSettings> AisacControlSettings;
};
