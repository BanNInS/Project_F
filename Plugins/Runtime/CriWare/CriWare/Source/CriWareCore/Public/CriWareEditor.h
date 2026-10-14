/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWareEditor.h
 *
 ****************************************************************************/

#pragma once

#include "CriWare.h"

#include "CriWareEditor.generated.h"

#if WITH_EDITORONLY_DATA
// Forward Declarations
class UAtomComponent;
class UAtomSoundBase;
class UAtomSoundCue;
class FCriWareCore;
#endif

/**
 * CriWareCore that drives the Editor.
 * Separate from UCriWare because it may have much different functionality than desired for an instance of a game itself.
 */
UCLASS(config = CriWare, Transient)
class CRIWARECORE_API UCriWareEditor 
	: public UCriWare
{
	GENERATED_BODY()

public:

	UCriWareEditor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

#if WITH_EDITORONLY_DATA

	//~ UCriWare Interface
	virtual void Init(TWeakPtr<FCriWareCore> InCriWareCore) override;

	// Atom
	UPROPERTY(Transient)
	TObjectPtr<UAtomSoundCue> PreviewAtomSoundCue;

	UPROPERTY(Transient)
	TObjectPtr<UAtomComponent> PreviewAtomComponent;

	/**
	 * Returns the preview Atom component.
	 */
	UAtomComponent* GetPreviewAtomComponent();

	/**
	 * Returns an audio component linked to the current scene that it is safe to play a sound on.
	 *
	 * @param Sound A sound to attach to the Atom component.
	 */
	UAtomComponent* ResetPreviewAtomComponent(UAtomSoundBase* Sound = nullptr);

	/**
	 * Plays a preview of a specified sound or node.
	 *
	 * @param Sound	A sound to attach to the Atom component.
	 */
	UAtomComponent* PlayPreviewSound(UAtomSoundBase* Sound);

	/**
	 * Clean up any world specific editor components so they can be GC correctly.
	 */
	void ClearPreviewComponents();

	//~ Begin UObject Interface.
	virtual void FinishDestroy() override;
	//~ End UObject Interface.

#endif // WITH_EDITORONLY_DATA
};

/** The editor object. */
extern CRIWARECORE_API class UCriWareEditor* GCriWareEditor;
