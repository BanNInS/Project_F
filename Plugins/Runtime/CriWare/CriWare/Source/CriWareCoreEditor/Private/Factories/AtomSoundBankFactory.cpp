/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBankFactory.cpp
 *
 ****************************************************************************/

#include "AtomSoundBankFactory.h"

#include "EditorFramework/AssetImportData.h"

#include "Atom/AtomSoundBank.h"

#include "CriWareCoreEditorPrivate.h"

#include "Atom/AtomRuntimeManager.h"
#include "CriWare.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

UAtomSoundBankFactory::UAtomSoundBankFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ReimportSoundBank(nullptr)
{
	bText = false;
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UAtomSoundBankFactory::FactoryCreateBinary(
	UClass*				InClass,
	UObject*			InParent,
	FName				InName,
	EObjectFlags		Flags,
	UObject*			Context,
	const TCHAR*		FileType,
	const uint8*&		Buffer,
	const uint8*		BufferEnd,
	FFeedbackContext*	Warn,
	bool&				bOutOperationCanceled
	)
{
	if (!GCriWare || !GCriWare->GetAtomRuntimeManager())
	{
		return nullptr;
	}

	UAtomSoundBank* SoundBank = nullptr;

	if (ReimportSoundBank)
	{
		// Stop all sounds and release data
		ReimportSoundBank->ReleaseResource();

		// remove from the resource manager
		GCriWare->GetAtomRuntimeManager()->FreeResource(ReimportSoundBank);

		// reuse original object to reimport 
		SoundBank = ReimportSoundBank;

		// clear reimport pointer
		ReimportSoundBank = nullptr;
	}
	else
	{
		// otherwise, create a new object
		SoundBank = NewObject<UAtomSoundBank>(InParent, InClass, InName, Flags);
	}

	if (SoundBank)
	{
		// copy data to sound bank
		SoundBank->RawData.Lock(LOCK_READ_WRITE);
		void* LockedData = SoundBank->RawData.Realloc(BufferEnd - Buffer);
		FMemory::Memcpy(LockedData, Buffer, BufferEnd - Buffer);
		SoundBank->RawData.Unlock();

		// setup AssetImportData
		if (SoundBank->AssetImportData == nullptr)
		{
			SoundBank->AssetImportData = NewObject<UAssetImportData>(SoundBank, TEXT("AssetImportData"));
		}

		// update AssetImportData
		if (SoundBank->AssetImportData != nullptr)
		{
			SoundBank->AssetImportData->Update(CurrentFilename);
		}
	}

	return SoundBank;
}

bool UAtomSoundBankFactory::CanReimportSoundBank(UAtomSoundBank* AtomSoundBank, TArray<FString>& OutFilenames)
{
	if (AtomSoundBank && AtomSoundBank->AssetImportData)
	{
		AtomSoundBank->AssetImportData->ExtractFilenames(OutFilenames);

		if (OutFilenames.Num() > 0 && !PreferredReimportPath.IsEmpty() && FPaths::GetExtension(PreferredReimportPath) != FPaths::GetExtension(*OutFilenames[0]))
		{
			return false;
		}

		return true;
	}

	return false;
}

//~ Begin FReimportHandler interface

bool UAtomSoundBankFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	return false;
}

void UAtomSoundBankFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UAtomSoundBank* AtomSoundBank = Cast<UAtomSoundBank>(Obj);
	if (AtomSoundBank && ensure(NewReimportPaths.Num() == 1))
	{
		AtomSoundBank->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UAtomSoundBankFactory::Reimport(UObject* Obj)
{
	if (!Obj || !Obj->IsA(UAtomSoundBank::StaticClass()))
	{
		return EReimportResult::Failed;
	}

	ReimportSoundBank = Cast<UAtomSoundBank>(Obj);

	// Check source path
	FString ResolvedSourceFilePath = ReimportSoundBank->AssetImportData->GetFirstFilename();
	if (!ResolvedSourceFilePath.Len())
	{
		return EReimportResult::Failed;
	}

	UE_LOG(LogCriWareCoreEditor, Log, TEXT("Performing atomic reimport of [%s]"), *ResolvedSourceFilePath);

	bool bIsRelative = FPaths::IsRelative(ResolvedSourceFilePath);
	if (bIsRelative != false)
	{
		ResolvedSourceFilePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir(), ResolvedSourceFilePath);
	}

	// Ensure that the file provided by the path exists
	if (IFileManager::Get().FileSize(*ResolvedSourceFilePath) == INDEX_NONE)
	{
		UE_LOG(LogCriWareCoreEditor, Warning, TEXT("Cannot reimport: source file cannot be found."));
		return EReimportResult::Failed;
	}

	// Reimport path
	ReimportSoundBank->AssetImportData->Update(ResolvedSourceFilePath);

	auto Result = EReimportResult::Succeeded;

	// validation
	/*	// if movie is nor validated, nor validating, it means path is not loadable
		if (ManaMovie->IsValidating() || ManaMovie->Validate()) {
			Result = EReimportResult::Succeeded;

			UE_LOG(LogCriWareEditor, Log, TEXT("Imported successfully"));

			// FIXME block with a timeout until validation is possible for asset importation.
		}
		else {
			// restore
			FileManaMovie->SetFilePath(PrevFile);
		}
	}*/

	return Result;
}

#undef LOCTEXT_NAMESPACE
