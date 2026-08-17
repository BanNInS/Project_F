/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomWaveBankFactory.h
 *
 ****************************************************************************/

#pragma once

#include "AtomSoundBankFactory.h"

#include "AtomWaveBankFactory.generated.h"

UCLASS(HideCategories = Object)
class UAtomWaveBankFactory
	: public UAtomSoundBankFactory
{
	GENERATED_BODY()

public:

	UAtomWaveBankFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory interface
	virtual UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* FileType, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	//~ End UFactory interface

	//~ Begin FReimportHandler interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	//~ end FReimportHandler interface
};
