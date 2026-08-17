/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomBusEffectPresetFactory.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "UObject/ObjectMacros.h"

// Forward Definitions
class UAtomBusEffectPreset;

#include "AtomBusEffectPresetFactory.generated.h"

UCLASS(MinimalAPI, HideCategories = Object)
class UAtomBusEffectPresetFactory : public UFactory
{
	GENERATED_BODY()

public:

	UAtomBusEffectPresetFactory(const FObjectInitializer& ObjectInitializer);

	/** The type of sound source effect preset that will be created */
	UPROPERTY(EditAnywhere, Category = AtomBusEffectFactory)
	TSubclassOf<UAtomBusEffectPreset> AtomBusEffectPresetClass;

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool CanCreateNew() const override;
	//~ End UFactory Interface	
};
