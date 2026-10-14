#pragma once

#include "CoreMinimal.h"

#include "AtomActiveSound.h"
#include "Atom/Mixer/AtomMixerTrace.h"

/**
 * Class that tracks virtualized looping active sounds that are eligible to revive re-trigger
 * as long as no stop request is received from the game thread.
 */
struct CRIWARECORE_API FAtomVirtualLoop
{
private:
	float TimeSinceLastUpdate;
	float TimeVirtualized;
	float UpdateInterval;

	FAtomActiveSound* ActiveSound;

	/**
	  * Check if provided active sound is in audible range.
	  */
	static bool IsInAudibleRange(const FAtomActiveSound& InActiveSound, const FAtomRuntime* InAudioDevice = nullptr);

public:

	FAtomVirtualLoop();

	/**
	 * Check to see if listener move is far enough such that a check for virtual loop realization is necessary
	 */
	static bool ShouldListenerMoveForceUpdate(const FTransform& LastTransform, const FTransform& CurrentTransform);

	/**
	 * Checks if provided active sound is available to be virtualized.  If so, returns new active sound ready to be
	 * added to virtual loop management by parent audio device.
	 */
	static bool Virtualize(FAtomActiveSound& InActiveSound, bool bDoRangeCheck, FAtomVirtualLoop& OutVirtualLoop);
	static bool Virtualize(FAtomActiveSound& InActiveSound, FAtomRuntime& AudioDevice, bool bDoRangeCheck, FAtomVirtualLoop& OutVirtualLoop);

	/**
	 * Whether the virtual loop system is enabled or not
	 */
	static bool IsEnabled();

	/**
	 * Returns the internally-managed active sound
	 */
	FAtomActiveSound& GetActiveSound();

	/**
	 * Returns the time the sound has been virtualized
	 */
	float GetTimeVirtualized() const;

	/**
	 * Returns the wait interval being observed before next update
	 */
	float GetUpdateInterval() const;

	/**
	 */
	const FAtomActiveSound& GetActiveSound() const;

	/**
	 * Overrides the update interval to the provided length
	 */
	void CalculateUpdateInterval();

	/**
	 * Takes aggregate update delta and updates focus so that realization
	 * check can test if ready to play.
	 */
	void UpdateFocusData(float DeltaTime);

	/**
	  * Updates the loop and checks if ready to play (or 'realize').
	  * Returns whether or not the sound is ready to be realized.
	  */
	bool Update(float DeltaTime, bool bForceUpdate);

	/**
	 * Whether sound is virtualized by Atom SDK using a virtual voice and continues to use an AtomSource.
	 */
	bool IsUsingVirtualVoice() const;

	/**
	 * Whether sound was virtualized by concurency system.
	 */
	bool IsVirtualizedDueToConcurrency() const;

#if ATOM_PROFILERTRACE_ENABLED
	void OnTraceStarted();
#endif
};
