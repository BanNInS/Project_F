
#pragma once

#include "UObject/Interface.h"

#include "IAtomActiveSoundUpdate.generated.h"

// Forward Declarations 
struct FAtomActiveSound;
struct FAtomSoundParseParameters;

/** Interface for modifying Atom active sounds during their update */
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UAtomActiveSoundUpdate : public UInterface
{
	GENERATED_BODY()
};

class IAtomActiveSoundUpdate
{
	GENERATED_BODY()

public:

	/**
	 * Gathers interior data that can affect the active sound. Non-const as this step can be used to track state about the sound on the implementing object.
	 *
	 * @param ActiveSound	The active sound affected.
	 * @param ParseParams	The parameters to apply to the playback instances.
	 */
	virtual void GatherInteriorData(const FAtomActiveSound& ActiveSound, FAtomSoundParseParameters& ParseParams) {};

	/**
	 * Applies interior data previously collected to the active sound and parse parameters.
	 *
	 * @param ActiveSound	The active sound affected.
	 * @param ParseParams	The parameters to apply to the playback instances.
	 */
	virtual void ApplyInteriorSettings(const FAtomActiveSound& ActiveSound, FAtomSoundParseParameters& ParseParams) {};

	/**
	 * Called while an active sound is being constructed. Will be followed by either
	 * NotifyActiveSoundCreated or NotifyVirtualizedSoundCreated.
	 *
	 * NOTE! Called on the GameThread
	 *
	 * @param ActiveSound   The active sound being associated
	 * @param Owner         The owner it is associated with, or nullptr
	 */
	virtual void NotifyActiveSoundOwner(FAtomActiveSound& ActiveSound, const UObject* Owner) {}

	/**
	 * Called when an active sound has just been added to the audio engine,
	 * both for brand new sounds and for virtualized sounds that have just become active.
	 * In the latter case, a corresponding NotifyVirtualizedSoundDeleting will be received.
	 * You can correlate the two objects by matching their GetPlayOrder() value.
	 *
	 * NOTE! Called on the AudioThread
	 *
	 * @param ActiveSound	The active sound being created
	 */
	virtual void NotifyActiveSoundCreated(FAtomActiveSound& ActiveSound) {}

	/**
	 * Called when an active sound has just been removed from the audio engine, by being stopped or virtualized.
	 * In either case, the referenced ActiveSound object is about to be deleted; any pointers to it should be discarded.
	 *
	 * NOTE! Called on the AudioThread
	 *
	 * @param ActiveSound	The active sound being deleted
	 */
	virtual void NotifyActiveSoundDeleting(const FAtomActiveSound& ActiveSound) {}

	/**
	 * Called when a virtualized sound has just been added to the audio engine,
	 * both for brand new sounds and for active sounds that have just become virtualized.
	 * When virtualizing, the corresponding NotifyActiveSoundDeleting will arrive after any fade-out has finished.
	 * You can correlate the two objects by matching their GetPlayOrder() value.
	 *
	 * NOTE! Called on the AudioThread
	 *
	 * @param ActiveSound	The virtualized sound being created
	 */
	virtual void NotifyVirtualizedSoundCreated(FAtomActiveSound& ActiveSound) {}

	/**
	 * Called when a virtualized sound has just been removed from the audio engine, by being stopped or re-triggered.
	 * In either case, the referenced ActiveSound object is about to be deleted; any pointers to it should be discarded.
	 *
	 * NOTE! Called on the AudioThread
	 *
	 * @param ActiveSound	The active sound being removed
	 */
	virtual void NotifyVirtualizedSoundDeleting(const FAtomActiveSound& ActiveSound) {}

	UE_DEPRECATED(5.0, "OnNotifyPendingDelete is deprecated. Use NotifyActiveSoundDeleting and/or NotifyVirtualizedSoundDeleting instead.")
	virtual void OnNotifyPendingDelete(const FAtomActiveSound& ActiveSound) {};
};
