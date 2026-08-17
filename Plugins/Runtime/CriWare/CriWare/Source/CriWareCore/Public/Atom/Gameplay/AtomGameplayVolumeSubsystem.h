
#pragma once

#include "Templates/SharedPointer.h"

#include "Atom/Interfaces/IAtomActiveSoundUpdate.h"
#include "Atom/AtomEngineSubsystem.h"
#include "AtomGameplayFlags.h"
#include "AtomGameplayVolumeListener.h"

#include "AtomGameplayVolumeSubsystem.generated.h"

// Forward Declarations 
class UWorld;
class FAtomProxyVolumeMutator;
class UAtomGameplayVolumeProxy;
class UAtomGameplayVolumeComponent;
class UPrimitiveComponent;

/**
 *  FAtomGameplayActiveSoundInfo - Helper struct for caching active sound data
 */
struct FAtomGameplayActiveSoundInfo
{
	// Interior settings & interpolation values
	FAtomInterpolatedInteriorSettings InteriorSettings;
	double LastUpdateTime = 0.0;

	// Current interior values
	float CurrentInteriorVolume = 1.f;
	float SourceInteriorVolume = 1.f;
	float CurrentInteriorLPF = MAX_FILTER_FREQUENCY;
	float SourceInteriorLPF = MAX_FILTER_FREQUENCY;

	TArray<TSharedPtr<FAtomProxyVolumeMutator>> CurrentMutators;

	void Update(double ListenerInteriorStartTime);
};

/**
 *  FAudioGameplayProxyUpdateResult - Helper struct for updating game thread proxies
 */
struct FAtomGameplayProxyUpdateResult
{
	TSet<uint32> EnteredProxies;
	TSet<uint32> ExitedProxies;
	bool bForceUpdate = false;
};

/**
 *  FAudioProxyMutatorSearchResult - Results from a audio proxy mutator search (see below).
 */
struct FAtomProxyMutatorSearchResult
{
	TSet<uint32> VolumeSet;
	TArray<TSharedPtr<FAtomProxyVolumeMutator>> MatchingMutators;
	FAtomReverbSettings ReverbSettings;
	FAtomInteriorSettings InteriorSettings;

	void Reset()
	{
		VolumeSet.Reset();
		MatchingMutators.Empty();
		ReverbSettings = FAtomReverbSettings();
		InteriorSettings = FAtomInteriorSettings();
	}
};

/**
 *  FAtomProxyMutatorSearchObject - Used for searching through proxy volumes to find relevant proxy mutators
 */
struct FAtomProxyMutatorSearchObject
{
	using PayloadFlags = AtomGameplay::EComponentPayload;

	// Search parameters
	uint32 WorldID = INDEX_NONE;
	FVector Location = FVector::ZeroVector;
	PayloadFlags PayloadType = PayloadFlags::AGCP_None;
	FAtomRuntimeHandle AtomRuntimeHandle;
	bool bFilterPayload = true;
	bool bCollectMutators = true;
	bool bGetDefaultAtomSettings = true;

	void SearchVolumes(const TArray<UAtomGameplayVolumeProxy*>& ProxyVolumes, FAtomProxyMutatorSearchResult& OutResult);
};

/**
 *  FAtomGameplayVolumeProxyInfo - Holds information relating to which volumes our listeners are inside of
 */
class FAtomGameplayVolumeProxyInfo
{
public:

	FAtomGameplayVolumeProxyInfo() = default;

	/** Aggregate proxies from all listeners into a single world list */
	void Update(const TArray<FAtomGameplayVolumeListener>& VolumeListeners, FAtomGameplayProxyUpdateResult& OutResult);

	/** Add a listener's index to be checked on the next Update call */
	void AddListenerIndex(int32 ListenerIndex);

	/** Returns true if the volume contains at least one listener */
	bool IsVolumeInCurrentList(uint32 VolumeID) const;

protected:

	TSet<uint32> CurrentProxies;
	TSet<uint32> PreviousProxies;

	TArray<int32> ListenerIndexes;
};

/**
 *  UAtomGameplayVolumeSubsystem
 */
