/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomConfigFactory.cpp
 *
 ****************************************************************************/

#include "AtomConfigFactory.h"

#include "EditorFramework/AssetImportData.h"
#include "Misc/FeedbackContext.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Runtime/Launch/Resources/Version.h"

#include "CriWare.h"
#include "CriWareCoreSettings.h"
#include "Atom/AtomConfig.h"
#include "Atom/AtomRack.h"
#include "Atom/AtomRuntime.h"

#include "CriWareCoreEditorPrivate.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

UAtomConfigFactory::UAtomConfigFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ReimportConfig(nullptr)
{
	Formats.Add(TEXT("acf;Atom Config File"));

	SupportedClass = UAtomConfig::StaticClass();
	bText = false;
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UAtomConfigFactory::FactoryCreateBinary(
	UClass* Class,
	UObject* InParent,
	FName Name,
	EObjectFlags Flags,
	UObject* Context,
	const TCHAR* FileType,
	const uint8*& Buffer,
	const uint8* BufferEnd,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled)
{
	UAtomConfig* AtomConfig = nullptr;

	if (ReimportConfig)
	{
		// reuse original object to reimport 
		AtomConfig = ReimportConfig;

		// clear reimport pointer
		ReimportConfig = nullptr;
	}
	else
	{
		AtomConfig = NewObject<UAtomConfig>(InParent, Name, Flags);
	}

	if (!AtomConfig)
	{
		return nullptr;
	}

	// Copy binary to AtomConfig raw data
	AtomConfig->RawData.Lock(LOCK_READ_WRITE);
	void* LockedData = AtomConfig->RawData.Realloc(BufferEnd - Buffer);
	if (LockedData)
	{
		FMemory::Memcpy(LockedData, Buffer, BufferEnd - Buffer);
		AtomConfig->RawData.Unlock();

		if (!AtomConfig->LoadSettings())
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("AtomConfig '%s' is malformed or version is incompatible with ADX Atom runtime."), *Name.ToString());
			return nullptr;
		}
	}
	else
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Can't get memory to import AtomConfig '%s'."), *Name.ToString());
		return nullptr;
	}

	if (!AtomConfig->AssetImportData)
	{
		AtomConfig->AssetImportData = NewObject<UAssetImportData>(AtomConfig, TEXT("AssetImportData"));
	}

	if (AtomConfig->AssetImportData)
	{
		AtomConfig->AssetImportData->Update(CurrentFilename);
	}

	// Automatic registration to plugin settings
	if (AtomConfig && GCriWare)
	{
		auto Settings = GetMutableDefault<UCriWareCoreSettings>();

		FText MessageBoxTitle = LOCTEXT("AtomConfigImportTitle", "Import Atom Configuration File");
		FText MessageBoxContent = LOCTEXT("AtomConfigImportWarning", "Do you want to update Atom plugin with this Atom Configuration?");

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
		auto Result = FMessageDialog::Open(EAppMsgType::YesNoCancel, MessageBoxContent, MessageBoxTitle);
#else
		auto Result = FMessageDialog::Open(EAppMsgType::YesNoCancel, MessageBoxContent, &MessageBoxTitle);
#endif
		bool bCancelByUser = Result == EAppReturnType::Type::Cancel;
		bool bSetupToSettings = Result == EAppReturnType::Type::Yes;

		if (bCancelByUser)
		{
			bOutOperationCanceled = true;
			return nullptr;
		}

		if (bSetupToSettings)
		{
			bool bSetupToMaster = Settings->ShowApplyAtomConfigToMasterRackDialog(AtomConfig, bCancelByUser);

			if (bCancelByUser)
			{
				bOutOperationCanceled = true;
				return nullptr;
			}

			FAtomRuntime* ActiveAtomRuntime = nullptr;
			if (FAtomRuntimeHandle RuntimeHandle = GCriWare->GetActiveAtomRuntime())
			{
				ActiveAtomRuntime = RuntimeHandle.GetAtomRuntime();
				if (ActiveAtomRuntime)
				{
					ActiveAtomRuntime->Deactivate();
				}
			}

			// Overwrite plugin settings
			Settings->AtomConfig = FSoftObjectPath(AtomConfig);
			Settings->SaveConfig(CPF_Config, *Settings->GetDefaultConfigFilename());

			// Register to current CriWare library
			if (!GCriWare->SetAtomConfiguration(AtomConfig))
			{
				Warn->Logf(ELogVerbosity::Error, TEXT("Failed to register AtomConfig '%s' to CriWare libary."), *Name.ToString());
			}
			else
			{
				if (bSetupToMaster)
				{
					if (auto MasterRackAsset = Cast<UAtomRack>(Settings->MasterRack.TryLoad()))
					{
						MasterRackAsset->SetDspBusSetting(AtomConfig->GetDefaultDspBusSetting(), true);
						MasterRackAsset->MarkPackageDirty();
					}
				}
			}

			if (ActiveAtomRuntime)
			{
				ActiveAtomRuntime->Activate();
			}
		}
	}

	return AtomConfig;
}

bool UAtomConfigFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UAtomConfig* AtomConfig = Cast<UAtomConfig>(Obj);

	if (AtomConfig && AtomConfig->AssetImportData) 
	{
		AtomConfig->AssetImportData->ExtractFilenames(OutFilenames);
		return true;
	}

	return false;
}

void UAtomConfigFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UAtomConfig* AtomConfig = Cast<UAtomConfig>(Obj);
	if (AtomConfig && ensure(NewReimportPaths.Num() == 1))
	{
		AtomConfig->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UAtomConfigFactory::Reimport(UObject* Obj)
{
	if (!Obj || !Obj->IsA(UAtomConfig::StaticClass()))
	{
		return EReimportResult::Failed;
	}

	UAtomConfig* AtomConfig = Cast<UAtomConfig>(Obj);
	ReimportConfig = AtomConfig;

	// Check source path
	FString ResolvedSourceFilePath = AtomConfig->AssetImportData->GetFirstFilename();
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

	// Remove config set to current Atom runtime if used
	if (GCriWare->GetAtomConfiguration() == AtomConfig)
	{
		GCriWare->ResetAtomConfiguration();
	}

	bool bImportWasCanceled = false;
	if (ImportObject(AtomConfig->GetClass(), AtomConfig->GetOuter(), *AtomConfig->GetName(), RF_Public | RF_Standalone, *ResolvedSourceFilePath, nullptr, bImportWasCanceled))
	{
		UE_LOG(LogCriWareCoreEditor, Log, TEXT("Imported successfully"));

		// Try to find the outer package so we can dirty it up
		if (AtomConfig->GetOuter())
		{
			AtomConfig->GetOuter()->MarkPackageDirty();
		}
		else
		{
			AtomConfig->MarkPackageDirty();
		}
	}
	else if (bImportWasCanceled)
	{
		UE_LOG(LogCriWareCoreEditor, Warning, TEXT("-- import canceled"));
		return EReimportResult::Cancelled;
	}
	else
	{
		UE_LOG(LogCriWareCoreEditor, Warning, TEXT("-- import failed"));
		return EReimportResult::Failed;
	}

	return EReimportResult::Succeeded;
}

#undef LOCTEXT_NAMESPACE
