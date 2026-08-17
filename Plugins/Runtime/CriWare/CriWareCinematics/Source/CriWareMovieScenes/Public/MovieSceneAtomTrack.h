/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : CriWareMovieScene
 * File     : MovieSceneAtomTrack.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MovieSceneNameableTrack.h"
#include "Compilation/IMovieSceneTrackTemplateProducer.h"
#include "Evaluation/MovieSceneEvalTemplate.h"

#include "MovieSceneAtomTrack.generated.h"

// Forward Definitions
class UAtomSoundBase;

namespace AtomTrackConstants
{
	/* Preview time when moving the cursor in the sequencer editor
	 * Note: Considering the buffering time of stream playback,
     * Set the preview time longer than the UE standard sound. */
	const float ScrubDuration = 0.050f;
}

/**
 * Handles manipulation of Atom audio.
 */
UCLASS(MinimalAPI)
class UMovieSceneAtomTrack
	: public UMovieSceneNameableTrack
{
	GENERATED_BODY()

public:

	UMovieSceneAtomTrack(const FObjectInitializer& ObjectInitializer);

	/** Adds a new sound cue to the audio */
	CRIWAREMOVIESCENES_API virtual UMovieSceneSection* AddNewSoundOnRow(UAtomSoundBase* Sound, FFrameNumber Time, int32 RowIndex);

	/** Adds a new sound cue on the next available/non-overlapping row */
	virtual UMovieSceneSection* AddNewSound(UAtomSoundBase* Sound, FFrameNumber Time) { return AddNewSoundOnRow(Sound, Time, INDEX_NONE); }

	/** @return The audio sections on this track */
	const TArray<UMovieSceneSection*>& GetAtomSections() const
	{
		return AtomSections;
	}

public:

	//~ UMovieSceneTrack Interface
	CRIWAREMOVIESCENES_API virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
	CRIWAREMOVIESCENES_API virtual void RemoveAllAnimationData() override;
	CRIWAREMOVIESCENES_API virtual bool HasSection(const UMovieSceneSection& Section) const override;
	CRIWAREMOVIESCENES_API virtual void AddSection(UMovieSceneSection& Section) override;
	CRIWAREMOVIESCENES_API virtual void RemoveSection(UMovieSceneSection& Section) override;
	CRIWAREMOVIESCENES_API virtual void RemoveSectionAt(int32 SectionIndex) override;
	CRIWAREMOVIESCENES_API virtual bool IsEmpty() const override;
	CRIWAREMOVIESCENES_API virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;
	CRIWAREMOVIESCENES_API virtual bool SupportsMultipleRows() const override;
	CRIWAREMOVIESCENES_API virtual UMovieSceneSection* CreateNewSection() override;
	// OLD ?
	//CRIWAREMOVIESCENES_API virtual FMovieSceneTrackRowSegmentBlenderPtr GetRowSegmentBlender() const override;

	// ~UObject interface
	CRIWAREMOVIESCENES_API virtual void PostRename(UObject* OldOuter, const FName OldName) override;

private:

	/** List of all root Atom sections */
	UPROPERTY()
	TArray<TObjectPtr<UMovieSceneSection>> AtomSections;

#if WITH_EDITORONLY_DATA

public:

	/**
	 * Get the height of this track's rows
	 */
	int32 GetRowHeight() const
	{
		return RowHeight;
	}

	/**
	 * Set the height of this track's rows
	 */
	void SetRowHeight(int32 NewRowHeight)
	{
		RowHeight = FMath::Max(16, NewRowHeight);
	}

private:

	/** The height for each row of this track */
	UPROPERTY()
	int32 RowHeight;

#endif
};
