/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomWaveBankFactory.cpp
 *
 ****************************************************************************/

 #include "AtomWaveBankFactory.h"

#include "EditorFramework/AssetImportData.h"

#include "CriWare.h"
#include "CriWareCoreSettings.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomResourceManager.h"
#include "Atom/AtomResource.h"
#include "Atom/AtomWaveBank.h"
#include "Atom/AtomWaveBankResource.h"
#include "Atom/AtomSoundWave.h"

#include "CriWareCoreEditorPrivate.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

UAtomWaveBankFactory::UAtomWaveBankFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Formats.Add(TEXT("awb;Atom Wave Bank File"));

	SupportedClass = UAtomWaveBank::StaticClass();
	bText = false;
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UAtomWaveBankFactory::FactoryCreateBinary(
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
	UObject* Obj = Super::FactoryCreateBinary(InClass, InParent, InName, Flags, Context, FileType, Buffer, BufferEnd, Warn, bOutOperationCanceled);
	UAtomWaveBank* AtomWaveBank = Cast<UAtomWaveBank>(Obj);

	if (AtomWaveBank)
	{
		auto Settings = GetDefault<UCriWareCoreSettings>();

		// load sub waves
		if (GCriWare)
		{
			if (auto AtomManager = GCriWare->GetAtomRuntimeManager())
			{
				// request the resource for this wave bank
				FAtomLoadParams LoadParams;
				LoadParams.Source = AtomWaveBank;
				LoadParams.RuntimeID = AtomManager->GetMainAtomRuntimeID();
				LoadParams.bAsyncLoad = false; // sync load
				auto AtomResource = AtomManager->GetAtomResourceManager().RequestSoundResource(LoadParams);

				// If we hit this, something went wrong in RequestSoundResource.
				if (!AtomResource.IsValid())
				{
					UE_LOG(LogCriWareCoreEditor, Display, TEXT("Failed to retrieve wave bank data from AtomWaveBank %s after successfully requesting it!"), *(AtomWaveBank->GetName()));
				}
				else
				{
					if (auto Resource = AtomResource.GetResource())
					{
						check(Resource->GetWaveBankResource());
						auto WaveBankResource = Resource->GetWaveBankResource();

						// Create sub waves.
						for (int Index = 0; Index < WaveBankResource->GetNumWaves(); Index++)
						{
							int WaveID = INDEX_NONE;
							if (WaveBankResource->GetWaveIDByIndex(Index, WaveID))
							{
								// try to get if exist otherwise create it (must match index, name and id)
								if (UAtomSoundWave* SoundWave = AtomWaveBank->GetSoundWave(Index))
								{
									// Keep existing, only udpate WaveInfo.
									SoundWave->Init(AtomWaveBank, WaveID);
									WaveBankResource->GetWaveformInfo(WaveID, SoundWave->WaveInfo);
								}
								else
								{
									// Note: [AssetName]_[number] is reserved for asset variations in unreal convention names. So we prefer to use number within the assetname.
									FString SoundWaveName = FString::Printf(TEXT("Wave%d"), Index);
									//FString SoundWaveName = FString::Printf(TEXT("%s_Wave%d"), *AtomWaveBank->GetName(), Index);
									UAtomSoundWave* NewSoundWave = NewObject<UAtomSoundWave>(AtomWaveBank, UAtomSoundWave::StaticClass(), *SoundWaveName, RF_Public);
									NewSoundWave->Init(AtomWaveBank, WaveID);
									WaveBankResource->GetWaveformInfo(WaveID, NewSoundWave->WaveInfo);

									AtomWaveBank->SoundWaves.Add(NewSoundWave);
								}
							}
							else
							{
								UE_LOG(LogCriWareCoreEditor, Warning, TEXT("Failed to retrieve identifier for wave index %d from AWB data in AtomWaveBank %s!"), Index, *(AtomWaveBank->GetName()));
							}
						}

						// trim to new wave size.
						AtomWaveBank->SoundWaves.SetNum(WaveBankResource->GetNumWaves());

						Resource->Unload();
					}

					AtomResource.Reset();

					for (auto SoundWave : AtomWaveBank->SoundWaves)
					{
						SoundWave->GenerateWaveSnapshot();
						SoundWave->SetRedrawThumbnail(true);
					}
				}
			}
		}

		AtomWaveBank->PostImport();
	}

	return AtomWaveBank;
}

//~ Begin FReimportHandler interface

bool UAtomWaveBankFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UAtomWaveBank* AtomWaveBank = Cast<UAtomWaveBank>(Obj);
	return CanReimportSoundBank(AtomWaveBank, OutFilenames);
}

EReimportResult::Type UAtomWaveBankFactory::Reimport(UObject* Obj)
{
	if (!Obj || !Obj->IsA(UAtomWaveBank::StaticClass()))
	{
		return EReimportResult::Failed;
	}

	UAtomWaveBank* AtomWaveBank = Cast<UAtomWaveBank>(Obj);

	auto Result = Super::Reimport(Obj);

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

	if (Result == EReimportResult::Succeeded)
	{	
		bool bCancelled = false;
		FString ResolvedSourceFilePath = AtomWaveBank->AssetImportData->GetFirstFilename();
		UObject* ResultObject = ImportObject(AtomWaveBank->GetClass(), AtomWaveBank->GetOuter(), *AtomWaveBank->GetName(), RF_Public | RF_Standalone, *ResolvedSourceFilePath, nullptr, bCancelled);

		AtomWaveBank = Cast<UAtomWaveBank>(ResultObject);

		// check results
		if (!AtomWaveBank) {
			if (bCancelled) {
				UE_LOG(LogCriWareCoreEditor, Warning, TEXT("-- import canceled"));
				return EReimportResult::Cancelled;
			}

			UE_LOG(LogCriWareCoreEditor, Warning, TEXT("-- import failed"));
			return EReimportResult::Failed;
		}

		UE_LOG(LogCriWareCoreEditor, Log, TEXT("-- imported successfully"));

		// Try to find the outer package so we can dirty it up
		if (AtomWaveBank->GetOuter())
		{
			AtomWaveBank->GetOuter()->MarkPackageDirty();
		}
		else
		{
			AtomWaveBank->MarkPackageDirty();
		}
	}
	else
	{
		UE_LOG(LogCriWareCoreEditor, Warning, TEXT("-- import failed"));
		return EReimportResult::Failed;
	}

	return EReimportResult::Succeeded;
}

#undef LOCTEXT_NAMESPACE
