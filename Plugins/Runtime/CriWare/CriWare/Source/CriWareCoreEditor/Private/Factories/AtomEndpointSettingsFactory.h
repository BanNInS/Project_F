/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomEndpointSettingsFactory.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "UObject/ObjectMacros.h"

#include "AtomEndpointSettingsFactory.generated.h"

// Forward Definitions
class UAtomEndpointSettingsBase;

// For creating our custom Atom endpoint settings
UCLASS(MinimalAPI, HideCategories = Object)
class UAtomEndpointSettingsFactory
	: public UFactory
{
	GENERATED_BODY()

public:

	UAtomEndpointSettingsFactory(const FObjectInitializer& ObjectInitializer);

	/** The type of endpoint settings that will be created */
	UPROPERTY(EditAnywhere, Category = AtomEndpointSettingsFactory)
	TSubclassOf<UAtomEndpointSettingsBase> GeneratorClass;

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	//~ End UFactory Interface
};
