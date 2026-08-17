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

#pragma once

#include "Factories/Factory.h"

#include "AtomAudioLinkSettingsFactory.generated.h"

UCLASS(HideCategories = Object, MinimalAPI)
class UAtomAudioLinkSettingsFactory
	: public UFactory
{
	GENERATED_BODY()

	UAtomAudioLinkSettingsFactory(const FObjectInitializer& ObjectInitializer);

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//virtual uint32 GetMenuCategories() const override;
};
