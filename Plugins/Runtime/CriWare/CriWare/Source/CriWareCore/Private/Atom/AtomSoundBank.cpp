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

#include "Atom/AtomSoundBank.h"

#include "EditorFramework/AssetImportData.h"
#include "Misc/CommandLine.h"
#include "Interfaces/ITargetPlatform.h"
#include "Interfaces/ITargetPlatformManagerModule.h"

#include "CriWare.h"
#include "CriWareCorePrivate.h"
#include "CriWareCoreSettings.h"
#include "CriWarePlatformUtils.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomCustomVersion.h"

#define LOCTEXT_NAMESPACE "AtomSoundBank"

#define ATOM_TRACE_SOUND_SERIALIZE 0

static int32 AtomSoundBankDefaultLoadingBehaviorCVar = 0;
FAutoConsoleVariableRef CVarAtomSoundBankDefaultLoadingBehavior(
	TEXT("atom.streamcache.AtomSoundBankDefaultLoadingBehavior"),
	AtomSoundBankDefaultLoadingBehaviorCVar,
	TEXT("This can be set to define the default behavior when a UAtomSoundBank (UAtomCueSheet or UAtomWaveBank) is loaded.\n")
	TEXT("0: Default (load on demand), 1: Retain audio data on load, 2: prime audio data on load, 3: load on demand (No audio data is loaded until a UAtomSoundBank is played or primed)."),
	ECVF_Default);

static int32 ForceAtomNonStreamingInEditorCVar = 0;
FAutoConsoleVariableRef CVarAtomForceNonStreamingSoundInEditor(
	TEXT("atom.editor.ForceAtomNonStreaming"),
	ForceAtomNonStreamingInEditorCVar,
	TEXT("When set to 1, forces any audio played by Atom to be non-streaming. May force a DDC miss.\n")
	,//TEXT("0: Honor the Play When Silent flag, 1: stop all silent non-procedural sources."),
	ECVF_Default);

/*
 * UAtomSoundBank class
 *****************************************************************************/

