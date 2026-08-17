
#pragma once

#include "UObject/ObjectMacros.h"

#include "Atom.h"

#include "AtomVolume.generated.h"

// Enum describing the state of checking Atom gameplay volume location
UENUM(BlueprintType)
enum class EAtomVolumeLocationState : uint8
{
	// Used for when the listener is located inside the Atom gameplay Volume
	InsideTheVolume,

	// Used for when the listener is located outside the Atom gameplay Volume
	OutsideTheVolume,
};

/** Struct to determine dynamic bus send data for use with Atom gameplay volumes. */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomVolumeBusSendSettings
{
	GENERATED_BODY()

public:

	/** The state the listener needs to be in, relative to the Atom volume, for this bus send list to be used for a given sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atom Volume Bus Sends")
	EAtomVolumeLocationState ListenerLocationState = EAtomVolumeLocationState::InsideTheVolume;

	/** Bus send array for sounds that are in the AtomListenerLocationState at the same time as the listener. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atom Volume Bus Sends")
	TArray<FAtomSoundToBusSend> BusSends;
};

/** Struct encapsulating settings for interior areas. */
USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomInteriorSettings
{
	GENERATED_BODY()

	// Whether these interior settings are the default values for the world
	UPROPERTY()
	bool bIsWorldSettings;

	// The desired volume of sounds outside the volume when the player is inside the volume
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteriorSettings)
	float ExteriorVolume;

	// The time over which to interpolate from the current volume to the desired volume of sounds outside the volume when the player enters the volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteriorSettings, meta = (Units = s))
	float ExteriorTime;

	// The desired LPF frequency cutoff in hertz of sounds outside the volume when the player is inside the volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteriorSettings, meta = (MinClamp = "24", MaxClamp = "24000", MinUI = "24", MaxUI = "24000", Units = Hz))
	float ExteriorLPF;

	// The time over which to interpolate from the current LPF to the desired LPF of sounds outside the volume when the player enters the volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteriorSettings, meta = (Units = s))
	float ExteriorLPFTime;

	// The desired volume of sounds inside the volume when the player is outside the volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteriorSettings)
	float InteriorVolume;

	// The time over which to interpolate from the current volume to the desired volume of sounds inside the volume when the player exits the volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteriorSettings, meta = (Units = s))
	float InteriorTime;

	// The desired LPF frequency cutoff in hertz of sounds inside the volume when the player is outside the volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteriorSettings, meta = (MinClamp = "24", MaxClamp = "24000", MinUI = "24", MaxUI = "24000", Units = Hz))
	float InteriorLPF;

	// The time over which to interpolate from the current LPF to the desired LPF of sounds inside the volume when the player exits the volume.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InteriorSettings, meta = (Units = s))
	float InteriorLPFTime;

	FAtomInteriorSettings();

	bool operator==(const FAtomInteriorSettings& Other) const;
	bool operator!=(const FAtomInteriorSettings& Other) const;
};

// for compatiblity
struct FAtomReverbSettings
{};
