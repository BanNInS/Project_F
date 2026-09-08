/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomCueSheetFactory.cpp
 *
 ****************************************************************************/

#include "AtomCueSheetFactory.h"

#include "EditorFramework/AssetImportData.h"
#include "ObjectTools.h" // todo: remove usage

#include "CriWare.h"
#include "CriWareCoreSettings.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomResource.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomCueSheetResource.h"
#include "Atom/AtomSoundCue.h"

#include "CriWareCoreEditorPrivate.h"

#define LOCTEXT_NAMESPACE "AtomEditorFactories"

UAtomCueSheetFactory::UAtomCueSheetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Formats.Add(TEXT("acb;Atom CueSheet Binary (Cue Bank) File"));

	SupportedClass = UAtomCueSheet::StaticClass();
	bText = false;
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UAtomCueSheetFactory::FactoryCreateBinary(
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
	UAtomCueSheet* AtomCueSheet = Cast<UAtomCueSheet>(Obj);

	if (AtomCueSheet)
	{
		auto Settings = GetDefault<UCriWareCoreSettings>();

		auto GetWaveformInfoForCue = [](FAtomCueSheetResource* CueSheetResource, int32 CueID)
		{
			FAtomWaveInfo WaveInfo;

			if (CueSheetResource->GetWaveformInfo(CueID, WaveInfo, true) == false)
			{
				// This is a pure action cue. We need to set some default values for dynamic voice generators.
				WaveInfo.NumChannels = 12; // default max
				WaveInfo.SampleRate = 48000; // default max
				WaveInfo.Format = EAtomFormat::HCA; // default - plays ADX and HCA
				WaveInfo.bIsStreamed = false; // action cue don't stream
			}

			return MoveTemp(WaveInfo);
		};

		// load sub cues
		if (GCriWare)
		{
			if (auto AtomManager = GCriWare->GetAtomRuntimeManager())
			{
				// request the resource for this cue sheet
				FAtomLoadParams LoadParams;
				LoadParams.Source = AtomCueSheet;
				LoadParams.RuntimeID = AtomManager->GetMainAtomRuntimeID();
				LoadParams.bAsyncLoad = false; // sync load
				auto AtomResource = AtomManager->GetAtomResourceManager().RequestSoundResource(LoadParams);

				// If we hit this, something went wrong in RequestSoundResource.
				if (!AtomResource.IsValid())
				{
					UE_LOG(LogCriWareCoreEditor, Display, TEXT("Failed to retrieve cue sheet data from AtomCueSheet %s after successfully requesting it!"), *(AtomCueSheet->GetName()));
				}
				else
				{
					if (auto Resource = AtomResource.GetResource())
					{
						check(Resource->GetCueSheetResource());
						auto CueSheetResource = Resource->GetCueSheetResource();
						
						// get wave bank slots
						uint16 NumWaveBankSlots = CueSheetResource->GetNumWaveBankSlots();
						if (AtomCueSheet->WaveBanks.Num() != NumWaveBankSlots)
						{
							AtomCueSheet->WaveBanks.AddDefaulted(NumWaveBankSlots);
						}

						// Create sub cues.
						for (int Index = 0; Index < CueSheetResource->GetNumCues(); Index++)
						{
							FAtomCueInfo CueInfo = {};
							if (CueSheetResource->GetCueInfoByIndex(Index, CueInfo))
							{
								// try to get if exist otherwise create it (must match index, name and id)
								if (UAtomSoundCue* SoundCue = AtomCueSheet->GetSoundCue(Index))
								{
									if (SoundCue->CueInfo.Name == CueInfo.Name && SoundCue->CueInfo.ID == CueInfo.ID)
									{
										// Keep existing, only udpate CueInfo and first WaveInfo.
										SoundCue->Init(AtomCueSheet, CueInfo);
										SoundCue->WaveInfo = GetWaveformInfoForCue(CueSheetResource, CueInfo.ID);
									}
									else
									{
										// Replace the sound cue with new one
										//FString SoundCueName = FString::Printf(TEXT("%s_%s"), *AtomCueSheet->GetName(), *CueInfo.Name);
										FString SoundCueName = FString::Printf(TEXT("%s"), *CueInfo.Name);
										FString SanitizedName = ObjectTools::SanitizeObjectName(SoundCueName); // todo: use cri sanitizer
										UAtomSoundCue* NewSoundCue = NewObject<UAtomSoundCue>(AtomCueSheet, UAtomSoundCue::StaticClass(), *SanitizedName, RF_Public);
										NewSoundCue->Init(AtomCueSheet, CueInfo);
										NewSoundCue->WaveInfo = GetWaveformInfoForCue(CueSheetResource, CueInfo.ID);

										AtomCueSheet->SoundCues[Index] = NewSoundCue;
									}
								}
								else
								{
									// append the sound cue
									//FString SoundCueName = FString::Printf(TEXT("%s_%s"), *AtomCueSheet->GetName(), *CueInfo.Name);
									FString SoundCueName = FString::Printf(TEXT("%s"), *CueInfo.Name);
									FString SanitizedName = ObjectTools::SanitizeObjectName(SoundCueName); // todo: use cri sanitizer
									UAtomSoundCue* NewSoundCue = NewObject<UAtomSoundCue>(AtomCueSheet, UAtomSoundCue::StaticClass(), *SanitizedName, RF_Public);
									NewSoundCue->Init(AtomCueSheet, CueInfo);
									NewSoundCue->WaveInfo = GetWaveformInfoForCue(CueSheetResource, CueInfo.ID);

									AtomCueSheet->SoundCues.Add(NewSoundCue);
								}
							}
							else
							{
								UE_LOG(LogCriWareCoreEditor, Warning, TEXT("Failed to retrieve info for cue index %d from ACB data in AtomCueSheet %s!"), Index, *(AtomCueSheet->GetName()));
							}
						}

						// trim to new cue size.
						AtomCueSheet->SoundCues.SetNum(CueSheetResource->GetNumCues());

						Resource->Unload();
					}
					
					AtomResource.Reset();

					for (auto SoundCue : AtomCueSheet->SoundCues)
					{
						SoundCue->GenerateWaveSnapshot();
						SoundCue->SetRedrawThumbnail(true);
					}
				}
			}
		}

		AtomCueSheet->PostImport();
	}

	return AtomCueSheet;
}

