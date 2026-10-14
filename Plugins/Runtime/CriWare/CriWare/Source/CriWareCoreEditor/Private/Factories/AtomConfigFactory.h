/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomConfigFactory.h
 *
 ****************************************************************************/

#pragma once

#include "Factories/Factory.h"
#include "EditorReimportHandler.h"

#include "AtomConfigFactory.generated.h"

// Forward Declarations
class UAtomConfig;

UCLASS(HideCategories = Object)
class UAtomConfigFactory 
	: public UFactory
	, public FReimportHandler
{
	GENERATED_BODY()

public:

	UAtomConfigFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	//~ End UFactory Interface

	//~ Begin FReimportHandler interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	//~ End FReimportHandler interface

protected:

	TObjectPtr<UAtomConfig> ReimportConfig;
};