UCLASS()
class CRIWARECORE_API UAtomGameplayVolumeSubsystem
	: public UAtomEngineSubsystem
	, public IAtomActiveSoundUpdate
{
	GENERATED_BODY()

public: 

	UAtomGameplayVolumeSubsystem();
	virtual ~UAtomGameplayVolumeSubsystem() = default;

	//~ Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	//~ End USubsystem interface

	//~ Begin UAtomEngineSubsystem interface
	virtual void Update() override;
	//~ End UAtomEngineSubsystem interface

	//~ Begin IAtomActiveSoundUpdateInterface
	virtual void GatherInteriorData(const FAtomActiveSound& ActiveSound, FAtomSoundParseParameters& ParseParams) override;
	virtual void ApplyInteriorSettings(const FAtomActiveSound& ActiveSound, FAtomSoundParseParameters& ParseParams) override;
	virtual void NotifyActiveSoundDeleting(const FAtomActiveSound& ActiveSound) override;
	//~ End IAtomActiveSoundUpdateInterface
	
	/** Add a volume to the system */
	void AddVolumeComponent(const UAtomGameplayVolumeComponent* VolumeComponent);

	/** Update an existing volume in the system */
	void UpdateVolumeComponent(const UAtomGameplayVolumeComponent* VolumeComponent);

	/** Remove a volume from the system */
	void RemoveVolumeComponent(const UAtomGameplayVolumeComponent* VolumeComponent);

	/** Gets the primitive component from the volume or null if volume proxy. */
	UPrimitiveComponent* GetPrimitiveComponent(const UAtomGameplayVolumeComponent* VolumeComponent);

	/** Gets the Volume form its VolumeID if exist or null. */
	const UAtomGameplayVolumeComponent* GetVolumeComponent(uint32 VolumeID);

	const void IterateOverVolumeComponents(TUniqueFunction<void(uint32, const UAtomGameplayVolumeComponent*)> ForEachComponent)
	{
		for (const auto& [VolumeID, AGVC] : AGVComponents)
		{
			ForEachComponent(VolumeID, AGVC);
		}
	}

protected:

	/** Returns true if we allow volumes from the world's type. */
	bool DoesSupportWorld(UWorld* World) const;

	/** (Atom Thread Only) Add, Update, Remove ProxyVolumes. */
	bool AddProxy(uint32 AtomGameplayVolumeID, uint32 WorldID);
	bool UpdateProxy(uint32 AtomGameplayVolumeID);
	bool RemoveProxy(uint32 AtomGameplayVolumeID);

	/** Returns true if a listener associated with WorldID is inside the volume (by ID). */
	bool IsAnyListenerInVolume(uint32 WorldID, uint32 VolumeID) const;

	/** Update the components driven by proxies on the game thread. */
	void UpdateComponentsFromProxyInfo(const FAtomGameplayProxyUpdateResult& ProxyResults) const;

	/** Update our representation of Atom listeners on the Atom thread. */
	void UpdateFromListeners();

	/** Generate a list of volume proxies from our component list */
	void GenerateVolumeProxyList();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAtomGameplayVolumeProxy>> TransientProxyList;

	// Components in our system.
	UPROPERTY(Transient)
	TMap<uint32, TObjectPtr<const UAtomGameplayVolumeComponent>> AGVComponents;

	// List of Component IDs (for proxies) the Atom thread is tracking
	TArray<uint32> KnownProxyIDs;

	// Atom thread representation of Listeners.
	TArray<FAtomGameplayVolumeListener> AGVListeners;

	// A collection of data about currently playing active sounds, indexed by the sound's unique ID
	TMap<uint32, FAtomGameplayActiveSoundInfo> ActiveSoundData;

	// A collection of listener & volume intersection data, by worldID
	TMap<uint32, FAtomGameplayVolumeProxyInfo> WorldProxyLists;

	// Time since last update call, in seconds
	float TimeSinceUpdate = 0.f;

	// Time when next update happens relative to last update
	float NextUpdateDeltaTime = 0.f;

	// The number of proxy volumes from the previous update
	int32 PreviousProxyCount = 0;

	// Force an enter / exit of a volume to handle data changing while a proxy is active
	bool bHasStaleProxy = false;
};
