/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomWaveBank.h
 *
 ****************************************************************************/

 #include "Atom/AtomWaveBank.h"

#include "Misc/Paths.h"
#include "EditorFramework/AssetImportData.h"

#include "CriWareCorePrivate.h"
#include "CriWareCoreSettings.h"
#include "CriWare.h"
#include "Atom/AtomCustomVersion.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomResourceManager.h"
#include "Atom/AtomWaveBankResource.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomSoundWave.h"

#define LOCTEXT_NAMESPACE "AtomWaveBank"

 // Custom serialization version for AtomWaveBank asset
struct FAtomWaveBankCustomVersion
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

const FGuid FAtomWaveBankCustomVersion::GUID(0x69813C22, 0xB62B43D4, 0x89408B89, 0x9F946025);
// Register the custom version with core
FCustomVersionRegistration GRegisterAtomWaveBankCustomVersion(FAtomWaveBankCustomVersion::GUID, FAtomWaveBankCustomVersion::LatestVersion, TEXT("AtomWaveBankVer"));

 /*
  * UAtomWaveBank class
  *****************************************************************************/

UAtomWaveBank::UAtomWaveBank(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ResourceData(nullptr)
	, ResourceSize(0)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

bool UAtomWaveBank::Validate() const
{
	return true;
}

int32 UAtomWaveBank::GetNumWaves() const
{
	if (auto Resource = AtomResource.GetResource())
	{
		if (Resource->IsLoaded())
		{
			check(Resource->GetWaveBankResource());
			return Resource->GetWaveBankResource()->GetNumWaves();
		}
	}

	return SoundWaves.Num();
}

UClass* UAtomWaveBank::GetSoundClass() const
{
	return UAtomSoundWave::StaticClass();
}

int UAtomWaveBank::GetNumSounds() const
{
	return GetNumWaves();
}

UAtomSoundBase* UAtomWaveBank::GetSound(int Index) const
{
	return GetSoundWave(Index);
}


bool UAtomWaveBank::IsLoaded() const
{
	if (auto Resource = AtomResource.GetResource())
	{
		return Resource->IsLoaded();
	}

	return false;
}

FAtomResource* UAtomWaveBank::CreateAtomResource()
{
	return new FAtomWaveBankResource(this);
}

bool UAtomWaveBank::InitAtomResource()
{
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
		UE_LOG(LogCriWareAtom, Display, TEXT("WaveBank '%s' Has already had Load() called, and taken ownership of it's compressed data."), *GetFullName());
	}

	ResourceData = (const uint8*)OwnedBulkDataPtr->GetPointer();
	if (!ResourceData)
	{
		UE_LOG(LogCriWareAtom, Error, TEXT("WaveBank '%s' was not loaded when it should have been, forcing a sync load."), *GetFullName());

		delete OwnedBulkDataPtr;
		RawData.ForceBulkDataResident();
		OwnedBulkDataPtr = RawData.StealFileMapping();
		ResourceData = (const uint8*)OwnedBulkDataPtr->GetPointer();
		if (!ResourceData)
		{
			UE_LOG(LogCriWareAtom, Fatal, TEXT("WaveBank '%s' failed to load even after forcing a sync load."), *GetFullName());
			return false;
		}
	}
#endif
	return true;
}

const uint8* UAtomWaveBank::GetResourceData()
{
	return ResourceData;
}

void UAtomWaveBank::RemoveAtomResource()
{
#if WITH_EDITOR
		if (ResourceData)
		{
			FMemory::Free((void*)ResourceData);
			ResourceSize = 0;
			ResourceData = nullptr;
		}
#else
	if (OwnedBulkDataPtr)
	{
		delete OwnedBulkDataPtr;
		OwnedBulkDataPtr = nullptr;
		ResourceData = nullptr;
		ResourceSize = 0;
	}
#endif
}

void UAtomWaveBank::DestroyAtomResource()
{
	if (IsRetainingResource())
	{
		ReleaseResource();
	}
}

bool UAtomWaveBank::CanUnloadAtomResource()
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

void UAtomWaveBank::PostLoad()
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
				// request the resource for this wave bank
				FAtomLoadParams LoadParams;
				LoadParams.Source = this;
				LoadParams.RuntimeID = AtomManager->GetMainAtomRuntimeID();
				LoadParams.bAsyncLoad = false; // sync load
				if (FAtomResourceHandle TempResource = AtomManager->GetAtomResourceManager().RequestSoundResource(LoadParams))
				{
					if (FAtomWaveBankResource* Resource = TempResource->GetWaveBankResource())
					{
						for (int Index = 0; Index < Resource->GetNumWaves(); Index++)
						{
							if (UAtomSoundWave* Sound = GetSoundWave(Index))
							{
								int32 WaveID = INDEX_NONE;
								Resource->GetWaveIDByIndex(Index, WaveID);
								Resource->GetWaveformInfo(WaveID, Sound->WaveInfo);
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

void UAtomWaveBank::BeginDestroy()
{
	Super::BeginDestroy();

	if (IsRetainingResource())
	{
		ReleaseResource();
	}
}

void UAtomWaveBank::FinishDestroy()
{
	Super::FinishDestroy();
}

bool UAtomWaveBank::IsReadyForFinishDestroy()
{
	// GC cannot collect while a loading task is running
	bool bIsReady = true;// AtomResource.IsValid() ? !AtomResource.GetResource()->IsAsyncLoading() : true;

	// todo stop all playback
	return Super::IsReadyForFinishDestroy() && bIsReady;
}

/*#if WITH_EDITOR
void UAtomWaveBank::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif*/

void UAtomWaveBank::PostInitProperties()
{
#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad)) {
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
	Super::PostInitProperties();
}

void UAtomWaveBank::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FAtomWaveBankCustomVersion::GUID);

#if WITH_EDITORONLY_DATA
	if ((Ar.IsLoading()) && (AssetImportData == nullptr))
	{
		// AssetImportData should always be valid
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
#endif
};

#if WITH_EDITORONLY_DATA
void UAtomWaveBank::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
    PRAGMA_DISABLE_DEPRECATION_WARNINGS;
    Super::GetAssetRegistryTags(OutTags);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS;
#else
    Super::GetAssetRegistryTags(OutTags);
    
    if (AssetImportData)
    {
        OutTags.Add(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(), FAssetRegistryTag::TT_Hidden));
    }
#endif
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
void UAtomWaveBank::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
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
