/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomBusEffect32BandEQ.cpp
 *
 ****************************************************************************/

#include "Atom/Effects/AtomBusEffect32BandEQ.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomBusEffect32BandEQ"

/*
 * UAtomBusEffect32BandEQPreset Implementation
 *****************************************************************************/

UAtomBusEffect32BandEQPreset::UAtomBusEffect32BandEQPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PluginName = TEXT("CRIWARE");
	EffectName = TEXT(CRIATOMEXASR_32BANDS_EQUALIZER_NAME);
	NumParameters = CRIATOMEXASR_MULTIBANDS_EQ_NUM_PARAMETERS(32);

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

#undef LOCTEXT_NAMESPACE
