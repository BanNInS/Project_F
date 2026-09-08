/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : CriWareSequencer
 * File     : AtomTrackEditor.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SWidget.h"
#include "ISequencerSection.h"
#include "MovieSceneTrack.h"
#include "ISequencer.h"
#include "ISequencerTrackEditor.h"
#include "MovieSceneTrackEditor.h"
#include "IContentBrowserSingleton.h"

#include "AtomTrackEditor.generated.h"

struct FAssetData;
class FAtomThumbnail;
class FDelegateHandle;
class FMenuBuilder;
class FSequencerSectionPainter;
class UAtomSoundBase;
class UMovieSceneAtomTrack;
struct FAtomAisacControl;

/** class to hold a Sound property to permit a Sound selection. (Internal Use only) */
UCLASS(Transient)
class UAtomAddTrackSoundHolder
	: public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(Transient, EditAnywhere, Category = Sound, meta = (AllowPrivateAccess))
	TObjectPtr<class UAtomSoundBase> Sound;
};

/**
 * Tools for atom tracks
 */
class CRIWARESEQUENCER_API FAtomTrackEditor
	: public FMovieSceneTrackEditor
{
public:

	/**
	 * Constructor
	 *
	 * @param InSequencer The sequencer instance to be used by this tool
	 */
	FAtomTrackEditor(TSharedRef<ISequencer> InSequencer);

	/** Virtual destructor. */
	virtual ~FAtomTrackEditor();

	/**
	 * Creates an instance of this class.  Called by a sequencer
	 *
	 * @param OwningSequencer The sequencer instance to be used by this tool
	 * @return The new instance of this class
	 */
	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> OwningSequencer);

public:
	// ISequencerTrackEditor interface

	virtual void OnInitialize() override;
	virtual void OnRelease() override;
	virtual void BuildAddTrackMenu(FMenuBuilder& MenuBuilder) override;
	virtual void BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass) override;
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params) override;
	virtual bool HandleAssetAdded(UObject* Asset, const FGuid& TargetObjectGuid) override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;
	virtual bool SupportsSequence(UMovieSceneSequence* InSequence) const override;
	virtual const FSlateBrush* GetIconBrush() const override;
	virtual bool IsResizable(UMovieSceneTrack* InTrack) const override;
	virtual void Resize(float NewSize, UMovieSceneTrack* InTrack) override;
	virtual bool OnAllowDrop(const FDragDropEvent& DragDropEvent, FSequencerDragDropParams& DragDropParams) override;
	virtual FReply OnDrop(const FDragDropEvent& DragDropEvent, const FSequencerDragDropParams& DragDropParams) override;

protected:

	/** Delegate for AnimatablePropertyChanged in HandleAssetAdded for sounds */
	FKeyPropertyResult AddNewSound(FFrameNumber KeyTime, class UAtomSoundBase* Sound, UMovieSceneAtomTrack* Track, int32 RowIndex);

	/** Delegate for AnimatablePropertyChanged in HandleAssetAdded for attached sounds */
	FKeyPropertyResult AddNewAttachedSound(FFrameNumber KeyTime, class UAtomSoundBase* Sound, UMovieSceneAtomTrack* Track, TArray<TWeakObjectPtr<UObject>> ObjectsToAttachTo);

