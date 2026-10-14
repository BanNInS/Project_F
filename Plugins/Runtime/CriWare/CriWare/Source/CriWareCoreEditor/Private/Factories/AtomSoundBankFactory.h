/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBankFactory.h
 *
 ****************************************************************************/

#pragma once

#include "Factories/Factory.h"
#include "EditorReimportHandler.h"

#include "AtomSoundBankFactory.generated.h"

class UAtomSoundBank;

UCLASS(abstract)
class UAtomSoundBankFactory
	: public UFactory
	, public FReimportHandler
{
	GENERATED_BODY()

public:

	UAtomSoundBankFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory interface
	virtual UObject* FactoryCreateBinary(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, const TCHAR* FileType, const uint8*& Buffer, const uint8* BufferEnd, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	//~ End UFactory interface

	//~ Begin FReimportHandler interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	//~ end FReimportHandler interface

protected:

	bool CanReimportSoundBank(UAtomSoundBank* AtomSoundBank, TArray<FString>& OutFilenames);
	TObjectPtr<UAtomSoundBank> ReimportSoundBank;
};
