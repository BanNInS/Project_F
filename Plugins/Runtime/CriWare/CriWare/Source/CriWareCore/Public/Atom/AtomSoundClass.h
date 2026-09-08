/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomSoundClass.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"
#if WITH_EDITOR
#include "EdGraph/EdGraph.h"
#endif // WITH_EDITOR

#include "Atom/Atom.h"
#include "Atom/Modulation/AtomModulationDestination.h"

#include "AtomSoundClass.generated.h"

USTRUCT(BlueprintType)
struct CRIWARECORE_API FAtomCategory
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Category")
	FName Name;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Category")
	int32 ID = INDEX_NONE;
};

/**
 * Structure containing configurable properties of a Sound Class.
 */
USTRUCT(BlueprintType)
struct FAtomSoundClassProperties
{
	GENERATED_BODY()

public:

	/** Volume multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General)
	float Volume;

	/** Pitch multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General)
	float Pitch;

	/** Lowpass filter cutoff frequency */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General)
	float LowPassFilterFrequency;

	/** Hipass filter cutoff frequency */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General)
	float HighPassFilterFrequency;

	/** The amount of a sound to bleed to the LFE channel */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Routing, meta = (DisplayName = "LFE Bleed"))
	//float LFEBleed;

	/** The amount to send to center channel (does not propagate to child classes) */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Routing)
	//float VoiceCenterChannelVolume;

	/** Distance scale to apply to sounds that play with this sound class.
	 * Sounds will have their attenuation distance scaled by this amount.
	 * Allows adjusting attenuation settings dynamically. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General)
	float AttenuationDistanceScale;

	/** Whether to inflate referencing sound's priority to always play. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General)
	uint8 bAlwaysPlay : 1;

	/** Whether or not this sound plays when the game is paused in the UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = General)
	uint8 bIsUISound : 1;

	/** Whether or not this sound class uses an Atom Craft voice limit group to control the number of voices independently. 
	 * When set, the number of voices played by sounds using this sound class is limited by a voice limit group from Atom Craft.
	 * Otherwise voice limit cis applyed over the entire game. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = General, meta = (DisplayName = "Use Atom Craft Voice Limit Scope"))
	uint8 bEnableVoiceLimitScope : 1;

	/** Whether or not this sound class uses Atom Craft category cue limit to control the number of cue playbacks independently.
	 * When set, the number of cues played by sounds using sound class is limited by category cue limit from Atom Craft.
	 * Otherwise, category cue limit is applyed over the entire game. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, Category = General, meta = (DisplayName = "Use Atom Craft Category Cue Limit Scope"))
	uint8 bEnableCategoryCueLimitScope : 1;

	/** Whether or not this sound class forces sounds to the center channel */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Routing)
	//uint8 bCenterChannelOnly : 1;

	/** Whether the Interior/Exterior volume and LPF modifiers should be applied */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Routing)
	uint8 bApplyAmbientVolumes : 1;

	/** Whether or not sounds referencing this class send to the reverb submix */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Submix, meta = (DisplayName = "Send to Master Reverb Submix"))
	//uint8 bReverb : 1;

	/** Send amount to master reverb effect for referencing unattenuated (2D) sounds. */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Submix)
	//float Default2DReverbSendAmount;

	/** Default modulation settings for sounds directly referencing this class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Modulation)
	FAtomSoundModulationDefaultSettings ModulationSettings;

	/** Specifies how and when compressed audio data is loaded for asset. */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Loading, meta = (DisplayName = "Loading Behavior Override"))
	//EAtomSoundWaveLoadingBehavior LoadingBehavior;

	/** Default output rack of referencing sounds. If unset, falls back to the 'Master Rack' as set in the 'Atom' category of Project Settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Submix)
	TObjectPtr<UAtomRack> DefaultRack;

	FAtomSoundClassProperties();
};

/** Class for sound class properties which are intended to be dynamically changing. */
struct FAtomSoundClassDynamicProperties
{
	FAtomDynamicParameter AttenuationScaleParam;

	FAtomSoundClassDynamicProperties()
		: AttenuationScaleParam(1.0f)
	{
	}
};

#if WITH_EDITOR
class UAtomSoundClass;

/** Interface for sound class graph interaction with the CriWareCoreEditor module. */
class IAtomSoundClassEditor
{
public:

	virtual ~IAtomSoundClassEditor() {}

	/** Refreshes the sound class graph links. */
	virtual void RefreshGraphLinks(UEdGraph* SoundClassGraph) = 0;
};
#endif

UCLASS(config = CriWare, HideCategories = Object, EditInLineNew, BlueprintType)
class CRIWARECORE_API UAtomSoundClass
	: public UObject
{
	GENERATED_BODY()

public:

	UAtomSoundClass(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Category", meta = (InlineEditConditionToggle))
	uint8 bOverrideCategory : 1;

	/** An Atom Category to link to. */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Category", meta = (EditCondition = "!bOverrideCategory"))
	//FAtomCategory CategoryToOverride;

	/** Configurable properties like volume and priority. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General, meta = (ShowOnlyInnerProperties))
	FAtomSoundClassProperties Properties;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General)
	TArray<TObjectPtr<UAtomSoundClass>> ChildClasses;

	UPROPERTY(BlueprintReadOnly, Category = General)
	TObjectPtr<UAtomSoundClass> ParentClass;

	//UFUNCTION(BlueprintCallable, Category = "Class")
	//TArray<UAtomSoundBase*> GetAllSounds();

	/** */
	UFUNCTION(BlueprintCallable, Category = "Class")
	void StopAllSounds();

	/** */
	UFUNCTION(BlueprintCallable, Category = "Class")
	void PauseAllSounds(bool bPause);

#if WITH_EDITORONLY_DATA
	/** EdGraph based representation of the SoundClass */
    TObjectPtr<UEdGraph> SoundClassGraph;
#endif // WITH_EDITORONLY_DATA

protected:

	//~ Begin UObject Interface.
	virtual FString GetDesc(void) override;
	virtual void BeginDestroy() override;
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface.

public:

	// Sound Class Editor functionality
#if WITH_EDITOR
	/**
	 * @return true if the child sound class exists in the tree.
	 */
	bool RecurseCheckChild(UAtomSoundClass* InChildSoundClass);

	/**
	 * Set the parent class of this AtomSoundClass, removing it as a child from its previous owner
	 *
	 * @param	InParentClass	The new parent class of this.
	 */
	void SetParentClass(UAtomSoundClass* InParentClass);

	/**
	 * Add Referenced objects.
	 *
	 * @param	InThis AtomSoundClass we are adding references from.
	 * @param	Collector The reference collector.
	 */
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

	/**
	 * Refresh all EdGraph representations of AtomSoundClasses
	 *
	 * @param	bIgnoreThis	Whether to ignore this AtomSoundClass if it's already up to date.
	 */
	void RefreshAllGraphs(bool bIgnoreThis);

	/** Sets the sound cue graph editor implementation. */
	static void SetSoundClassEditor(TSharedPtr<IAtomSoundClassEditor> InSoundClassEditor);

	/** Gets the sound cue graph editor implementation. */
	static TSharedPtr<IAtomSoundClassEditor> GetSoundClassEditor();

private:

	/** Ptr to interface to sound class editor operations. */
	static TSharedPtr<IAtomSoundClassEditor> SoundClassEditor;

#endif
};
