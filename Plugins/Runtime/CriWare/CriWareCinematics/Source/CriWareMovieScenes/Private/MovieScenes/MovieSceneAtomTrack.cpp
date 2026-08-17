/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : CriWareMovieScene
 * File     : MovieSceneAtomTrack.cpp
 *
 ****************************************************************************/

#include "MovieSceneAtomTrack.h"

#include "MovieScene.h"
#include "Evaluation/MovieSceneSegment.h"
#include "Compilation/MovieSceneSegmentCompiler.h"
#include "Compilation/MovieSceneCompilerRules.h"
#include "MovieSceneCommonHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "Atom/AtomSoundBase.h"

#include "MovieSceneAtomSection.h"
#include "MovieSceneAtomSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovieSceneAtomTrack)

#define LOCTEXT_NAMESPACE "MovieSceneAtomTrack"

UMovieSceneAtomTrack::UMovieSceneAtomTrack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);
#if WITH_EDITORONLY_DATA
	TrackTint = FColor(93, 95, 136);
	RowHeight = 50;
#endif
}

const TArray<UMovieSceneSection*>& UMovieSceneAtomTrack::GetAllSections() const
{
	return AtomSections;
}

bool UMovieSceneAtomTrack::SupportsMultipleRows() const
{
	return true;
}

bool UMovieSceneAtomTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UMovieSceneAtomSection::StaticClass();
}

void UMovieSceneAtomTrack::RemoveAllAnimationData()
{
	AtomSections.Empty();
}

bool UMovieSceneAtomTrack::HasSection(const UMovieSceneSection& Section) const
{
	return AtomSections.Contains(&Section);
}

void UMovieSceneAtomTrack::AddSection(UMovieSceneSection& Section)
{
	AtomSections.Add(&Section);
}

void UMovieSceneAtomTrack::RemoveSection(UMovieSceneSection& Section)
{
	AtomSections.Remove(&Section);
}

void UMovieSceneAtomTrack::RemoveSectionAt(int32 SectionIndex)
{
	AtomSections.RemoveAt(SectionIndex);
}

bool UMovieSceneAtomTrack::IsEmpty() const
{
	return AtomSections.Num() == 0;
}

UMovieSceneSection* UMovieSceneAtomTrack::AddNewSoundOnRow(UAtomSoundBase* Sound, FFrameNumber Time, int32 RowIndex)
{
	check(Sound);

	// get frame rate
	FFrameRate FrameRate = GetTypedOuter<UMovieScene>()->GetTickResolution();

	// determine initial duration
	// @todo Once we have infinite sections, we can remove this
	// @todo ^^ Why? Infinte sections would mean there's no starting time?
	//FFrameTime DurationToUse = 1.f * FrameRate; // if all else fails, use 1 second duration

	// get length of audio data in seconds
	float SoundDuration = Sound->GetDuration(); // seconds
	if (SoundDuration == ATOM_INDEFINITELY_LOOPING_DURATION)
	{
		SoundDuration = Sound->WaveInfo.GetDuration();
	}

	// Convert to frame units
	FFrameTime DurationToUse = SoundDuration * FrameRate;

	// add section
	UMovieSceneAtomSection* NewSection = NewObject<UMovieSceneAtomSection>(this);
	NewSection->InitialPlacementOnRow(AtomSections, Time, DurationToUse.FrameNumber.Value, RowIndex);
	NewSection->SetSound(Sound);

#if WITH_EDITORONLY_DATA
	// Use the timecode info from the sound wave if it's available to populate
	// the section's TimecodeSource property. Otherwise try the base sound's
	// timecode offset.
//	TOptional<FSoundWaveTimecodeInfo> TimecodeInfo;
//	if (const USoundWave* SoundWave = Cast<USoundWave>(Sound))
//	{
//		TimecodeInfo = SoundWave->GetTimecodeInfo();
//	}
//
//	if (TimecodeInfo.IsSet())
//	{
//		const double NumSecondsSinceMidnight = TimecodeInfo->GetNumSecondsSinceMidnight();
//		const FTimecode Timecode(NumSecondsSinceMidnight, TimecodeInfo->TimecodeRate, TimecodeInfo->bTimecodeIsDropFrame, /* InbRollover = */ true);
//
//		NewSection->TimecodeSource = FMovieSceneTimecodeSource(Timecode);
//	}
//	else
//	{
//		const TOptional<FSoundTimecodeOffset> TimecodeOffset = Sound->GetTimecodeOffset();
//		if (TimecodeOffset.IsSet())
//		{
//			const double NumSecondsSinceMidnight = TimecodeOffset->NumOfSecondsSinceMidnight;
//
//			// The timecode offset does not carry a rate with it, so just use the
//			// display rate for this movie scene.
//			const FFrameRate DisplayRate = GetTypedOuter<UMovieScene>()->GetDisplayRate();
//			const FTimecode Timecode(NumSecondsSinceMidnight, DisplayRate, /* InbRollover = */ true);
//
//			NewSection->TimecodeSource = FMovieSceneTimecodeSource(Timecode);
//		}
//	}
#endif

	AtomSections.Add(NewSection);

	return NewSection;
}

UMovieSceneSection* UMovieSceneAtomTrack::CreateNewSection()
{
	return NewObject<UMovieSceneAtomSection>(this, NAME_None, RF_Transactional);
}

// OLD ?
//FMovieSceneTrackRowSegmentBlenderPtr UMovieSceneAtomTrack::GetRowSegmentBlender() const
//{
//	struct FBlender : FMovieSceneTrackRowSegmentBlender
//	{
//		virtual void Blend(FSegmentBlendData& BlendData) const override
//		{
//			// Run the default high pass filter for overlap priority
//			MovieSceneSegmentCompiler::FilterOutUnderlappingSections(BlendData);
//
//			// Weed out based on array index (legacy behaviour)
//			MovieSceneSegmentCompiler::BlendSegmentLegacySectionOrder(BlendData);
//		}
//	};
//	return FBlender();
//}

void UMovieSceneAtomTrack::PostRename(UObject* OldOuter, const FName OldName)
{
	Super::PostRename(OldOuter, OldName);

	// Recache the channel proxy because attach in FAtomChannelEditorData is dependent upon the outer chain
	for (TObjectPtr<UMovieSceneSection>& Section : AtomSections)
	{
		if (UMovieSceneAtomSection* AtomSection = Cast<UMovieSceneAtomSection>(Section.Get()))
		{
			AtomSection->CacheChannelProxy();
		}
	}
}

#undef LOCTEXT_NAMESPACE
