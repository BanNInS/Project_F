/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareAudioLinkEditor
 * File     : AtomAudioLinkSettingsFactory.h
 *
 ****************************************************************************/

#include "AtomAudioLinkSettingsFactory.h"

#include "AtomAudioLinkSettings.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

/*
 * Atom AudioLink Settings Factory
 *****************************************************************************/

UAtomAudioLinkSettingsFactory::UAtomAudioLinkSettingsFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UAtomAudioLinkSettings::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UAtomAudioLinkSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UAtomAudioLinkSettings* AudioLinkSettings = NewObject<UAtomAudioLinkSettings>(InParent, InName, Flags);

	return AudioLinkSettings;
}

/*uint32 UAtomAudioLinkSettingsFactory::GetMenuCategories() const
{
   return EAssetTypeCategories::Sounds;
}*/

#undef LOCTEXT_NAMESPACE
