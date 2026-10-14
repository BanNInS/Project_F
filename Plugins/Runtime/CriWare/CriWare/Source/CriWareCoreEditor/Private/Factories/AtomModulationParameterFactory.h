/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomModulationParameterFactory.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "UObject/ObjectMacros.h"

#include "AtomModulationParameterFactory.generated.h"

 // Forward Definitions
class UAtomModulationParameter;

UCLASS(MinimalAPI, HideCategories = Object)
class UAtomModulationParameterFactory
	: public UFactory
{
	GENERATED_BODY()

public:

	UAtomModulationParameterFactory(const FObjectInitializer& ObjectInitializer);

	/** The type of sound source effect preset that will be created */
	UPROPERTY(EditAnywhere, Category = ModulationParameterFactory)
		TSubclassOf<UAtomModulationParameter> ParameterClass;

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	//~ End UFactory Interface
};
