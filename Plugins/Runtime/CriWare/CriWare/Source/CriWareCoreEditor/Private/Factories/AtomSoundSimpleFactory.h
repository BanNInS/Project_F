/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundSimpleFactory.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"

#include "AtomSoundSimpleFactory.generated.h"

// Forward Declarations
class UAtomSoundWave;

UCLASS(HideCategories = Object, MinimalAPI)
class UAtomSoundSimpleFactory
	: public UFactory
{
	GENERATED_BODY()

public:

	UAtomSoundSimpleFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ End UFactory interface

	/** Sound waves to create the simple sound with */
	UPROPERTY()
	TArray<TObjectPtr<UAtomSoundWave>> SoundWaves;
};