private:
	
	/** Callback for executing the "Add Atom Track" menu entry. */
	void HandleAddAtomTrackMenuEntryExecute();

	/** Callback for executing the "Add Atom Track" menu entry on an actor */
	void HandleAddAttachedAtomTrackMenuEntryExecute(FMenuBuilder& MenuBuilder, TArray<FGuid> ObjectBindings);

	/** Callback for executing the "AISAC Parameters" menu entries on a track */
	void HandleAddAisacControlActionExecute(UMovieSceneAtomTrack* Track, int32 RowIndex, FAtomAisacControl AisacControl);

	/** Atom sub menu */
	TSharedRef<SWidget> BuildAtomSubMenu(FOnAssetSelected OnAssetSelected, FOnAssetEnterPressed OnAssetEnterPressed, UMovieSceneTrack* Track, int32 InRowIndex);

	/** Atom AISAC menu section */
	void BuildAddAisacTrackMenu(FMenuBuilder& MenuBuilder, UMovieSceneTrack* Track, int32 InRowIndex);
	
	/** Atom asset selected */
	void OnAtomAssetSelected(const FAssetData& AssetData, UMovieSceneTrack* Track);

	/** Atom asset enter pressed */
	void OnAtomAssetEnterPressed(const TArray<FAssetData>& AssetData, UMovieSceneTrack* Track);

	/** Attached atom asset selected */
	void OnAttachedAtomAssetSelected(const FAssetData& AssetData, TArray<FGuid> ObjectBindings);

	/** Attached atom asset enter pressed */
	void OnAttachedAtomEnterPressed(const TArray<FAssetData>& AssetData, TArray<FGuid> ObjectBindings);

	/** Registers a delegate with the sequencer for monitoring edits */
	void RegisterMovieSceneChangedDelegate();

	/** Movie Scene Data Changed Delegate */
	void OnMovieSceneDataChanged(EMovieSceneDataChangeType InChangeType);

	/** Returns true if the given Sequence or any subsequence contains an audio track */
	bool SequenceContainsAtomTrack(const UMovieSceneSequence* InSequence);

	/** Will return true if a sequence contains an audio track and the user was notified about the potential clock source issue */
	bool CheckSequenceClockSource();

	/** Prompts user and potentially modifies settings pref USequencerSettings::bAutoSelectAudioClockSource */
	void PromptUserForClockSource();

	/** Sets the clock source for the given sequence to use the audio clock */
	void SetClockSourceToAudioClock();

private:

	TObjectPtr<UAtomAddTrackSoundHolder> SoundPropertyHolderObject;

	TSharedPtr<IDetailsView> SoundDetailView;

	FDelegateHandle MovieSceneChangedDelegate;
};

/**
 * Class for atom sections, handles drawing of all waveform previews.
 */
class CRIWARESEQUENCER_API FAtomSection
	: public ISequencerSection
	, public TSharedFromThis<FAtomSection>
{
public:

	/** Constructor. */
	FAtomSection(UMovieSceneSection& InSection, TWeakPtr<ISequencer> InSequencer);

	/** Virtual destructor. */
	virtual ~FAtomSection();

public:
	
	// ISequencerSection interface

	virtual UMovieSceneSection* GetSectionObject() override;
	virtual FText GetSectionTitle() const override;
	virtual FText GetSectionToolTip() const override;
	virtual float GetSectionHeight() const override;
	virtual int32 OnPaintSection(FSequencerSectionPainter& Painter) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const FGeometry& ParentGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual void BeginResizeSection() override;
	virtual void ResizeSection(ESequencerSectionResizeMode ResizeMode, FFrameNumber ResizeTime) override;
	virtual void BeginSlipSection() override;
	virtual void SlipSection(FFrameNumber SlipTime) override;
	virtual bool RequestDeleteKeyArea(const TArray<FName>& KeyAreaNamePaths) override;

private:
	
	/* Re-creates the texture used to preview the waveform. */
	void RegenerateWaveforms(TRange<float> DrawRange, int32 XOffset, int32 XSize, const FColor& ColorTint, float DisplayScale);

private:
	
	/** The section we are visualizing. */
	UMovieSceneSection& Section;

	/** The waveform thumbnail render object. */
	TSharedPtr<class FAtomThumbnail> WaveformThumbnail;

	/** Stored data about the waveform to determine when it is invalidated. */
	TRange<float> StoredDrawRange;
	FFrameNumber StoredStartOffset;
	int32 StoredXOffset;
	int32 StoredXSize;
	FColor StoredColor;
	float StoredSectionHeight;
	bool bStoredLooping;

	/** Stored sound atom cue to determine when it is invalidated. */
	TWeakObjectPtr<UAtomSoundBase> StoredSoundBase;

	TWeakPtr<ISequencer> Sequencer;

	/** Cached start offset value valid only during resize */
	FFrameNumber InitialStartOffsetDuringResize;

	/** Cached start time valid only during resize */
	FFrameNumber InitialStartTimeDuringResize;
};