//~ Begin FReimportHandler interface
bool UAtomCueSheetFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	UAtomCueSheet* AtomCueSheet = Cast<UAtomCueSheet>(Obj);
	return CanReimportSoundBank(AtomCueSheet, OutFilenames);
}

EReimportResult::Type UAtomCueSheetFactory::Reimport(UObject* Obj)
{
	if (!Obj || !Obj->IsA(UAtomCueSheet::StaticClass()))
	{
		return EReimportResult::Failed;
	}

	UAtomCueSheet* AtomCueSheet = Cast<UAtomCueSheet>(Obj);

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
		FString ResolvedSourceFilePath = AtomCueSheet->AssetImportData->GetFirstFilename();
		UObject* ResultObject = ImportObject(AtomCueSheet->GetClass(), AtomCueSheet->GetOuter(), *AtomCueSheet->GetName(), RF_Public | RF_Standalone, *ResolvedSourceFilePath, nullptr, bCancelled);

		AtomCueSheet = Cast<UAtomCueSheet>(ResultObject);

		// check results
		if (!AtomCueSheet) {
			if (bCancelled) {
				UE_LOG(LogCriWareCoreEditor, Warning, TEXT("-- import canceled"));
				return EReimportResult::Cancelled;
			}

			UE_LOG(LogCriWareCoreEditor, Warning, TEXT("-- import failed"));
			return EReimportResult::Failed;
		}

		UE_LOG(LogCriWareCoreEditor, Log, TEXT("-- imported successfully"));

		// Try to find the outer package so we can dirty it up
		if (AtomCueSheet->GetOuter())
		{
			AtomCueSheet->GetOuter()->MarkPackageDirty();
		}
		else
		{
			AtomCueSheet->MarkPackageDirty();
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
