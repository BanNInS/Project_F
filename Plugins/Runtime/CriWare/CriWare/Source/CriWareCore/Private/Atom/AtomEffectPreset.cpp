/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomEffectPreset.cpp
 *
 ****************************************************************************/

#include "Atom/AtomEffectPreset.h"

#include "CriWareCorePrivate.h"

#define LOCTEXT_NAMESPACE "AtomEffectPreset"

UAtomEffectPreset::UAtomEffectPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

#undef LOCTEXT_NAMESPACE
