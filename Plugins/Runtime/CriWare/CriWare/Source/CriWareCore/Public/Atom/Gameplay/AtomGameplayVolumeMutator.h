
#pragma once

#include "Templates/SharedPointer.h"

#include "CriWareDefines.h"
#include "Atom/Atom.h"
#include "AtomGameplayFlags.h"
#include "AtomGameplayVolumeComponent.h"
#include "Atom/Modulation/AtomAisacModulation.h"

#include "AtomGameplayVolumeMutator.generated.h"

// Forward Declarations 
class FAtomGameplayVolumeListener;
struct FAtomGameplayActiveSoundInfo;
struct FAtomInteriorSettings;
enum class EAtomMultiPositionType : uint8;

/**
 *  FAtomProxyActiveSoundParams - Helper struct for collecting info about the active sound from affecting proxy mutators
 */
struct FAtomProxyActiveSoundParams
{
	FAtomProxyActiveSoundParams() = delete;
	FAtomProxyActiveSoundParams(const FAtomGameplayActiveSoundInfo& SoundInfo, const FAtomGameplayVolumeListener& InListener);

	float SourceInteriorVolume = 1.0f;

	float SourceInteriorLPF = ATOM_MAX_FILTER_FREQUENCY;

	bool bAllowSpatialization = false;
	bool bUsingWorldSettings = false;
	bool bListenerInVolume = false;

	bool bAffectedByAttenuation = false;
	bool bAffectedByFilter = false;
	bool bSoundMovable = false;

	const FAtomGameplayVolumeListener& Listener;
	const FAtomGameplayActiveSoundInfo& Sound;

	TArray<FAtomSoundToBusSend> SoundBusSends;
	TMap<FAtomAisacControl, float> AisacControls;

	FTransform Transform;
	EAtomMultiPositionType MultiPositionType;
	TArray<FTransform> MultiPositions;

	void UpdateInteriorValues();
};

/**
 *  FAtomProxyMutatorPriorities - Used for finding the highest priority mutators on a proxy
 */
struct FAtomProxyMutatorPriorities
{
	using PayloadFlags = AtomGameplay::EComponentPayload;

	TMap<FName, int32> PriorityMap;
	PayloadFlags PayloadType = PayloadFlags::AGCP_None;
	bool bFilterPayload = true;
};

/**
 *  FAtomProxyVolumeMutator - An Atom thread representation of the payload for an AtomGameplayVolumeComponent.
 */
class CRIWARECORE_API FAtomProxyVolumeMutator
	: public TSharedFromThis<FAtomProxyVolumeMutator>
{
public:

	FAtomProxyVolumeMutator();
	virtual ~FAtomProxyVolumeMutator() = default;

	using PayloadFlags = AtomGameplay::EComponentPayload;

	virtual void UpdatePriority(FAtomProxyMutatorPriorities& Priorities) const;
	virtual bool CheckPriority(const FAtomProxyMutatorPriorities& Priorities) const;

	virtual void Apply(FAtomInteriorSettings& InteriorSettings) const;
	virtual void Apply(FAtomProxyActiveSoundParams& Params) const {}
	virtual void Apply(FAtomGameplayVolumeListener& Listener) const {}

	virtual void Remove(FAtomProxyActiveSoundParams& Params) const {}
	virtual void Remove(FAtomGameplayVolumeListener& Listener) const {}

	bool HasPayloadType(PayloadFlags InType) const;

	int32 Priority = INDEX_NONE;
	uint32 VolumeID = INDEX_NONE;
	uint32 WorldID = INDEX_NONE;

	FName MutatorName;

	PayloadFlags PayloadType;

protected:

	constexpr static const TCHAR MutatorBaseName[] = TEXT("MutatorBase");
};

/**
 *  UAtomGameplayVolumeMutator - These components are used for more complex interactions with AtomGameplayVolumes.
 *		Currently, components inheriting this base can affect interior settings as well as active sounds or the Atom listener(s) inside the volume.
 *		See also: AtomFilterVolumeComponent, AtomAttenuationVolumeComponent, AtomBusSendComponent, AtomAisacVolumeComponent, and AtomReverbVolumeComponent
 */
UCLASS(Abstract, HideDropdown, meta = (IsBlueprintBase = false))
class CRIWARECORE_API UAtomGameplayVolumeMutator
	: public UAtomGameplayComponent
{
	GENERATED_BODY()

public:

	UAtomGameplayVolumeMutator(const FObjectInitializer& ObjectInitializer);
	virtual ~UAtomGameplayVolumeMutator() = default;

	UFUNCTION(BlueprintCallable, Category = "AtomGameplay")
	void SetPriority(int32 InPriority);

	int32 GetPriority() const { return Priority; }

	/** Create and fill the appropriate proxy mutator for this component */
	virtual TSharedPtr<FAtomProxyVolumeMutator> CreateMutator() const final;

protected:

	//~ Begin UAtomGameplayComponent interface 
	virtual void Enable() override;
	//~ End UAtomGameplayComponent interface
	
	/** Create this component's type of mutator */
	virtual TSharedPtr<FAtomProxyVolumeMutator> FactoryMutator() const;

	/** Override in child classes to copy additional data needed to mutators */
	virtual void CopyAtomDataToMutator(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const {}

	/** Notify our parent volume our proxy may need updating */
	void NotifyDataChanged() const;

	// The priority of this component.  In the case of overlapping volumes or multiple affecting components, the highest priority is chosen.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AtomGameplay", Meta = (AllowPrivateAccess = "true"))
	int32 Priority = 0;

private:

	/** Called for you during mutator creation. See CopyAtomDataToMutator for adding data to derived classes */
	void CopyAtomDataToMutatorBase(TSharedPtr<FAtomProxyVolumeMutator>& Mutator) const;
};
