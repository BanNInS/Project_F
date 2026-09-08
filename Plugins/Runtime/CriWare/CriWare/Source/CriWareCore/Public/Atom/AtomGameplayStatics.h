#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AtomGameplayStatics.generated.h"

class UAtomComponent;
class UAtomAttenuation;
class UAtomSoundClass;
class UAtomInitialActiveSoundParams;

UCLASS(meta = (ScriptName = "AtomGameplayStatics"))
class CRIWARECORE_API UAtomGameplayStatics 
	: public UBlueprintFunctionLibrary 
{
	GENERATED_BODY()

public:
	// --- Sound functions ------------------------------
	
	/**
	 * Determines if any audio listeners are within range of the specified location
	 * @param Location		The location from which test if a listener is in range
	 * @param MaximumRange	The distance away from Location to test if any listener is within
	 * @note This will always return false if there is no Atom runtime, or the Atom runtime is disabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom", meta = (WorldContext = "WorldContextObject"))
	static bool AreAnyListenersWithinRange(const UObject* WorldContextObject, const FVector& Location, float MaximumRange);
	

	/**
	 * Finds and returns the position of the closest listener to the specified location
	 * @param Location						The location from which we'd like to find the closest listener, in world space.
	 * @param MaximumRange					The maximum distance away from Location that a listener can be.
	 * @param bAllowAttenuationOverride		True for the adjusted listener position (if attenuation override is set), false for the raw listener position (for panning)
	 * @param ListenerPosition				[Out] The position of the closest listener in world space, if found.
	 * @return true if we've successfully found a listener within MaximumRange of Location, otherwise false.
 	 * @note This will always return false if there is no Atom runtime, or the Atom runtime is disabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atom", meta = (WorldContext = "WorldContextObject"))
	static bool GetClosestListenerLocation(const UObject* WorldContextObject, const FVector& Location, float MaximumRange, const bool bAllowAttenuationOverride, FVector& ListenerPosition);

	/**
	* Sets a global pitch modulation scalar that will apply to all non-UI sounds
	*
	* * Fire and Forget.
	* * Not Replicated.
	* @param PitchModulation - A pitch modulation value to globally set.
	* @param TimeSec - A time value to linearly interpolate the global modulation pitch over from it's current value.
	*/
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Atom", meta = (WorldContext = "WorldContextObject"))
	static void SetGlobalPitchModulation(const UObject* WorldContextObject, float PitchModulation, float TimeSec);

	/** 
	* Linearly interpolates the attenuation distance scale value from it's current attenuation distance override value 
	* (1.0f it not overridden) to its new attenuation distance override, over the given amount of time
	*
	* * Fire and Forget.
	* * Not Replicated.
	* @param SoundClass - Sound class to to use to set the attenuation distance scale on.
	* @param DistanceAttenuationScale - A scalar for the attenuation distance used for computing distance attenuation. 
	* @param TimeSec - A time value to linearly interpolate from the current distance attenuation scale value to the new value.
	*/
	//UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Atom", meta = (WorldContext = "WorldContextObject"))
	//static void SetSoundClassDistanceScale(const UObject* WorldContextObject, USoundClass* SoundClass, float DistanceAttenuationScale, float TimeSec = 0.0f);


	/**
	* Sets the global listener focus parameters, which will scale focus behavior of sounds based on their focus azimuth
	* settings in their attenuation settings.
	*
	* * Fire and Forget.
	* * Not Replicated.
	* @param FocusAzimuthScale - An angle scale value used to scale the azimuth angle that defines where sounds are in-focus.
	* @param NonFocusAzimuthScale- An angle scale value used to scale the azimuth angle that defines where sounds are out-of-focus.
	* @param FocusDistanceScale - A distance scale value to use for sounds which are in-focus. Values < 1.0 will reduce perceived 
	*							  distance to sounds, values > 1.0 will increase perceived distance to in-focus sounds.
	* @param NonFocusDistanceScale - A distance scale value to use for sounds which are out-of-focus. Values < 1.0 will reduce 
	*								 perceived distance to sounds, values > 1.0 will increase perceived distance to in-focus sounds.
	* @param FocusVolumeScale- A volume attenuation value to use for sounds which are in-focus.
	* @param NonFocusVolumeScale- A volume attenuation value to use for sounds which are out-of-focus.
	* @param FocusPriorityScale - A priority scale value (> 0.0) to use for sounds which are in-focus. Values < 1.0 will reduce
	*							  the priority of in-focus sounds, values > 1.0 will increase the priority of in-focus sounds.
	* @param NonFocusPriorityScale - A priority scale value (> 0.0) to use for sounds which are out-of-focus. Values < 1.0 will
	*								 reduce the priority of sounds out-of-focus sounds, values > 1.0 will increase the priority of
	*								 out-of-focus sounds.
	*/
	//UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Atom", meta = (WorldContext = "WorldContextObject"))
	//static void SetGlobalListenerFocusParameters(const UObject* WorldContextObject, float FocusAzimuthScale = 1.0f, float NonFocusAzimuthScale = 1.0f, float FocusDistanceScale = 1.0f, float NonFocusDistanceScale = 1.0f, float FocusVolumeScale = 1.0f, float NonFocusVolumeScale = 1.0f, float FocusPriorityScale = 1.0f, float NonFocusPriorityScale = 1.0f);

	/**
	 * Plays a sound directly with no attenuation, perfect for UI sounds.
	 *
	 * * Fire and Forget.
	 * * Not Replicated.
	 * @param Sound - Sound to play.
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the sound to begin playback at
 	 * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param OwningActor - The actor to use as the "owner" for concurrency settings purposes. 
	 *						Allows PlaySound calls to do a concurrency limit per owner.
	 * @param bIsUISound - True if sound is UI related, else false
	 */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Atom", meta=( WorldContext="WorldContextObject", AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true" ))
	static void PlaySound2D(const UObject* WorldContextObject, UAtomSoundBase* Sound, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, UAtomConcurrency* ConcurrencySettings = nullptr, const AActor* OwningActor = nullptr, bool bIsUISound = true);

	/**
	 * This function allows users to create Atom Components with settings specifically for non-spatialized,
	 * non-distance-attenuated sounds. Atom Components created using this function by default will not have 
	 * Spatialization applied. Sound instances will begin playing upon spawning this Atom Component.
	 *
	 * * Not Replicated.
	 * @param Sound - Sound to play.
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the sound to begin playback at
	 * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param PersistAcrossLevelTransition - Whether the sound should continue to play when the map it was played in is unloaded
	 * @param bAutoDestroy - Whether the returned Atom Component will be automatically cleaned up when the sound finishes 
	 *						 (by completing or stopping) or whether it can be reactivated
	 * @return An Atom Component to manipulate the spawned sound
	 */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Atom", meta=( WorldContext="WorldContextObject", AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true", Keywords = "play" ))
	static UAtomComponent* SpawnSound2D(const UObject* WorldContextObject, UAtomSoundBase* Sound, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, UAtomConcurrency* ConcurrencySettings = nullptr, bool bPersistAcrossLevelTransition = false, bool bAutoDestroy = true);

	/**
	 * This function allows users to create Atom Components in advance of playback with settings specifically for non-spatialized,
	 * non-distance-attenuated sounds. Atom Components created using this function by default will not have Spatialization applied.
	 * @param Sound - Sound to create.
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far into the sound to begin playback at
	 * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param PersistAcrossLevelTransition - Whether the sound should continue to play when the map it was played in is unloaded
	 * @param bAutoDestroy - Whether the returned Atom Component will be automatically cleaned up when the sound finishes 
	 *						 (by completing or stopping), or whether it can be reactivated
	 * @return An Atom Component to manipulate the created sound
	 */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Atom", meta=( WorldContext="WorldContextObject", AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true", Keywords = "play" ))
	static UAtomComponent* CreateSound2D(const UObject* WorldContextObject, UAtomSoundBase* Sound, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, UAtomConcurrency* ConcurrencySettings = nullptr, bool bPersistAcrossLevelTransition = false, bool bAutoDestroy = true);

	/**
	 * Plays a sound at the given location. This is a fire and forget sound and does not travel with any actor. 
	 * Replication is also not handled at this point.
	 * @param Sound - sound to play
	 * @param Location - World position to play sound at
	 * @param Rotation - World rotation to play sound at
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the sound to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play sound with
	 * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param OwningActor - The actor to use as the "owner" for concurrency settings purposes. Allows PlaySound calls
	 *						to do a concurrency limit per owner.
	 */
	UFUNCTION(BlueprintCallable, Category="Atom", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	static void PlaySoundAtLocation(const UObject* WorldContextObject, UAtomSoundBase* Sound, FVector Location, FRotator Rotation, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, class UAtomAttenuation* AttenuationSettings = nullptr, UAtomConcurrency* ConcurrencySettings = nullptr, const AActor* OwningActor = nullptr, UAtomInitialActiveSoundParams* InitialParams = nullptr);

	static void PlaySoundAtLocation(const UObject* WorldContextObject, UAtomSoundBase* Sound, FVector Location, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, class UAtomAttenuation* AttenuationSettings = nullptr, UAtomConcurrency* ConcurrencySettings = nullptr, UAtomInitialActiveSoundParams* InitialParams = nullptr)
	{
		PlaySoundAtLocation(WorldContextObject, Sound, Location, FRotator::ZeroRotator, VolumeMultiplier, PitchMultiplier, StartTime, AttenuationSettings, ConcurrencySettings, nullptr, InitialParams);
	}

	/**
	 * Spawns a sound at the given location. This does not travel with any actor. Replication is also not handled at this point.
	 * @param Sound - sound to play
	 * @param Location - World position to play sound at
	 * @param Rotation - World rotation to play sound at
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the sound to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play sound with
	 * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param bAutoDestroy - Whether the returned Atom Component will be automatically cleaned up when the sound finishes 
	 *						 (by completing or stopping) or whether it can be reactivated
	 * @return An Atom Component to manipulate the spawned sound
	 */
	UFUNCTION(BlueprintCallable, Category="Atom", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	static UAtomComponent* SpawnSoundAtLocation(const UObject* WorldContextObject, UAtomSoundBase* Sound, FVector Location, FRotator Rotation = FRotator::ZeroRotator, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, class UAtomAttenuation* AttenuationSettings = nullptr, UAtomConcurrency* ConcurrencySettings = nullptr, bool bAutoDestroy = true);

	/** This function allows users to create and play Atom Components attached to a specific Scene Component. 
	 *  Useful for spatialized and/or distance-attenuated sounds that need to follow another object in space.
	 * @param Sound - sound to play
	 * @param AttachComponent - Component to attach to.
	 * @param AttachPointName - Optional named point within the AttachComponent to play the sound at
	 * @param Location - Depending on the value of Location Type this is either a relative offset from 
	 *					 the attach component/point or an absolute world position that will be translated to a relative offset
	 * @param Rotation - Depending on the value of Location Type this is either a relative offset from
	 *					 the attach component/point or an absolute world rotation that will be translated to a relative offset
	 * @param LocationType - Specifies whether Location is a relative offset or an absolute world position
	 * @param bStopWhenAttachedToDestroyed - Specifies whether the sound should stop playing when the
	 *										 owner of the attach to component is destroyed.
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the sound to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play sound with
	 * @param ConcurrencySettings - Override concurrency settings package to play sound with
	 * @param bAutoDestroy - Whether the returned Atom Component will be automatically cleaned up when the sound finishes
	 *						 (by completing or stopping) or whether it can be reactivated
	 * @return An Atom Component to manipulate the spawned sound
	 */
	UFUNCTION(BlueprintCallable, Category="Atom", meta=(AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	static UAtomComponent* SpawnSoundAttached(UAtomSoundBase* Sound, USceneComponent* AttachToComponent, FName AttachPointName = NAME_None, FVector Location = FVector(ForceInit), FRotator Rotation = FRotator::ZeroRotator, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bStopWhenAttachedToDestroyed = false, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, UAtomAttenuation* AttenuationSettings = nullptr, UAtomConcurrency* ConcurrencySettings = nullptr, bool bAutoDestroy = true);

	static class UAtomComponent* SpawnSoundAttached(UAtomSoundBase* Sound, USceneComponent* AttachToComponent, FName AttachPointName, FVector Location, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bStopWhenAttachedToDestroyed = false, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, UAtomAttenuation* AttenuationSettings = nullptr, UAtomConcurrency* ConcurrencySettings = nullptr, bool bAutoDestroy = true)
	{
		return SpawnSoundAttached(Sound, AttachToComponent, AttachPointName, Location, FRotator::ZeroRotator, LocationType, bStopWhenAttachedToDestroyed, VolumeMultiplier, PitchMultiplier, StartTime, AttenuationSettings, ConcurrencySettings, bAutoDestroy);
	}

	/**
	 * Plays a dialogue directly with no attenuation, perfect for UI.
	 *
	 * * Fire and Forget.
	 * * Not Replicated.
	 * @param Dialogue - dialogue to play
	 * @param Context - context the dialogue is to play in
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the dialogue to begin playback at
	 */
	//UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Atom", meta=( WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true" ))
	//static void PlayDialogue2D(const UObject* WorldContextObject, UDialogueWave* Dialogue, const struct FDialogueContext& Context, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f);

	/**
	 * Spawns a DialogueWave, a special type of Asset that requires Context data in order to resolve a specific SoundBase,
	 * which is then passed on to the new Atom Component. Atom Components created using this function by default will not
	 * have Spatialization applied. Sound instances will begin playing upon spawning this Atom Component.
	 *
	 * * Not Replicated.
	 * @param Dialogue - dialogue to play
	 * @param Context - context the dialogue is to play in
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the dialogue to begin playback at
	 * @param bAutoDestroy - Whether the returned Atom Component will be automatically cleaned up when the sound
	 *						 finishes (by completing or stopping) or whether it can be reactivated
	 * @return An Atom Component to manipulate the spawned sound
	 */
	//UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Atom", meta=( WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play" ))
	//static UAtomComponent* SpawnDialogue2D(const UObject* WorldContextObject, UDialogueWave* Dialogue, const struct FDialogueContext& Context, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, bool bAutoDestroy = true);

	/** Plays a dialogue at the given location. This is a fire and forget sound and does not travel with any actor. 
	 *	Replication is also not handled at this point.
	 * @param Dialogue - dialogue to play
	 * @param Context - context the dialogue is to play in
	 * @param Location - World position to play dialogue at
	 * @param Rotation - World rotation to play dialogue at
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the dialogue to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play sound with
	 */
	//UFUNCTION(BlueprintCallable, Category="Atom", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "4", UnsafeDuringActorConstruction = "true"))
	//static void PlayDialogueAtLocation(const UObject* WorldContextObject, class UDialogueWave* Dialogue, const FDialogueContext& Context, FVector Location, FRotator Rotation, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, UAtomAttenuation* AttenuationSettings = nullptr);

	//static void PlayDialogueAtLocation(const UObject* WorldContextObject, UDialogueWave* Dialogue, const FDialogueContext& Context, FVector Location, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, UAtomAttenuation* AttenuationSettings = nullptr)
	//{
	//	PlayDialogueAtLocation(WorldContextObject, Dialogue, Context, Location, FRotator::ZeroRotator, VolumeMultiplier, PitchMultiplier, StartTime, AttenuationSettings);
	//}

	/** Spawns a DialogueWave, a special type of Asset that requires Context data in order to resolve a specific SoundBase,
	 *  which is then passed on to the new Atom Component. This function allows users to create and play Atom Components at a
	 *  specific World Location and Rotation. Useful for spatialized and/or distance-attenuated dialogue.
	 * @param Dialogue - Dialogue to play
	 * @param Context - Context the dialogue is to play in
	 * @param Location - World position to play dialogue at
	 * @param Rotation - World rotation to play dialogue at
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far into the dialogue to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play sound with
	 * @param bAutoDestroy - Whether the returned Atom Component will be automatically cleaned up when the sound finishes
	 *						 (by completing or stopping) or whether it can be reactivated
	 * @return Atom Component to manipulate the playing dialogue with
	 */
	//UFUNCTION(BlueprintCallable, Category="Atom", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "4", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	//static UAtomComponent* SpawnDialogueAtLocation(const UObject* WorldContextObject, UDialogueWave* Dialogue, const struct FDialogueContext& Context, FVector Location, FRotator Rotation = FRotator::ZeroRotator, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, UAtomAttenuation* AttenuationSettings = nullptr, bool bAutoDestroy = true);

	/** Spawns a DialogueWave, a special type of Asset that requires Context data in order to resolve a specific SoundBase,
	 *	which is then passed on to the new Atom Component. This function allows users to create and play Atom Components
	 *	attached to a specific Scene Component. Useful for spatialized and/or distance-attenuated dialogue that needs to 
	 *	follow another object in space.
	 * @param Dialogue - dialogue to play
	 * @param Context - context the dialogue is to play in
	 * @param AttachComponent - Component to attach to.
	 * @param AttachPointName - Optional named point within the AttachComponent to play the sound at
	 * @param Location - Depending on the value of Location Type this is either a relative offset from the 
	 *					 attach component/point or an absolute world position that will be translated to a relative offset
	 * @param Rotation - Depending on the value of Location Type this is either a relative offset from the 
	 *					 attach component/point or an absolute world rotation that will be translated to a relative offset
	 * @param LocationType - Specifies whether Location is a relative offset or an absolute world position
	 * @param bStopWhenAttachedToDestroyed - Specifies whether the sound should stop playing when the owner its attached
	 *										 to is destroyed.
	 * @param VolumeMultiplier - A linear scalar multiplied with the volume, in order to make the sound louder or softer.
	 * @param PitchMultiplier - A linear scalar multiplied with the pitch.
	 * @param StartTime - How far in to the dialogue to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play sound with
	 * @param bAutoDestroy - Whether the returned Atom Component will be automatically cleaned up when the sound finishes 
	 *						 (by completing or stopping) or whether it can be reactivated
	 * @return Atom Component to manipulate the playing dialogue with
	 */
	//UFUNCTION(BlueprintCallable, Category="Atom", meta=(AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	//static UAtomComponent* SpawnDialogueAttached(UDialogueWave* Dialogue, const FDialogueContext& Context, USceneComponent* AttachToComponent, FName AttachPointName = NAME_None, FVector Location = FVector(ForceInit), FRotator Rotation = FRotator::ZeroRotator, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bStopWhenAttachedToDestroyed = false, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, UAtomAttenuation* AttenuationSettings = nullptr, bool bAutoDestroy = true);

	//static UAtomComponent* SpawnDialogueAttached(UDialogueWave* Dialogue, const FDialogueContext& Context, USceneComponent* AttachToComponent, FName AttachPointName, FVector Location, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bStopWhenAttachedToDestroyed = false, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, UAtomAttenuation* AttenuationSettings = nullptr, bool bAutoDestroy = true)
	//{
	//	return SpawnDialogueAttached(Dialogue, Context, AttachToComponent, AttachPointName, Location, FRotator::ZeroRotator, LocationType, bStopWhenAttachedToDestroyed, VolumeMultiplier, PitchMultiplier, StartTime, AttenuationSettings, bAutoDestroy);
	//}

	/** Plays a force feedback effect at the given location. This is a fire and forget effect and does not travel with any actor. Replication is also not handled at this point.
	 * @param ForceFeedbackEffect - effect to play
	 * @param Location - World position to center the effect at
	 * @param Rotation - World rotation to center the effect at
	 * @param IntensityMultiplier - Intensity multiplier 
	 * @param StartTime - How far in to the feedback effect to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play effect with
	 * @param bAutoDestroy - Whether the returned force feedback component will be automatically cleaned up when the feedback pattern finishes (by completing or stopping) or whether it can be reactivated
	 * @return Force Feedback Component to manipulate the playing feedback effect with
	 */
	//UFUNCTION(BlueprintCallable, Category="ForceFeedback", meta=(WorldContext="WorldContextObject", AdvancedDisplay = "3", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	//static UForceFeedbackComponent* SpawnForceFeedbackAtLocation(const UObject* WorldContextObject, UForceFeedbackEffect* ForceFeedbackEffect, FVector Location, FRotator Rotation = FRotator::ZeroRotator, bool bLooping = false, float IntensityMultiplier = 1.f, float StartTime = 0.f, UForceFeedbackAttenuation* AttenuationSettings = nullptr, bool bAutoDestroy = true);

	/** Plays a force feedback effect attached to and following the specified component. This is a fire and forget effect. Replication is also not handled at this point.
	 * @param ForceFeedbackEffect - effect to play
	 * @param AttachComponent - Component to attach to.
	 * @param AttachPointName - Optional named point within the AttachComponent to attach to
	 * @param Location - Depending on the value of Location Type this is either a relative offset from the attach component/point or an absolute world position that will be translated to a relative offset
	 * @param Rotation - Depending on the value of Location Type this is either a relative offset from the attach component/point or an absolute world rotation that will be translated to a relative offset
	 * @param LocationType - Specifies whether Location is a relative offset or an absolute world position
	 * @param bStopWhenAttachedToDestroyed - Specifies whether the feedback effect should stop playing when the owner of the attach to component is destroyed.
	 * @param IntensityMultiplier - Intensity multiplier 
	 * @param StartTime - How far in to the feedback effect to begin playback at
	 * @param AttenuationSettings - Override attenuation settings package to play effect with
	 * @param bAutoDestroy - Whether the returned force feedback component will be automatically cleaned up when the feedback patern finishes (by completing or stopping) or whether it can be reactivated
	 * @return Force Feedback Component to manipulate the playing feedback effect with
	*/
	//UFUNCTION(BlueprintCallable, Category="ForceFeedback", meta=(AdvancedDisplay = "2", UnsafeDuringActorConstruction = "true", Keywords = "play"))
	//static UForceFeedbackComponent* SpawnForceFeedbackAttached(UForceFeedbackEffect* ForceFeedbackEffect, USceneComponent* AttachToComponent, FName AttachPointName = NAME_None, FVector Location = FVector(ForceInit), FRotator Rotation = FRotator::ZeroRotator, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bStopWhenAttachedToDestroyed = false, bool bLooping = false, float IntensityMultiplier = 1.f, float StartTime = 0.f, class UForceFeedbackAttenuation* AttenuationSettings = nullptr, bool bAutoDestroy = true);

	/**
	 * Will set subtitles to be enabled or disabled.
	 * @param bEnabled will enable subtitle drawing if true, disable if false.
	 */
	//UFUNCTION(BlueprintCallable, Category = "Audio|Subtitles")
	//static void SetSubtitlesEnabled(bool bEnabled);

	/**
	 * Returns whether or not subtitles are currently enabled.
	 * @return true if subtitles are enabled.
	 */
	//UFUNCTION(BlueprintPure, Category = "Audio|Subtitles")
	//static bool AreSubtitlesEnabled();

	// --- Audio Functions ----------------------------
	/** Set the sound mix of the audio system for special EQing */
	//UFUNCTION(BlueprintCallable, Category="Atom", meta=(WorldContext = "WorldContextObject"))
	//static void SetBaseSoundMix(const UObject* WorldContextObject, class USoundMix* InSoundMix);

	/** Primes the sound, caching the needed data for fast playback with streamed sounds. */
	UFUNCTION(BlueprintCallable, Category = "Atom")
	static void PrimeSound(UAtomSoundBase* InSound);

	/** Primes the sound in the given UAtomSoundClass, caching the needed data for fast playback with streamed sounds. */
	UFUNCTION(BlueprintCallable, Category = "Atom")
	static void PrimeAllSoundsInSoundClass(class UAtomSoundClass* InSoundClass);

	/** Iterate through all sound waves and releases handles to retained chunks. (If the chunk is not being played it will be up for eviction) */
	//UFUNCTION(BlueprintCallable, Category = "Atom")
	//static void UnRetainAllSoundsInSoundClass(class UAtomSoundClass* InSoundClass);

	/** Overrides the sound class adjuster in the given sound mix. If the sound class does not exist in the input sound mix, 
	 *	the sound class adjuster will be added to the list of active sound mix modifiers. 
	 * @param InSoundMixModifier The sound mix to modify.
	 * @param InSoundClass The sound class to override (or add) in the sound mix.
	 * @param Volume The volume scale to set the sound class adjuster to.
	 * @param Pitch The pitch scale to set the sound class adjuster to.
	 * @param FadeInTime The interpolation time to use to go from the current sound class adjuster values to the new values.
	 * @param bApplyToChildren Whether or not to apply this override to the sound class' children or to just the specified sound class.
	 */
	//UFUNCTION(BlueprintCallable, Category = "Atom", meta = (WorldContext = "WorldContextObject"))
	//static void SetSoundMixClassOverride(const UObject* WorldContextObject, class USoundMix* InSoundMixModifier, class USoundClass* InSoundClass, float Volume = 1.0f, float Pitch = 1.0f, float FadeInTime = 1.0f, bool bApplyToChildren = true);

	/** Clears any existing override of the Sound Class Adjuster in the given Sound Mix
	 * @param InSoundMixModifier The sound mix to modify.
	 * @param InSoundClass The sound class in the sound mix to clear overrides from.
	 * @param FadeOutTime The interpolation time to use to go from the current sound class adjuster override values to the non-override values.
	*/
	//UFUNCTION(BlueprintCallable, Category = "Atom", meta = (WorldContext = "WorldContextObject"))
	//static void ClearSoundMixClassOverride(const UObject* WorldContextObject, class USoundMix* InSoundMixModifier, class USoundClass* InSoundClass, float FadeOutTime = 1.0f);

	/** Push a sound mix modifier onto the audio system 
	 * @param InSoundMixModifier The Sound Mix Modifier to add to the system
	 */
	//UFUNCTION(BlueprintCallable, Category="Atom", meta=(WorldContext = "WorldContextObject"))
	//static void PushSoundMixModifier(const UObject* WorldContextObject, class USoundMix* InSoundMixModifier);

	/** Pop a sound mix modifier from the audio system 
	 *	@param InSoundMixModifier The Sound Mix Modifier to remove from the system
	 */
	//UFUNCTION(BlueprintCallable, Category="Atom", meta=(WorldContext = "WorldContextObject"))
	//static void PopSoundMixModifier(const UObject* WorldContextObject, class USoundMix* InSoundMixModifier);

	/** Clear all sound mix modifiers from the audio system */
	//UFUNCTION(BlueprintCallable, Category="Atom", meta=(WorldContext = "WorldContextObject"))
	//static void ClearSoundMixModifiers(const UObject* WorldContextObject);

	/** Activates a Reverb Effect without the need for an Audio Volume
	 * @param ReverbEffect Reverb Effect to use
	 * @param TagName Tag to associate with Reverb Effect
	 * @param Priority Priority of the Reverb Effect
	 * @param Volume Volume level of Reverb Effect
	 * @param FadeTime Time before Reverb Effect is fully active
	 */
	//UFUNCTION(BlueprintCallable, Category="Atom", meta=(WorldContext = "WorldContextObject", AdvancedDisplay = "2"))
	//static void ActivateReverbEffect(const UObject* WorldContextObject, class UReverbEffect* ReverbEffect, FName TagName, float Priority = 0.f, float Volume = 0.5f, float FadeTime = 2.f);

	/**
	 * Deactivates a Reverb Effect that was applied outside of an Audio Volume
	 *
	 * @param TagName Tag associated with Reverb Effect to remove
	 */
	//UFUNCTION(BlueprintCallable, Category="Atom", meta=(WorldContext = "WorldContextObject"))
	//static void DeactivateReverbEffect(const UObject* WorldContextObject, FName TagName);

	/** 
	 * Returns the highest priority reverb settings currently active from any source (Audio Volumes or manual settings). 
	 */
	//UFUNCTION(BlueprintCallable, Category = "Atom", meta = (WorldContext = "WorldContextObject"))
	//static class UReverbEffect* GetCurrentReverbEffect(const UObject* WorldContextObject);

	/**
	 * Sets the max number of voices (also known as "channels") dynamically by percentage. E.g. if you want to temporarily
	 * reduce voice count by 50%, use 0.50. Later, you can return to the original max voice count by using 1.0.
	 * @param MaxChannelCountScale The percentage of the original voice count to set the max number of voices to
	 */
	//UFUNCTION(BlueprintCallable, Category = "Atom", meta = (WorldContext = "WorldContextObject"))
	//static void SetMaxAudioChannelsScaled(const UObject* WorldContextObject, float MaxChannelCountScale);

	/**
	 * Retrieves the max voice count currently used by the audio engine.
	 */
	//UFUNCTION(BlueprintCallable, Category = "Atom", meta = (WorldContext = "WorldContextObject"))
	//static int32 GetMaxAudioChannelCount(const UObject* WorldContextObject);
};
