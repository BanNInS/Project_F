/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomModulationControlBusFactory.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "UObject/ObjectMacros.h"

#include "AtomModulationControlBusFactory.generated.h"

UCLASS(MinimalAPI, HideCategories = Object)
class UAtomModulationControlBusFactory
	: public UFactory
{
	GENERATED_BODY()

public:

	UAtomModulationControlBusFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	//~ End UFactory Interface
};

UCLASS(MinimalAPI, HideCategories = Object)
class UAtomModulationControlBusMixFactory
    : public UFactory
{
    GENERATED_BODY()

public:

    UAtomModulationControlBusMixFactory(const FObjectInitializer& ObjectInitializer);

    //~ Begin UFactory Interface
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual bool CanCreateNew() const override;
    //~ End UFactory Interface
};