UAtomSoundBank::UAtomSoundBank(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CachedSoundWaveLoadingBehavior(EAtomSoundWaveLoadingBehavior::Uninitialized)
{

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

void UAtomSoundBank::Serialize(FArchive& Ar)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UAtomSoundBank::Serialize"), STAT_AtomSoundBank_Serialize, STATGROUP_LoadTime);

	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FAtomCustomVersion::GUID);

	bool bCooked = Ar.IsCooking();
	Ar << bCooked;

	if (FPlatformProperties::RequiresCookedData() && !bCooked && Ar.IsLoading())
	{
		UE_LOG(LogCriWareAtom, Fatal, TEXT("This platform requires cooked packages, and Atom data was not cooked into %s."), *GetFullName());
	}

	bool bShouldStreamSound = false;

	if (Ar.IsSaving() || Ar.IsCooking())
	{
#if WITH_ENGINE
		// If there is an AutoStreamingThreshold set for the platform we're cooking to,
		// we use it to determine whether this USoundWave should be streaming:
		const ITargetPlatform* CookingTarget = Ar.CookingTarget();
		if (CookingTarget != nullptr)
		{
			bShouldStreamSound = IsStreaming(*CookingTarget->IniPlatformName());
		}
		else
#endif
		{
			bShouldStreamSound = IsStreaming(nullptr);
		}
	}
	else
	{
		bShouldStreamSound = IsStreaming(nullptr);
	}

	if (bCooked)
	{
		if (!bShouldStreamSound)
		{
			if (Ar.IsCooking())
			{
#if WITH_ENGINE
				const ITargetPlatform* CookingTarget = Ar.CookingTarget();
				bool bMapped = CookingTarget->SupportsFeature(ETargetPlatformFeatures::MemoryMappedFiles) && CookingTarget->SupportsFeature(ETargetPlatformFeatures::MemoryMappedAudio);

				const uint32 OldBulkDataFlags = RawData.GetBulkDataFlags();

				if (bMapped)
				{
#if ATOM_TRACE_SOUND_SERIALIZE
					UE_LOG(LogTemp, Warning, TEXT("Cook mapped streamed")); // mapped always saved not inline
#endif
					RawData.SetBulkDataFlags(BULKDATA_Force_NOT_InlinePayload);
					RawData.SetBulkDataFlags(BULKDATA_MemoryMappedPayload);
					RawData.ClearBulkDataFlags(BULKDATA_ForceInlinePayload);
				}
				else // inline
				{
#if ATOM_TRACE_SOUND_SERIALIZE
					UE_LOG(LogTemp, Warning, TEXT("Cook not mapped inline"));
#endif
					RawData.SetBulkDataFlags(BULKDATA_ForceInlinePayload);
					RawData.ClearBulkDataFlags(BULKDATA_Force_NOT_InlinePayload | BULKDATA_MemoryMappedPayload);
				}
				
				RawData.SetBulkDataFlags(BULKDATA_SingleUse);
				RawData.Serialize(Ar, this, INDEX_NONE, false);
				RawData.ClearBulkDataFlags(0xFFFFFFFF);
				RawData.SetBulkDataFlags(OldBulkDataFlags);
#endif
			}
			else // loading in game
			{
				if (FPlatformProperties::SupportsMemoryMappedFiles() && FPlatformProperties::SupportsMemoryMappedAudio())
				{
#if ATOM_TRACE_SOUND_SERIALIZE
					UE_LOG(LogTemp, Warning, TEXT("Load mapped inline"));
#endif
					RawData.Serialize(Ar, this, INDEX_NONE, true);
				}
				else
				{
#if ATOM_TRACE_SOUND_SERIALIZE
					UE_LOG(LogTemp, Warning, TEXT("Load not mapped inline"));
#endif
					RawData.Serialize(Ar, this, INDEX_NONE, false);
				}
			}
		}
		else // streamed
		{
			if (Ar.IsCooking())
			{
#if WITH_ENGINE
				const ITargetPlatform* CookingTarget = Ar.CookingTarget();
				const uint32 OldBulkDataFlags = RawData.GetBulkDataFlags();

#if ATOM_TRACE_SOUND_SERIALIZE
				UE_LOG(LogTemp, Warning, TEXT("Cook not mapped streamed"));
#endif

				RawData.SetBulkDataFlags(BULKDATA_Force_NOT_InlinePayload);
				RawData.ClearBulkDataFlags(BULKDATA_ForceInlinePayload);
				
				RawData.SetBulkDataFlags(BULKDATA_SingleUse);
				RawData.Serialize(Ar, this, INDEX_NONE, false);
				RawData.ClearBulkDataFlags(0xFFFFFFFF);
				RawData.SetBulkDataFlags(OldBulkDataFlags);
#endif
			}
			else // loading in game
			{
#if ATOM_TRACE_SOUND_SERIALIZE
				UE_LOG(LogTemp, Warning, TEXT("Load not mapped streamed"));
#endif
				RawData.Serialize(Ar, this, INDEX_NONE, false);
			}
		}
	}
	else if (!Ar.IsObjectReferenceCollector()) // !cooking or cooked (editor) - Skip bulk data serialization when collecting references
	{
		if (bShouldStreamSound)
		{
			if (Ar.IsSaving())
			{
				const uint32 OldBulkDataFlags = RawData.GetBulkDataFlags();

#if ATOM_TRACE_SOUND_SERIALIZE
				UE_LOG(LogTemp, Warning, TEXT("Save [Editor]"));
#endif
				RawData.SetBulkDataFlags(BULKDATA_Force_NOT_InlinePayload);
				RawData.ClearBulkDataFlags(BULKDATA_ForceInlinePayload);

				RawData.Serialize(Ar, this, INDEX_NONE, false);
				RawData.ClearBulkDataFlags(0xFFFFFFFF);
				RawData.SetBulkDataFlags(OldBulkDataFlags);
			}
			else
			{
#if ATOM_TRACE_SOUND_SERIALIZE
				UE_LOG(LogTemp, Warning, TEXT("Load [Editor]"));
#endif
				RawData.Serialize(Ar, this, INDEX_NONE, false);
			}
		}
		else // saving / loading - inline
		{
			if (Ar.IsSaving())
			{
				const uint32 OldBulkDataFlags = RawData.GetBulkDataFlags();

#if ATOM_TRACE_SOUND_SERIALIZE
				UE_LOG(LogTemp, Warning, TEXT("Save Inlined [Editor]"));
#endif
				RawData.SetBulkDataFlags(BULKDATA_ForceInlinePayload);
				RawData.ClearBulkDataFlags(BULKDATA_Force_NOT_InlinePayload);

				RawData.Serialize(Ar, this, INDEX_NONE, false);
				RawData.ClearBulkDataFlags(0xFFFFFFFF);
				RawData.SetBulkDataFlags(OldBulkDataFlags);
			}
			else
			{
#if ATOM_TRACE_SOUND_SERIALIZE
				UE_LOG(LogTemp, Warning, TEXT("Load Inlined [Editor]"));
#endif
				RawData.Serialize(Ar, this, INDEX_NONE, false);
			}
		}
	}

	// Loading
	/*if (!(IsTemplate() || IsRunningDedicatedServer()) && Ar.IsLoading())
	{
		if (!bCooked)
		{
			EAtomSoundWaveLoadingBehavior CurrentLoadingBehavior = GetLoadingBehavior();

			// If stream caching is enabled, here we determine if we should retain or prime this wave on load.
			if (bShouldStreamSound)
			{
				if (CurrentLoadingBehavior == EAtomSoundWaveLoadingBehavior::PrimeOnLoad
					|| CurrentLoadingBehavior == EAtomSoundWaveLoadingBehavior::RetainOnLoad)
				{
					RawData.Serialize(Ar, this, INDEX_NONE, false);
				}
			}
		}
		else
		{
			RawData.Serialize(Ar, this, INDEX_NONE, false);
		}
	}*/
}

