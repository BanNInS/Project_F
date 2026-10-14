
#pragma once

#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "Containers/ArrayView.h"
#include "Containers/Set.h"
#include "HAL/CriticalSection.h"

#include "AtomModulation.h"
#include "AtomSoundModulation.h"
#include "AtomAisacModulation.h"
#include "Atom/AtomUtility.h"

#include "AtomModulationDestination.generated.h"

// Forward Declarations
class UAtomModulatorBase;
class UAtomAisacPatch;
namespace Atom
{
	struct FModulationParameter;
}

/*
 * Modulation destination settings - defines various structures an enums to setup modulation destinations.
 */

 /** Enumeration for diffent moduation routing for a modulation destination of Atom sound. */
UENUM(BlueprintType)
enum class EAtomModulationRouting : uint8
{
	/* Disables modulation routing. */
	Disable,

	/* Inherits modulation routing (AtomComponent inherits from Sound, Sound inherits from SoundClass). */
	Inherit,

	/* Ignores inherited settings and uses modulation settings on this object. */
	Override,

	/* Performs set union on local modulation sources with those inherited (AtomComponent inherits from Sound, Sound inherits from SoundClass). */
	Union,

	/* Ignores modulations and values in UnrealEngine and use the original settings from DAW (CRI Atom Craft) set by sound designer. */
	DAW UMETA(DisplayName="DAW - Atom Craft")
};

UENUM(BlueprintType)
enum class EAtomModulationDestination : uint8
{
	/* Volume modulation */
	Volume,

	/* Pitch modulation */
	Pitch,

	/* Cutoff Frequency of a lowpass filter */
	Lowpass,

	/* Cutoff Frequency of a highpass filter */
	Highpass,

	Count UMETA(Hidden)
};

/** Structure allowing modulation override for Atom sound. */
USTRUCT(BlueprintType)
struct FAtomSoundModulationDefaultSettings
{
	GENERATED_BODY()

public:

	CRIWARECORE_API FAtomSoundModulationDefaultSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation, meta = (DisplayName = "AISAC", AtomParam = "AISAC"))
	FAtomAisacModulationDefaultSettings AisacModulations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation, meta = (DisplayName = "Volume", AtomParam = "Volume", AtomParamClass = "AtomModulationParameterVolume"))
	FAtomSoundModulationSettings VolumeModulation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation, meta = (DisplayName = "Pitch", AtomParam = "Pitch", AtomParamClass = "AtomModulationParameterBipolar"))
	FAtomSoundModulationSettings PitchModulation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation, meta = (DisplayName = "Highpass", AtomParam = "HPFCutoffFrequency", AtomParamClass = "AtomModulationParameterHPFFrequency"))
	FAtomSoundModulationSettings HighpassModulation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation, meta = (DisplayName = "Lowpass", AtomParam = "LPFCutoffFrequency", AtomParamClass = "AtomModulationParameterLPFFrequency"))
	FAtomSoundModulationSettings LowpassModulation;

	FAtomSoundModulationSettings GetSettingsForDestination(EAtomModulationDestination Destination) const;
};

