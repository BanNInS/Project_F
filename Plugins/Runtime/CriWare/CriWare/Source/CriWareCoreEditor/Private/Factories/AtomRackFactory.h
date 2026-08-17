/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomRackFactory.h
 *
 ****************************************************************************/

#pragma once

#include "Factories/Factory.h"

#include "AtomRackFactory.generated.h"

UCLASS(HideCategories = Object)
class UAtomRackFactory
	: public UFactory
{
	GENERATED_BODY()

public:

	UAtomRackFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	//~ End UFactory Interface	
};

UCLASS(hidecategories = Object)
class UAtomSoundfieldRackFactory
	: public UFactory
{
	GENERATED_BODY()

	UAtomSoundfieldRackFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	//~ Begin UFactory Interface
};

UCLASS(HideCategories = Object)
class UAtomEndpointRackFactory
	: public UFactory
{
	GENERATED_BODY()

public:

	UAtomEndpointRackFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	//~ End UFactory Interface	
};

UCLASS(hidecategories = Object)
class UAtomSoundfieldEndpointRackFactory
	: public UFactory
{
	GENERATED_BODY()

	UAtomSoundfieldEndpointRackFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	//~ Begin UFactory Interface	
};
