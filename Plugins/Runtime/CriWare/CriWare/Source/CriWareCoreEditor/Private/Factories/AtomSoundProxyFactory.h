/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundProxyFactory.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"

#include "AtomSoundProxyFactory.generated.h"

// Forward Declarations
class UAtomSoundBase;

UCLASS(HideCategories = Object, MinimalAPI)
class UAtomSoundProxyFactory
	: public UFactory
{
	GENERATED_BODY()

public:

	UAtomSoundProxyFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ End UFactory interface

	/** Sound to create the Proxy sound with */
	UPROPERTY(BlueprintReadWrite, Category=SoundProxy)
	TObjectPtr<UAtomSoundBase> Sound;
};

UCLASS()
class UMySoundFactory
	: public UFactory
{
	GENERATED_BODY()

public:

	UMySoundFactory(const FObjectInitializer& ObjectInitializer);

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