/** Structure allowing modulation routing and override for source Atom sound. */
USTRUCT(BlueprintType)
struct FAtomSoundModulationRoutingSettings
	: public FAtomSoundModulationDefaultSettings
{
	GENERATED_BODY()

public:

	CRIWARECORE_API FAtomSoundModulationRoutingSettings();

	/** What volume modulation settings to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation)
	EAtomModulationRouting VolumeRouting = EAtomModulationRouting::Inherit;

	/** What pitch modulation settings to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation)
	EAtomModulationRouting PitchRouting = EAtomModulationRouting::Inherit;

	/** What low-pass modulation settings to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation)
	EAtomModulationRouting LowpassRouting = EAtomModulationRouting::Inherit;

	/** What high-pass modulation settings to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation)
	EAtomModulationRouting HighpassRouting = EAtomModulationRouting::Inherit;

	EAtomModulationRouting GetRoutingForDestination(EAtomModulationDestination Destination) const;
};

namespace Atom
{
	struct FModulationDestination
	{
	public:

		CRIWARECORE_API FModulationDestination() = default;

		CRIWARECORE_API FModulationDestination(const FModulationDestination& InModulationDestination);
		CRIWARECORE_API FModulationDestination(FModulationDestination&& InModulationDestination);

		CRIWARECORE_API FModulationDestination& operator=(const FModulationDestination& InModulationDestination);
		CRIWARECORE_API FModulationDestination& operator=(FModulationDestination&& InModulationDestination);

		/** Initializes the modulation destination
			* InRuntimeId - Atom runtime associated with modulation plugin instance
			* bInIsBuffered - Whether or not to run destination in "buffered mode," which manages an internal buffer to smooth modulation value between process calls
			* bInValueNormalized - Whether or not to keep the output value in normalized, unitless [0.0f, 1.0f] space
			*/
		CRIWARECORE_API void Init(FAtomRuntimeId InRuntimeID, bool bInIsBuffered = false, bool bInValueNormalized = false);

		/** Initializes the modulation destination
			* InRuntimeId - Atom runtime associated with modulation plugin instance
			* InParameterName - Name of parameter used to mix/convert destination value to/from unit space
			* bInIsBuffered - Whether or not to run destination in "buffered mode," which manages an internal buffer to smooth modulation value between process calls
			* bInValueNormalized - Whether or not to keep the output value in normalized, unitless [0.0f, 1.0f] space
			*/
		CRIWARECORE_API void Init(FAtomRuntimeId InRuntimeID, FName InParameterName, bool bInIsBuffered = false, bool bInValueNormalized = false);

		/** returns whether or not destination references an active modulator */
		CRIWARECORE_API bool IsActive();

		/* Updates internal value (or buffer if set to bIsBuffered) to current modulated result using the provided value as the base carrier value to modulate.
			* Returns true if value was updated.
			*/
		CRIWARECORE_API bool ProcessControl(float InValueUnitBase, int32 InNumSamples = 0);

		CRIWARECORE_API void UpdateModulators(const TSet<TObjectPtr<UAtomModulatorBase>>& InModulators);
		CRIWARECORE_API void UpdateModulators(const TSet<UAtomModulatorBase*>& InModulators);
		CRIWARECORE_API void UpdateModulators(const TSet<const UAtomModulatorBase*>& InModulators);

	private:

		CRIWARECORE_API void UpdateModulatorsInternal(TArray<TUniquePtr<Atom::IModulatorSettings>>&& ProxySettings);

		struct FModulationDestinationData
		{
			FAtomRuntimeId RuntimeID = FAtomRuntimeId(INDEX_NONE);

			float ValueTarget = 1.0f;

			bool bIsBuffered = false;
			bool bValueNormalized = false;
			bool bHasProcessed = false;

			FAlignedFloatBuffer OutputBuffer;

			TSet<FModulatorHandle> Handles;

			FModulationParameter Parameter;

			mutable FCriticalSection HandleCritSection;

			FModulationDestinationData& operator=(const FModulationDestinationData& InDestInfo);
			FModulationDestinationData& operator=(FModulationDestinationData&& InDestInfo);
			const FAtomRuntimeId& GetRuntimeID() const;
			const FModulationParameter& GetParameter() const;
			void SetHandles(TSet<FModulatorHandle>&& Handles);
			void ResetHandles();
		};

		TSharedRef<FModulationDestinationData> DestinationData{ MakeShared<FModulationDestinationData>() };

	public:
		/** Returns buffer of interpolated modulation values. If not set to "IsBuffered" when initialized, returns an empty array. */
		FORCEINLINE const FAlignedFloatBuffer& GetBuffer() const
		{
			return DestinationData->OutputBuffer;
		}

		/** Returns whether or not the destination has requested to
			* process the control or not. */
		FORCEINLINE bool GetHasProcessed() const
		{
			return DestinationData->bHasProcessed;
		}

		/** Returns sample value last reported by modulator. Returns value in unit space, unless
			* 'ValueNormalized' option is set on initialization.
			*/
		FORCEINLINE float GetValue() const
		{
			return DestinationData->ValueTarget;
		}
	};
} // namespace

#define CRI_API CRIWARECORE_API

UCLASS(MinimalAPI, config = Engine, EditInlineNew, BlueprintType)
class UAtomModulationDestination : public UObject
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TObjectPtr<const UAtomModulatorBase> Modulator;

	Atom::FModulationDestination Destination;

public:
	CRI_API virtual void PostInitProperties() override;

	// Returns true if a modulator was set and has been cleared.
	UFUNCTION(BlueprintCallable, Category = "Atom|Modulation", DisplayName = "Clear Modulator")
	CRI_API UPARAM(DisplayName = "Modulator Cleared") bool ClearModulator();

	// Returns currently set modulator.
	UFUNCTION(BlueprintPure, Category = "Atom|Modulation")
	CRI_API UPARAM(DisplayName = "Modulator") const UAtomModulatorBase* GetModulator() const;

	// Returns the last calculated modulator value sampled from the thread controls are processed on.
	UFUNCTION(BlueprintPure, Category = "Atom|Modulation", DisplayName = "Get Watched Modulator Value")
	CRI_API UPARAM(DisplayName = "Value") float GetValue() const;

	// Returns true if modulator was set to new value or was already set to provided value.
	UFUNCTION(BlueprintCallable, Category = "Atom|Modulation", DisplayName = "Set Watched Modulator")
	CRI_API UPARAM(DisplayName = "Modulator Set") bool SetModulator(const UAtomModulatorBase* InModulator);
};

#undef CRI_API
