/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWareEditor.cpp
 *
 ****************************************************************************/

#include "CriWareEditor.h"

#if WITH_EDITORONLY_DATA
#include "CriWareCore.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomComponent.h"
#endif

UCriWareEditor* GCriWareEditor = nullptr;

#define LOCTEXT_NAMESPACE "CriWareEditor"

UCriWareEditor::UCriWareEditor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_EDITORONLY_DATA

void UCriWareEditor::Init(TWeakPtr<FCriWareCore> InCriWareCore)
{
	Super::Init(InCriWareCore);
}

UAtomComponent* UCriWareEditor::GetPreviewAtomComponent()
{
	return PreviewAtomComponent;
}

UAtomComponent* UCriWareEditor::ResetPreviewAtomComponent(UAtomSoundBase* Sound /* = nullptr */)
{
	if (FAtomRuntime* AtomRuntime = GetCriWareCore()->GetMainAtomRuntimeRaw())
	{
		if (PreviewAtomComponent)
		{
			PreviewAtomComponent->Stop();
		}
		else
		{
			TObjectPtr<UAtomCueSheet> Owner = NewObject<UAtomCueSheet>();
			PreviewAtomSoundCue = NewObject<UAtomSoundCue>((UObject*)Owner);
			PreviewAtomSoundCue->CueSheet = Owner;
			// Set world to nullptr as it will most likely become invalid in the next PIE/Simulate session and the
			// component will be left with invalid pointer.
			PreviewAtomComponent = FAtomRuntime::CreateComponent(PreviewAtomSoundCue);
		}

		check(PreviewAtomComponent);
		// Mark as a preview component so the distance calculations can be ignored
		PreviewAtomComponent->bPreviewComponent = true;

		if (Sound)
		{
			PreviewAtomComponent->Sound = Sound;
		}
	}

	return PreviewAtomComponent;
}

UAtomComponent* UCriWareEditor::PlayPreviewSound(UAtomSoundBase* Sound)
{
	if (UAtomComponent* AtomComponent = ResetPreviewAtomComponent(Sound))
	{
		AtomComponent->bAutoDestroy = false;
		AtomComponent->bIsUISound = true;
		AtomComponent->bAllowSpatialization = false;
		//AtomComponent->bReverb = false;
		//AtomComponent->bCenterChannelOnly = false;
		AtomComponent->bIsPreviewSound = true;
		AtomComponent->Play();

		return AtomComponent;
	}

	return nullptr;
}

void UCriWareEditor::ClearPreviewComponents()
{
	if (PreviewAtomComponent)
	{
		PreviewAtomComponent->Stop();

		// Just null out so they get GC'd
		PreviewAtomSoundCue = nullptr;
		PreviewAtomComponent->Sound = nullptr;
		PreviewAtomComponent = nullptr;
	}
}

void UCriWareEditor::FinishDestroy()
{
	if (GCriWareEditor)
	{
		GCriWareEditor = nullptr;
	}

	Super::FinishDestroy();
}

#endif // WITH_EDITORDATA_ONLY

#undef LOCTEXT_NAMESPACE
