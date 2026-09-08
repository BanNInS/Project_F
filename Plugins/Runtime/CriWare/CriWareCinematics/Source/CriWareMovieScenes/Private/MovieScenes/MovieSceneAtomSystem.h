/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : Movie Scene Atom System
 * File     : MovieSceneAtomSystem.h
 *
 ****************************************************************************/

#pragma once

#include "CoreTypes.h"
#include "EntitySystem/MovieSceneEntityIDs.h"
#include "UObject/ObjectMacros.h"
#include "EntitySystem/MovieSceneEntitySystem.h"

#include "MovieSceneAtomComponentTypes.h"
//#include "MovieSceneAtomTriggerChannel.h"

#include "MovieSceneAtomSystem.generated.h"


// Forward Definitions
//class IAtomParameterControllerInterface;
class UAtomComponent;
class UMovieSceneAtomSection;
class UAtomSoundBase;
struct FMoveSceneAtomTriggerState;

namespace UE::MovieScene
{
	struct FGatherAtomInputs;
	struct FGatherAtomTriggers;
	struct FEvaluateAtom;
	struct FPreAnimatedAtomStorage;

	struct FAtomComponentInputEvaluationData
	{
		TMap<FName, float> Inputs_Float;
		TMap<FName, FString> Inputs_String;
		TMap<FName, bool> Inputs_Bool;
		TMap<FName, int32> Inputs_Int;
		TArray<FName> Inputs_Trigger;
	};

	struct FAtomComponentEvaluationData
	{
		/** The Atom component that was created to play audio. */
		TWeakObjectPtr<UAtomComponent> AtomComponent;

		/** Volume multiplier to use this frame. */
		double VolumeMultiplier;

		/** Pitch multiplier to use this frame. */
		double PitchMultiplier;

		/**
		 * Set whenever we ask the Atom component to start playing a sound.
		 * Used to detect desyncs caused when Sequencer evaluates at more-than-real-time.
		 */
		TOptional<float> PartialDesyncComputation;

		/** Flag to keep track of Atom components evaluated on a given frame. */
		bool bEvaluatedThisFrame;
	};
}

/**
 * System for evaluating Atom tracks
 */
UCLASS()
class UMovieSceneAtomSystem : public UMovieSceneEntitySystem
{
	GENERATED_BODY()

public:

	using FInstanceHandle = UE::MovieScene::FInstanceHandle;
	using FMovieSceneEntityID = UE::MovieScene::FMovieSceneEntityID;
	using FAtomComponentEvaluationData = UE::MovieScene::FAtomComponentEvaluationData;
	using FAtomComponentInputEvaluationData = UE::MovieScene::FAtomComponentInputEvaluationData;

	UMovieSceneAtomSystem(const FObjectInitializer& ObjInit);

	//~ UMovieSceneEntitySystem members
	virtual void OnLink() override;
	virtual void OnUnlink() override;
	virtual void OnSchedulePersistentTasks(UE::MovieScene::IEntitySystemScheduler* TaskScheduler) override;
	virtual void OnRun(FSystemTaskPrerequisites& InPrerequisites, FSystemSubsequentTasks& Subsequents) override;

	/**
	 * Get the evaluation data for the given actor and section. Pass a null actor key for root (world) audio.
	 */
	FAtomComponentEvaluationData* GetAtomComponentEvaluationData(FInstanceHandle InstanceHandle, FObjectKey ActorKey, FObjectKey SectionKey);

	/**
	 * Adds an Atom component to the given bound sequencer object.
	 * WARNING: Only to be called on the game thread.
	 */
	FAtomComponentEvaluationData* AddBoundObjectAtomComponent(FInstanceHandle InstanceHandle, UMovieSceneAtomSection* Section, UObject* PrincipalObject);

	/**
	 * Adds an Atom component to the world, for playing root Atom tracks.
	 * WARNING: Only to be called on the game thread.
	 */
	FAtomComponentEvaluationData* AddRootAtomComponent(FInstanceHandle InstanceHandle, UMovieSceneAtomSection* Section, UWorld* World);

	/**
	 * Stop the audio on the Atom component associated with the given Atom section.
	 */
	void StopSound(FInstanceHandle InstanceHandle, FObjectKey ActorKey, FObjectKey SectionKey);

	/**
	 * Reset shared accumulation data required every evaluation frame
	 */
	void ResetSharedData();

private:

	using FInstanceObjectKey = TTuple<FInstanceHandle, FObjectKey>;

	/** Map of all created Atom components */
	using FAtomComponentBySectionKey = TMap<FInstanceObjectKey, FAtomComponentEvaluationData>;
	using FAtomComponentsByActorKey = TMap<FObjectKey, FAtomComponentBySectionKey>;
	FAtomComponentsByActorKey AtomComponentsByActorKey;

	/** Map of Atom input values, rebuilt every frame */
	using FAtomInputsBySectionKey = TMap<FInstanceObjectKey, FAtomComponentInputEvaluationData>;
	FAtomInputsBySectionKey AtomInputsBySectionKey;

	/** Pre-animated state */
	TSharedPtr<UE::MovieScene::FPreAnimatedAtomStorage> PreAnimatedStorage;

	friend struct UE::MovieScene::FGatherAtomInputs;
	friend struct UE::MovieScene::FGatherAtomTriggers;
	friend struct UE::MovieScene::FEvaluateAtom;
};