#if WITH_EDITOR
void UAtomSoundBank::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UAtomSoundBank, LoadingBehavior))
	{
		// Update and cache new loading behavior if it has changed. This
		// must be called before a new FSoundWaveProxy is created. 
		CachedSoundWaveLoadingBehavior = EAtomSoundWaveLoadingBehavior::Uninitialized;
		CacheInheritedLoadingBehavior();

		if (GCriWare)
		{
			if (auto RuntimeManager = GCriWare->GetAtomRuntimeManager())
			{
				RuntimeManager->StopSoundsUsingResource(this);
				RuntimeManager->FreeResource(this);
			}
		}

		// reset local handle
		AtomResource.Reset();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UAtomSoundBank::PostImport()
{
	//if (SoundWaveDataPtr)
	//{
	//	SoundWaveDataPtr->InitializeDataFromSoundWave(*this);
	//}

	CachedSoundWaveLoadingBehavior = EAtomSoundWaveLoadingBehavior::Uninitialized;
	CacheInheritedLoadingBehavior();
}
#endif // WITH_EDITOR

void UAtomSoundBank::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
    PRAGMA_DISABLE_DEPRECATION_WARNINGS;
    Super::GetAssetRegistryTags(OutTags);
    PRAGMA_ENABLE_DEPRECATION_WARNINGS;
#else
    Super::GetAssetRegistryTags(OutTags);
    
    OutTags.Add(FAssetRegistryTag(TEXT("Sounds"), FString::FromInt(GetNumSounds()), FAssetRegistryTag::TT_Numerical));
#endif
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
void UAtomSoundBank::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
    Super::GetAssetRegistryTags(Context);
    
	Context.AddTag(FAssetRegistryTag(TEXT("Sounds"), FString::FromInt(GetNumSounds()), FAssetRegistryTag::TT_Numerical));
}
#endif

