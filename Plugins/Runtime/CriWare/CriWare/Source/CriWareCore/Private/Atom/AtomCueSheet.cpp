/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomCueSheet.h
 *
 ****************************************************************************/

#include "Atom/AtomCueSheet.h"

#include "Misc/Paths.h"
#include "EditorFramework/AssetImportData.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#include "CriWareCorePrivate.h"
#include "CriWareCoreSettings.h"
#include "CriWare.h"
#include "Atom/AtomCustomVersion.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomResourceManager.h"
#include "Atom/AtomCueSheetResource.h"
#include "Atom/AtomWaveBank.h"
#include "Atom/AtomSoundCue.h"

#define LOCTEXT_NAMESPACE "AtomCueSheet"

// Custom serialization version for AtomCueSheet asset
struct FAtomCueSheetCustomVersion
{
	enum Type
	{
		// Before any version changes were made in the plugin
		BeforeCustomVersionWasAdded = 0,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1,
	};

	// The GUID for this custom version number
	static const FGuid GUID;
};

const FGuid FAtomCueSheetCustomVersion::GUID(0x3D7628A7, 0xBEA44296, 0xAE294FBB, 0x4CF8BF34);
// Register the custom version with core
FCustomVersionRegistration GRegisterAtomCueSheetCustomVersion(FAtomCueSheetCustomVersion::GUID, FAtomCueSheetCustomVersion::LatestVersion, TEXT("AtomCueSheetVer"));

/*
 * UAtomCueSheet class
 *****************************************************************************/

UAtomCueSheet::UAtomCueSheet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ResourceData(nullptr)
	, ResourceSize(0)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());

	/*IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
	AssetRegistry.OnAssetAdded().AddLambda([](const FAssetData& InAssetData)
	{
		if (InAssetData.GetClass()->IsChildOf<UAtomCueSheet>())
		{
			if (UAtomCueSheet* CueSheet = CastChecked<UAtomCueSheet>(InAssetData.GetAsset()))
			{
				//AssetRegistry.AddPath(InAssetData.);
				//CueSheet->SoundCues;

				int i = 0;
			}
		}

		if (InAssetData.GetClass()->IsChildOf<UAtomSoundCue>())
		{
			int i = 0;
		}
	});*/
#endif
}

bool UAtomCueSheet::Validate() const
{
	return true;
}

int32 UAtomCueSheet::GetNumCues() const
{
	if (auto Resource = AtomResource.GetResource())
	{
		if (Resource->IsLoaded())
		{
			check(Resource->GetCueSheetResource());
			return Resource->GetCueSheetResource()->GetNumCues();
		}
	}

	return SoundCues.Num();
}

UAtomSoundCue* UAtomCueSheet::GetSoundCueByName(const FString& CueName) const
{
	for (int Index = 0; Index < GetNumCues(); Index++)
	{
		auto Cue = GetSoundCue(Index);
		if (Cue && Cue->CueInfo.Name == CueName)
		{
			return Cue;
			break;
		}
	}

	return nullptr;
}

UClass* UAtomCueSheet::GetSoundClass() const
{
	return UAtomSoundCue::StaticClass();
}

int UAtomCueSheet::GetNumSounds() const
{
	return GetNumCues();
}

UAtomSoundBase* UAtomCueSheet::GetSound(int Index) const
{
	return GetSoundCue(Index);
}

bool UAtomCueSheet::IsLoaded() const
{
	if (auto Resource = AtomResource.GetResource())
	{
		return Resource->IsLoaded();
	}

	return false;
}

FAtomResource* UAtomCueSheet::CreateAtomResource()
{
	// return a resource object for dealing with CueSheet data and info
	return new FAtomCueSheetResource(this);
}

bool UAtomCueSheet::InitAtomResource()
{
	// create the data for the resource

#if WITH_EDITOR
	check(!ResourceData);
	RawData.GetCopy((void**)&ResourceData, true);
#else
	if (!OwnedBulkDataPtr)
	{
		OwnedBulkDataPtr = RawData.StealFileMapping();
	}
	else
	{
		UE_LOG(LogCriWareAtom, Display, TEXT("CueSheet '%s' Has already had Load() called, and taken ownership of it's compressed data."), *GetFullName());
	}

	ResourceData = (const uint8*)OwnedBulkDataPtr->GetPointer();
	if (!ResourceData)
	{
		UE_LOG(LogCriWareAtom, Error, TEXT("CueSheet '%s' was not loaded when it should have been, forcing a sync load."), *GetFullName());

		delete OwnedBulkDataPtr;
		RawData.ForceBulkDataResident();
		OwnedBulkDataPtr = RawData.StealFileMapping();
		ResourceData = (const uint8*)OwnedBulkDataPtr->GetPointer();
		if (!ResourceData)
		{
			UE_LOG(LogCriWareAtom, Fatal, TEXT("CueSheet '%s' failed to load even after forcing a sync load."), *GetFullName());
			return false;
		}
	}
#endif
	return true;
}

const uint8* UAtomCueSheet::GetResourceData()
{
	// return the raw data
	return ResourceData;
}

