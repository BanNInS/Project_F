/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareAtomMixer2Editor
 * File     : AtomEndpointFactory.h
 *
 ****************************************************************************/

#pragma once

#include "Factories/Factory.h"

#include "AtomEndpointFactory.generated.h"

UCLASS(HideCategories = Object)
class UAtomEndpointFactory
	: public UFactory
{
	GENERATED_BODY()

public:

	UAtomEndpointFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	//~ End UFactory Interface	
};

UCLASS(hidecategories = Object)
class UAtomSoundfieldEndpointFactory
	: public UFactory
{
	GENERATED_BODY()

	UAtomSoundfieldEndpointFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	//~ Begin UFactory Interface	
};