bool UAtomSoundBank::IsStreaming(const TCHAR* PlatformName /* = nullptr */) const
{
	const bool bIsForceInline = LoadingBehavior == EAtomSoundWaveLoadingBehavior::ForceInline;
	const bool bIsForcedNonStreamingInEditor = (GIsEditor && ForceAtomNonStreamingInEditorCVar != 0);

	const bool bIsForcedNonStreaming = (bIsForceInline || bIsForcedNonStreamingInEditor);

	if (bIsForcedNonStreaming)
	{
		*bIsStreamingProxyFlag = false; // updates shared flag
	}
	else
	{
		*bIsStreamingProxyFlag = IsStreaming(*FCriWarePlatformUtils::GetCookOverrides(PlatformName));
	}

	return *bIsStreamingProxyFlag;
}

bool UAtomSoundBank::IsStreaming(const FCriWareCookOverridesSettings& Overrides) const
{
	// We stream if (A) bStreaming is set to true, (B) bForceInline is false and either bUseLoadOnDemand was set to true in
	// our cook overrides, or the AutoStreamingThreshold was set and this sound is longer than the auto streaming threshold.
	const bool bIsForceInline = LoadingBehavior == EAtomSoundWaveLoadingBehavior::ForceInline;
	const bool bIsForcedNonStreamingInEditor = (GIsEditor && ForceAtomNonStreamingInEditorCVar != 0);

	const bool bIsForcedNonStreaming = (bIsForceInline || bIsForcedNonStreamingInEditor);

	if (bIsForcedNonStreaming)
	{
		*bIsStreamingProxyFlag = false;
	}
	else
	{
		*bIsStreamingProxyFlag = true;
	}

	return *bIsStreamingProxyFlag;
}

EAtomSoundWaveLoadingBehavior UAtomSoundBank::GetLoadingBehavior() const
{
	checkf(CachedSoundWaveLoadingBehavior != EAtomSoundWaveLoadingBehavior::Uninitialized
		, TEXT("Calling GetLoadingBehavior() is only valid if CacheInheritedLoadingBehavior has already been called on the game thread. (SoundBank: %s)")
		, *GetFullName());

	if ((LoadingBehavior != EAtomSoundWaveLoadingBehavior::Inherited && !*bLoadingBehaviorOverriddenPtr))
	{
		// If this sound wave specifies it's own loading behavior, use that.
		return LoadingBehavior;
	}
	else if (*bLoadingBehaviorOverriddenPtr)
	{
		// If this sound wave has already had it's loading behavior cached from soundclasses or soundcues, use that.
		return CachedSoundWaveLoadingBehavior;
	}
	else
	{
		// Otherwise, use the loading behavior defined by our cvar.
		ensureAlwaysMsgf(AtomSoundBankDefaultLoadingBehaviorCVar >= 0 && AtomSoundBankDefaultLoadingBehaviorCVar < 4, TEXT("Invalid default loading behavior CVar value. Use value 0, 1, 2 or 3."));
		return (EAtomSoundWaveLoadingBehavior)FMath::Clamp<int32>(AtomSoundBankDefaultLoadingBehaviorCVar, 0, (int32)EAtomSoundWaveLoadingBehavior::LoadOnDemand);
	}
}

