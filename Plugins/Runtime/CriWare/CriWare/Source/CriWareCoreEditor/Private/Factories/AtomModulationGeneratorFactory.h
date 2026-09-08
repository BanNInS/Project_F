/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomModulationGeneratorFactory.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "UObject/ObjectMacros.h"

#include "AtomModulationGeneratorFactory.generated.h"

 // Forward Definitions
class UAtomModulationGenerator;

UCLASS(MinimalAPI, HideCategories = Object)
class UAtomModulationGeneratorFactory
    : public UFactory
{
    GENERATED_BODY()

public:

	UAtomModulationGeneratorFactory(const FObjectInitializer& ObjectInitializer);

	/** The type of sound source effect preset that will be created */
	UPROPERTY(EditAnywhere, Category = AtomModulationGeneratorFactory)
	TSubclassOf<UAtomModulationGenerator> GeneratorClass;

    //~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
    virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual bool CanCreateNew() const override;
    //~ End UFactory Interface
};