void UAtomCueSheet::RemoveAtomResource()
{
	// free the data since the resouce does not need it anymore

#if WITH_EDITOR
	if (ResourceData)
	{
		FMemory::Free((void*)ResourceData);
		ResourceSize = 0;
		ResourceData = nullptr;
	}
#else
	delete OwnedBulkDataPtr;
	OwnedBulkDataPtr = nullptr;
	ResourceData = nullptr;
	ResourceSize = 0;
#endif
}

void UAtomCueSheet::DestroyAtomResource()
{
	// the resource is about to be fully destroyed by the resource manager, 
	// release the reference to the resource if retained
	if (IsRetainingResource())
	{
		ReleaseResource();
	}
}

bool UAtomCueSheet::CanUnloadAtomResource()
{
	EAtomSoundWaveLoadingBehavior CurrentLoadingBehavior = GetLoadingBehavior();
	if (CurrentLoadingBehavior != EAtomSoundWaveLoadingBehavior::RetainOnLoad)
	{
		return true;
	}

	return false;
}


/* UObject  overrides
*****************************************************************************/

void UAtomCueSheet::PostLoad()
{
	Super::PostLoad();

	if (GetOutermost()->HasAnyPackageFlags(PKG_ReloadingForCooker))
	{
		return;
	}

#if WITH_EDITORONLY_DATA
	if (GetLinkerCustomVersion(FAtomCustomVersion::GUID) < FAtomCustomVersion::LatestVersion)
	{
		// reload info from payload
		if (GCriWare)
		{
			if (auto AtomManager = GCriWare->GetAtomRuntimeManager())
			{
				// request the resource for this cue sheet
				FAtomLoadParams LoadParams;
				LoadParams.Source = this;
				LoadParams.RuntimeID = AtomManager->GetMainAtomRuntimeID();
				LoadParams.bAsyncLoad = false; // sync load
				if (FAtomResourceHandle TempResource = AtomManager->GetAtomResourceManager().RequestSoundResource(LoadParams))
				{
					if (FAtomCueSheetResource* Resource = TempResource->GetCueSheetResource())
					{
						for (int Index = 0; Index < Resource->GetNumCues(); Index++)
						{
							if (UAtomSoundCue* SoundCue = GetSoundCue(Index))
							{
								Resource->GetCueInfoByIndex(Index, SoundCue->CueInfo);
								Resource->GetWaveformInfo(SoundCue->CueInfo.ID, SoundCue->WaveInfo, true);
							}
						}
						Resource->Unload();
					}
					TempResource.Reset();
				}

				auto Package = GetOutermost();
				if (Package)
				{
					Package->SetDirtyFlag(true);
					Package->PackageMarkedDirtyEvent.Broadcast(Package, true);
				}
			}
		}
	}
#endif

	CacheInheritedLoadingBehavior();

	EAtomSoundWaveLoadingBehavior ActualLoadingBehavior = GetLoadingBehavior();

	if (IsStreaming(nullptr))
	{
		bool bShouldPrime = (ActualLoadingBehavior == EAtomSoundWaveLoadingBehavior::PrimeOnLoad);
		bShouldPrime |= (GIsEditor && (ActualLoadingBehavior == EAtomSoundWaveLoadingBehavior::RetainOnLoad)); // treat this scenario like PrimeOnLoad

		if (bShouldPrime)
		{
			RetainResource();
		}
	}
}

void UAtomCueSheet::BeginDestroy()
{
	Super::BeginDestroy();
}

void UAtomCueSheet::FinishDestroy()
{
	Super::FinishDestroy();

	ReleaseResource();
}

bool UAtomCueSheet::IsReadyForFinishDestroy()
{
	// GC cannot collect while a loading task is running
	bool bIsReady = AtomResource.IsValid() ? !AtomResource.GetResource()->IsAsyncLoading() : true;

	// todo stop all playback
	return Super::IsReadyForFinishDestroy() && bIsReady;
}


#if WITH_EDITOR
void UAtomCueSheet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomCueSheet, WaveBanks))
	{
		if (GCriWare)
		{
			if (auto RuntimeManager = GCriWare->GetAtomRuntimeManager())
			{
				RuntimeManager->StopSoundsUsingResource(this);
				RuntimeManager->FreeResource(this);
			}
		}

		for (auto SoundCue : SoundCues)
		{
			SoundCue->GenerateWaveSnapshot();
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UAtomCueSheet::PostInitProperties()
{
#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
	Super::PostInitProperties();
}

void UAtomCueSheet::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FAtomCueSheetCustomVersion::GUID);

#if WITH_EDITORONLY_DATA
	if ((Ar.IsLoading()) && (AssetImportData == nullptr))
	{
		// AssetImportData should always be valid
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
};

#if WITH_EDITORONLY_DATA
void UAtomCueSheet::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
    PRAGMA_DISABLE_DEPRECATION_WARNINGS;
    Super::GetAssetRegistryTags(OutTags);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS;
#else
    if (AssetImportData)
    {
        OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
    }
    
    Super::GetAssetRegistryTags(OutTags);
#endif
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
void UAtomCueSheet::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
    Super::GetAssetRegistryTags(Context);
    
	if (AssetImportData)
	{
        Context.AddTag(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
	}
}
#endif
#endif

#undef LOCTEXT_NAMESPACE