void UAtomSoundBank::CacheInheritedLoadingBehavior() const
{
	check(IsInGameThread());

	// Determine this sound wave's loading behavior and cache it.
	if (LoadingBehavior != EAtomSoundWaveLoadingBehavior::Inherited)
	{
		// If this sound wave specifies it's own loading behavior, use that.
		if (CachedSoundWaveLoadingBehavior == EAtomSoundWaveLoadingBehavior::Uninitialized)
		{
			CachedSoundWaveLoadingBehavior = LoadingBehavior;
		}
	}
	else if (*bLoadingBehaviorOverriddenPtr)
	{
		ensureMsgf(CachedSoundWaveLoadingBehavior != EAtomSoundWaveLoadingBehavior::Inherited, TEXT("Sound set loading behavior to Inherited on SoundBank: %s"), *GetFullName());
	}
	else
	{
		// if this is true then the behavior should not be Inherited here
		check(!*bLoadingBehaviorOverriddenPtr);

		//USoundClass* CurrentSoundClass = GetSoundClass();
		EAtomSoundWaveLoadingBehavior SoundClassLoadingBehavior = EAtomSoundWaveLoadingBehavior::Inherited;

		// Recurse through this sound class's parents until we find an override.
		/*while (SoundClassLoadingBehavior == ESoundWaveLoadingBehavior::Inherited && CurrentSoundClass != nullptr)
		{
			SoundClassLoadingBehavior = CurrentSoundClass->Properties.LoadingBehavior;
			CurrentSoundClass = CurrentSoundClass->ParentClass;
		}*/

		// If we could not find an override in the sound class hierarchy, use the loading behavior defined by our cvar.
		//if (SoundClassLoadingBehavior == ESoundWaveLoadingBehavior::Inherited)
		{
			// query the default loading behavior CVar
			ensureAlwaysMsgf(AtomSoundBankDefaultLoadingBehaviorCVar >= 0 && AtomSoundBankDefaultLoadingBehaviorCVar < 4, TEXT("Invalid default loading behavior CVar value. Use value 0, 1, 2 or 3."));
			EAtomSoundWaveLoadingBehavior DefaultLoadingBehavior = (EAtomSoundWaveLoadingBehavior)FMath::Clamp<int32>(AtomSoundBankDefaultLoadingBehaviorCVar, 0, (int32)EAtomSoundWaveLoadingBehavior::LoadOnDemand);

			// override this loading behavior w/ our default
			SoundClassLoadingBehavior = DefaultLoadingBehavior;
			*bLoadingBehaviorOverriddenPtr = true;
		}

		CachedSoundWaveLoadingBehavior = SoundClassLoadingBehavior;
	}
}

void UAtomSoundBank::RetainResource()
{
	if (GCriWare)
	{
		if (auto RuntimeManager = GCriWare->GetCriWareCore()->GetAtomRuntimeManager())
		{
			auto OnLoadCompleted = [WeakThis = MakeWeakObjectPtr(this)](EAtomResourceLoadResult LoadResult)
			{
				if (WeakThis.IsValid() && (LoadResult == EAtomResourceLoadResult::Completed || LoadResult == EAtomResourceLoadResult::AlreadyLoaded))
				{
					UAtomSoundBank* ThisSoundBank = WeakThis.Get();
					FAtomResourceHandle& ResourceHandle = ThisSoundBank->AtomResource;

					// If we hit this, something went wrong in RequestSoundResource.
					if (!ResourceHandle.IsValid())
					{
						UE_LOG(LogCriWareAtom, Display, TEXT("Failed to retrieve cri atom handle from AtomSoundBank %s after successfully requesting it!"), *(WeakThis->GetName()));
					}
				}
			};

			FAtomLoadParams LoadParams;
			LoadParams.Source = this;
			LoadParams.Scope = EAtomResourceScope::Shared; // this resource is shared over multiple objects
			LoadParams.bAsyncLoad = true;
			LoadParams.bIsStreamed = IsStreaming(nullptr);
			LoadParams.OnLoadCompleted = OnLoadCompleted;
			LoadParams.ThreadToCallOnLoadCompletedOn = ENamedThreads::GameThread;
			AtomResource = RuntimeManager->GetAtomResourceManager().RequestSoundResource(LoadParams);
		}
	}
}

void UAtomSoundBank::ReleaseResource()
{
	if (AtomResource.IsValid())
	{
		if (GCriWare)
		{
			if (auto RuntimeManager = GCriWare->GetCriWareCore()->GetAtomRuntimeManager())
			{
				// stop all consumers accessing the resouece in active runtime.
				RuntimeManager->StopSoundsUsingResource(this);
			}
		}

		// force unload even if something else keep an handle (CueSheet)
		AtomResource->Unload();

		// reset local handle
		AtomResource.Reset();
	}
}

bool UAtomSoundBank::IsRetainingResource() const
{
	return AtomResource.IsValid();
}

FByteBulkData* UAtomSoundBank::GetRawData()
{
	if (IsTemplate() || IsRunningDedicatedServer())
	{
		return nullptr;
	}

	return RawData.GetBulkDataSize() > 0 ? &RawData : nullptr;
}

#undef LOCTEXT_NAMESPACE
