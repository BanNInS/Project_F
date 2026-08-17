/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundClassFactory.h
 *
 ****************************************************************************/

#pragma once

#include "Factories/Factory.h"

#include "AtomSoundClassFactory.generated.h"

UCLASS()
class UAtomSoundClassFactory
    : public UFactory
{
    GENERATED_BODY()

public:

    UAtomSoundClassFactory(const FObjectInitializer& ObjectInitializer);

    //~ Begin UFactory Interface
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual bool CanCreateNew() const override;
    //~ End UFactory Interface
};
