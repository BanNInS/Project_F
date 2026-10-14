/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomCueSheetResource.cpp
 *
 ****************************************************************************/

#include "Atom/AtomCueSheetResource.h"

#include "CriWare.h"
#include "CriWareMemory.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomCueSheet.h"
#include "Atom/AtomWaveBank.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomWaveBankResource.h"

#define ATOM_TRACE_RESOURCES 0

void FAtomCueSheetResource::IterateOverSoundResourceDependencies(const FIterateDependenciesCallback& Callback) const
{
	UAtomCueSheet* CueSheet = Cast<UAtomCueSheet>(Source);
	check(CueSheet);
	for (auto& WaveBank : CueSheet->WaveBanks)
	{
		Callback(WaveBank);
	}
}

bool FAtomCueSheetResource::AsyncLoad(TFunction<void(EAtomResourceLoadResult)> OnLoadCompleted, ENamedThreads::Type CallbackThread, EAtomResourceScope Scope)
{
	// todo: replace with source proxy 
	UAtomCueSheet* CueSheet = Cast<UAtomCueSheet>(Source);
	check(CueSheet);

	// add callaback to the thread safe multicast event fired from the loading thread
	if (OnLoadCompleted)
	{
		OnLoadCompletedEvent.Add(OnLoadCompleted, CallbackThread);
	}

	// check resource load status
	if (OnLoadCompletedEvent.WasBroadcasted())
	{
		OnLoadCompletedEvent.Broadcast(EAtomResourceLoadResult::AlreadyLoaded);
		return false;
	}

	if (IsAsyncLoading())
	{
		return false;
	}

	// some awb are not loaded - load them
	if (FAtomStreamableResource::IsLoaded())
	{
		LoadTask = new FLoadTask([this, CueSheet, Scope](bool& bShutdown)
		{
			for (int32 Index = 0; Index < CueSheet->WaveBanks.Num(); Index++)
			{
				if (UAtomWaveBank* WaveBank = CueSheet->WaveBanks[Index])
				{
					if (const FAtomResourceHandle* AwbHandlePtr = AttachedAwbResources.Find(Index))
					{
						if (FAtomResource* Resource = AwbHandlePtr->GetResource())
						{
							if (!Resource->IsLoaded())
							{
								AttachWaveBankInternal(WaveBank, (uint16)Index, true, false); // re-attach, sync load
							}
						}
					}
				}
			}

			OnLoadCompletedEvent.Broadcast(EAtomResourceLoadResult::Completed);
		});

		return true;
	}

	// access to bulk data of the source
	auto RawData = CueSheet->GetRawData();
	if (!RawData)
	{
		OnLoadCompletedEvent.Broadcast(EAtomResourceLoadResult::Interrupted);
		return false;
	};

	// check if streaming is asked
	bool bRequestStreamed = CueSheet->IsStreaming();

	// check if we can stream from file or use inlined memory
	bool bUseInlined = !RawData->DoesExist() || RawData->IsInlined() || !bRequestStreamed;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	bool bUseIODispatcher = true; // IoDispatcher requests can handle AsyncLoad2 from Unreal 5.2
#else
	bool bUseIODispatcher = RawData->IsUsingIODispatcher();
#endif

	bool bUseAutomaticFs = GetDefault<UCriWareCoreSettings>()->bUseAutomaticFileSystemManagement;

	//bool bIsInSeparateFile = RawData->IsInSeparateFile();
	//bool bIsInExternalResource = RawData->IsInExternalResource();
	//bool bCanBeLoaded = RawData->CanLoadFromDisk();
	//bool bUseMemoryMapped = RawData->IsDataMemoryMapped();
	//UE_LOG(LogTemp, Warning, TEXT("AsyncLoad CS: SepFile %d ExtRes %d MMap %d OnDisk %d IODisp %d Inline %d"), bIsInSeparateFile, bIsInExternalResource, bUseMemoryMapped, bCanBeLoaded, bUseIODispatcher, bUseInlined);

	FString FilePath;
	int64 Offset;
	int64 Size;
	if (!bUseInlined)
	{
		if (bUseIODispatcher)
		{
			FilePath = FString::Printf(TEXT("@%d"), ResourceID);
			Offset = RawData->GetBulkDataOffsetInFile();
			Size = RawData->GetBulkDataSize();
		}
		else
		{
			//FilePath = RawData->GetPackagePath().GetLocalFullPath(RawData->GetPackageSegment());
			//Offset = RawData->GetBulkDataOffsetInFile();
			FilePath = FString::Printf(TEXT("%%%d"), ResourceID);
			Offset = 0;
			Size = RawData->GetBulkDataSize();
		}
	}
	else // inlined 
	{
		Offset = 0;
		Size = RawData->GetBulkDataSize();
	}

	// call to file loading  - criAtomExAcb_LoadAcbData is a synchronous function
	LoadTask = new FLoadTask([this, CueSheet, bUseInlined, bUseAutomaticFs, Offset, Size, FilePath, Scope](bool& bShutdown)
	{
		if (CueSheet->Validate() && !CueSheet->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
		{
			if (!bUseInlined)
			{
				static const CriChar8 SectionName[] = "ACB";

				CriFsBinderHn FsBinderAcbHn = nullptr;
				
				if (!bUseAutomaticFs)
				{
					FCriWareApi::criFsBinder_Create(&FsBinderAcbHn);
				}
				else
				{
					CriSint32 WorkSize;
					FCriWareApi::criFsBinder_GetWorkSize(&WorkSize);

					SetFsBinderWork(WorkSize);
					if (FsBinderWork == nullptr)
					{
						OnLoadCompletedEvent.Broadcast(EAtomResourceLoadResult::Interrupted);
						return;
					}
					FCriWareApi::criFsBinder_CreateWithWork(&FsBinderAcbHn, FsBinderWork, WorkSize);
				}
				FCriFsBinderPtr FsBinderAcb = MakeCriHandle(FsBinderAcbHn);

				CriFsBinderId FsBinderAcbID = -1;
				FCriWareApi::criFsBinder_BindFileSection(FsBinderAcb, nullptr, TCHAR_TO_UTF8(*FilePath), Offset, Size, SectionName, nullptr, 0, &FsBinderAcbID);

				CriFsBinderStatus FsBinderStatus = CRIFSBINDER_STATUS_NONE;
				while (FsBinderStatus != CRIFSBINDER_STATUS_COMPLETE
					&& FsBinderStatus != CRIFSBINDER_STATUS_ERROR)
				{
					FCriWareApi::criFsBinder_GetStatus(FsBinderAcbID, &FsBinderStatus);
				}

				FCriAtomExAcbPtr ExAcb;
				if (FsBinderStatus == CRIFSBINDER_STATUS_COMPLETE)
				{
					ExAcb = MakeCriHandle(FCriWareApi::criAtomExAcb_LoadAcbFile(FsBinderAcb, SectionName, nullptr, nullptr, nullptr, 0));
				}

				if (!ExAcb.Get())
				{
					// Failed
					FsBinderAcb.Reset();

					OnLoadCompletedEvent.Broadcast(EAtomResourceLoadResult::Completed);
					return;
				}

				SetNativeHandle(ExAcb);
				SetFsHandle(FsBinderAcb);
			}
			else
			{
				FCriAtomExAcbPtr ExAcb;

				// check if data is already loaded (if used by another runtime or in another scope)
				// if not, we load the data to memory.
				const uint8* ResourceData = Source->GetResourceData();
				bool bInitResource = false;
				if (!ResourceData && Source->InitAtomResource())
				{
					ResourceData = Source->GetResourceData();
					bInitResource = true;
				}

#if WITH_EDITOR // actually atom need a writable memory so we have to ensure an extra copy even in Game.
				// test try to get an unique data for unique resource (atom cannot have concurency acces to same memory)
				if (Scope == EAtomResourceScope::Unique)
#endif
				{
					auto RawData = CueSheet->GetRawData();

					check(!LocalData);
					RawData->GetCopy((void**)&LocalData, true);
					ResourceData = LocalData;
				}

				if (ResourceData)
				{
					ExAcb = MakeCriHandle(FCriWareApi::criAtomExAcb_LoadAcbData((void*)ResourceData, Size, nullptr, nullptr, nullptr, 0));
				}

				if (!ExAcb.Get())
				{
					// Failed

					// release only if we loaded it
					if (bInitResource)
					{
						Source->RemoveAtomResource();
					}

					OnLoadCompletedEvent.Broadcast(EAtomResourceLoadResult::Completed);
					return;
				}

				SetNativeHandle(ExAcb);
			}

			// get wave banks and try to load if possible
			for (int32 Index = 0; Index < CueSheet->WaveBanks.Num(); Index++)
			{
				UAtomWaveBank* WaveBank = CueSheet->WaveBanks[Index];
				AttachWaveBankInternal(WaveBank, (uint16)Index, false, false);
			}
		}

		OnLoadCompletedEvent.Broadcast(EAtomResourceLoadResult::Completed);
	});

	return true;
}

void FAtomCueSheetResource::Unload()
{
	// Wait for loading to finalize. 
	if (IsAsyncLoading())
	{
		MakeSureIsLoaded(EAtomResourceScope::Shared); // scope ignored
	}
	
	// Unload runtime resource.
	if (FAtomStreamableResource::IsLoaded())
	{
		CriResource.Reset();
	}
	FsBinder.Reset();
	ReleaseFsBinderWork();
	bIsStreamed = false;

	// Destroy attached wave bank binders. 
	AttachedAwbResources.Reset();

	// Remove the task.
	if (LoadTask)
	{
		delete LoadTask;
		LoadTask = nullptr;
	}

	if (LocalData)
	{
		FMemory::Free(LocalData);
		LocalData = nullptr;
	}

	OnLoadCompletedEvent.Reset();
}

void FAtomCueSheetResource::Teardown()
{
	// unload if loaded and release natives handles.
	Unload();

	// Unload underlying data resource 
	// - Release Only if last handle destroyed (or external call).
	// - Release only if not used by any other resource.
	bool bIsSourceUsedByAnotherResource = false;
	if (GCriWare)
	{
		if (auto ResourceManager = GCriWare->GetAtomResourceManager())
		{
			ResourceManager->IterateOverAllResources([this, &bIsSourceUsedByAnotherResource](FAtomResourceId ID, FAtomResource* Resource)
				{
					if (Resource->GetSoundResource() == Source && Resource != this)
					{
						bIsSourceUsedByAnotherResource = true;
					}
				});
		}
	}

	UObject* SourceObject = Cast<UObject>(Source);
	if (!bIsSourceUsedByAnotherResource && IsValid(SourceObject) && !SourceObject->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
	{
		Source->RemoveAtomResource();
	}
}

bool FAtomCueSheetResource::IsLoaded() const
{
	if (FAtomStreamableResource::IsLoaded())
	{
		for (auto& Pair : AttachedAwbResources)
		{
			const FAtomResourceHandle& AwbResource = Pair.Value;
			if (!AwbResource.IsValid() || !AwbResource->IsLoaded())
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

int32 FAtomCueSheetResource::GetNumCues() const
{
	if (FAtomStreamableResource::IsLoaded())
	{
		return FCriWareApi::criAtomExAcb_GetNumCues(CriResource);
	}

	return 0;
}

static void LoadCueInfo(const FCriAtomExAcbPtr& ExAcbResource, const CriAtomExCueInfo& AtomExCueInfo, FAtomCueInfo& CueInfo, float DistanceFactorToApply)
{
	if (!ExAcbResource)
	{
		return;
	}

	CueInfo.ID = static_cast<int32>(AtomExCueInfo.id);
	CueInfo.Name = FString(UTF8_TO_TCHAR(AtomExCueInfo.name));
	CueInfo.Type = (EAtomCueType)AtomExCueInfo.type;
	CueInfo.bHeaderVisibility = (AtomExCueInfo.header_visibility == 1);
	CueInfo.bIgnorePlayerParameters = (AtomExCueInfo.ignore_player_parameter == 1);

	// Duration (ms -> ticks)
	CueInfo.Duration = AtomExCueInfo.length >= 0 ? FTimespan(AtomExCueInfo.length * 10000UL) : FTimespan::FromSeconds(INDEFINITELY_LOOPING_DURATION);

	// Volume and pitch
	CueInfo.Volume = Atom::ConvertToDecibels(AtomExCueInfo.volume);
	//CueInfo.Pitch = AtomExCueInfo.pitch;
	
	// Loop
	CueInfo.bLooping = AtomExCueInfo.length == -1;

	// Virtualization
	switch (AtomExCueInfo.silent_mode)
	{
	default:
	case CRIATOMEX_SILENT_MODE_NORMAL: CueInfo.VirtualizationMode = EAtomVirtualizationMode::Normal; break;
	case CRIATOMEX_SILENT_MODE_STOP: CueInfo.VirtualizationMode = EAtomVirtualizationMode::StopWhenSilent; break;
	case CRIATOMEX_SILENT_MODE_VIRTUAL: CueInfo.VirtualizationMode = EAtomVirtualizationMode::PlayWhenSilent; break;
	case CRIATOMEX_SILENT_MODE_VIRTUAL_RETRIGGER: CueInfo.VirtualizationMode = EAtomVirtualizationMode::Retrigger; break;
	}

	// Priority
	// convert to float range because Atom sdk use a greater range than the per-cue range priority for scaling.
	CueInfo.Priority = AtomExCueInfo.priority;

	// Sequence Metrics
	CueInfo.CueLimit = AtomExCueInfo.num_limits;
	CueInfo.NumBlocks = AtomExCueInfo.num_blocks;
	CueInfo.NumTracks = AtomExCueInfo.num_tracks;
	CueInfo.NumRelatedWaveforms = AtomExCueInfo.num_related_waveforms;

	// GameVariable if used
	CueInfo.GameVariable = (CueInfo.Type == EAtomCueType::SwitchGameVariable)
				? FName(UTF8_TO_TCHAR(AtomExCueInfo.game_variable_info.name))
				: NAME_None;

	// UserData - will be an independnat function 
	//if (AtomExCueInfo.user_data != nullptr) {
	//	CueInfo.UserData = FString(UTF8_TO_TCHAR(AtomExCueInfo.user_data));
	//}

	// CategoryName list
	CueInfo.CategoryNames.Reset();
	for (size_t Index = 0; Index < CRIATOMEXCATEGORY_MAX_CATEGORIES_PER_PLAYBACK; ++Index)
	{
		CriAtomExCategoryInfo CategoryInfo;
		if (FCriWareApi::criAtomExAcf_GetCategoryInfo(AtomExCueInfo.categories[Index], &CategoryInfo) == CRI_FALSE)
		{
			break;
		}
		CueInfo.CategoryNames.Add(CategoryInfo.name);
	}

	// Spatialization
	CueInfo.SpatializationType = AtomExCueInfo.pan_type >= CRIATOMEX_PAN_TYPE_PAN3D ? (EAtomCueSpatializationType)AtomExCueInfo.pan_type : EAtomCueSpatializationType::Auto;

	// Attenuation distances
	CueInfo.AttenuationDistance.MaxDistance = AtomExCueInfo.pos3d_info.max_distance / DistanceFactorToApply;
	CueInfo.AttenuationDistance.MinDistance = AtomExCueInfo.pos3d_info.min_distance / DistanceFactorToApply;
	CueInfo.AttenuationDistance.ConeInsideAngle = AtomExCueInfo.pos3d_info.cone_inside_angle; // in degree
	CueInfo.AttenuationDistance.ConeOutsideAngle = AtomExCueInfo.pos3d_info.cone_outside_angle; // in degree
	CueInfo.AttenuationDistance.DopplerFactor = AtomExCueInfo.pos3d_info.doppler_factor;
	CueInfo.AttenuationDistance.SourceRadius = AtomExCueInfo.pos3d_info.source_radius / DistanceFactorToApply;
	CueInfo.AttenuationDistance.InteriorDistance = AtomExCueInfo.pos3d_info.interior_distance / DistanceFactorToApply;

	// AISACControlName list
	CueInfo.AisacControls.Reset();
	const CriSint32 NumControls = FCriWareApi::criAtomExAcb_GetNumUsableAisacControlsById(ExAcbResource, CueInfo.ID);
	for (CriSint32 Index = 0; Index < NumControls; ++Index)
	{
		CriAtomExAisacControlInfo ControlInfo;
		CriBool Result = FCriWareApi::criAtomExAcb_GetUsableAisacControlById(ExAcbResource, CueInfo.ID, (CriUint16)Index, &ControlInfo);
		if (Result != CRI_FALSE)
		{
			const int32 ControlID = (int32)ControlInfo.id;
			const FString ControlNameFromLoadedAcf = UTF8_TO_TCHAR(ControlInfo.name);
			const FName ControlName = ControlNameFromLoadedAcf.IsEmpty() ? FName(FString::Printf(TEXT("AISAC Control %d"), ControlID)) : FName(ControlNameFromLoadedAcf);
			CueInfo.AisacControls.Add({ ControlName, ControlID }); // saved as FName (unsafe and inaccurate obtained from current ACF) and ID (safe)
		}
	}

	// ParameterPalletAssigned
	CueInfo.bIsParameterPalletAssigned = FCriWareApi::criAtomExAcb_IsParameterPalletAssignedByName(ExAcbResource, TCHAR_TO_UTF8(*CueInfo.Name));
}

bool FAtomCueSheetResource::GetCueInfoByIndex(int32 CueIndex, FAtomCueInfo& OutCueInfo) const
{
	if (FAtomStreamableResource::IsLoaded())
	{
		CriAtomExCueInfo AtomExCueInfo = {};
		if (FCriWareApi::criAtomExAcb_GetCueInfoByIndex(CriResource, CueIndex, &AtomExCueInfo))
		{
			LoadCueInfo(CriResource, AtomExCueInfo, OutCueInfo, 0.01f/*DistanceFactor*/);
			return true;
		}
	}

	return false;
}

bool FAtomCueSheetResource::GetWaveformInfo(int32 CueID, FAtomWaveInfo& OutWaveInfo, bool bReadFromWaveBank /* = false */, bool bForceFromWaveBank /* = false */)
{
	if (FAtomStreamableResource::IsLoaded())
	{
		CriAtomExWaveformInfo NativeWaveInfo = {};

		FScopeLock Lock(&SDKCriticalSection);

		if (FCriWareApi::criAtomExAcb_GetWaveformInfoById(CriResource, static_cast<CriAtomExCueId>(CueID), &NativeWaveInfo))
		{
			if (NativeWaveInfo.num_channels <= 0)
			{
				return false;
			}

			// get wave information from underlying awb if used and requested
			if (bReadFromWaveBank && NativeWaveInfo.streaming_flag)
			{
				uint16 Index = 0;

				if (CriAtomAwbHn AwbHandle = GetNativeAwbHandle(Index))
				{
					if (FAtomResourceHandle* ResourceHandle = AttachedAwbResources.Find(Index))
					{
						if (ResourceHandle->IsValid())
						{
							if (const FCriFsBinderPtr& FsBinderPtr = ResourceHandle->GetResource()->GetWaveBankResource()->FsBinder)
							{
								bool Result = FAtomWaveBankResource::GetWaveformInfoInternal(AwbHandle, FsBinderPtr, NativeWaveInfo.wave_id, OutWaveInfo);
								if (Result)
								{
									OutWaveInfo.bIsStreamed = true;
								}

								return Result;
							}
						}
					}
				}

				// if using a wave bank but unable to retreive wave info, die
				if (bForceFromWaveBank)
				{
					return false;
				}
			}

			// or store wave information from cue wave info
			OutWaveInfo.WaveID = NativeWaveInfo.wave_id;
			OutWaveInfo.Format = FAtomRuntime::GetFormatFromAtomExFormat(NativeWaveInfo.format);
			OutWaveInfo.SampleRate = NativeWaveInfo.sampling_rate;
			OutWaveInfo.NumChannels = NativeWaveInfo.num_channels;
			OutWaveInfo.NumFrames = NativeWaveInfo.num_samples; // note: cri 'samples' are ue4 'frames'
			OutWaveInfo.bIsStreamed = NativeWaveInfo.streaming_flag;

			return true;
		}
	}

	return false;
}

uint16 FAtomCueSheetResource::GetNumWaveBankSlots() const
{
	//const CriChar8* criAtomExAcb_GetAwbFileSlotName(CriAtomExAcbHn acb_hn, CriUint16 index);
	// 
	//CriBool criAtomExAcb_IsAttachedAwbFile(CriAtomExAcbHn acb_hn, const CriChar8* awb_name);

	/*void CRIAPI criAtomExAcb_AttachAwbFile(
		CriAtomExAcbHn acb_hn,
		CriFsBinderHn awb_binder,
		const CriChar8 * awb_path,
		const CriChar8 * awb_name,
		void* work,
		CriSint32 work_size);*/

	/*void CRIAPI criAtomExAcb_DetachAwbFile(
		CriAtomExAcbHn acb_hn,
		const CriChar8 * awb_name);*/

	if (IsLoaded())
	{
		return (int32)FCriWareApi::criAtomExAcb_GetNumAwbFileSlots(CriResource);
	}

	return 0;
}

FString FAtomCueSheetResource::GetWaveBankSlotName(uint16 Index) const
{
	return FString(UTF8_TO_TCHAR(FCriWareApi::criAtomExAcb_GetAwbFileSlotName(CriResource, (CriUint16)Index)));
}

bool FAtomCueSheetResource::AttachWaveBank(IAtomSoundResource* AtomWaveBankSource, uint16 Index, bool bCanReplace /* = false */, bool bAsyncLoad /* = false */)
{
	if (FAtomStreamableResource::IsLoaded())
	{
		return AttachWaveBankInternal(AtomWaveBankSource, Index, bCanReplace, bAsyncLoad);
	}

	return false;
}

bool FAtomCueSheetResource::AttachWaveBankInternal(IAtomSoundResource* WaveBankSource, uint16 Index, bool bCanReplace, bool bAsyncLoad)
{
	if (!WaveBankSource)
	{
		return false;
	}

	const CriChar8* SlotName = FCriWareApi::criAtomExAcb_GetAwbFileSlotName(CriResource, (CriUint16)Index);

	// check if exist
	if (FCriWareApi::criAtomExAcb_IsAttachedAwbFile(CriResource, SlotName) == CRI_TRUE)
	{
		if (!bCanReplace)
		{
			UE_LOG(LogCriWareAtom, Warning, TEXT("Attach a WaveBank to CueSheet failed: WaveBank is already attached."));
			return false;
		}

		// detach
		FCriWareApi::criAtomExAcb_DetachAwbFile(CriResource, SlotName);
	}

	// get awb data file info
	UAtomWaveBank* WaveBank = Cast<UAtomWaveBank>(WaveBankSource);
	check(WaveBank);

	if (GCriWare)
	{
		if (auto ResourceManager = GCriWare->GetAtomResourceManager())
		{
			FAtomLoadParams LoadParams;
			LoadParams.Source = WaveBank;
			LoadParams.Scope = EAtomResourceScope::Shared; // this resource is shared over multiple objects
			LoadParams.bAsyncLoad = bAsyncLoad;
			LoadParams.bIsStreamed = true; // Note: in the case it is already loaded and inlined the resorce will be inlined.
			FAtomResourceHandle AwbResource = ResourceManager->RequestSoundResource(LoadParams);

			FCriFsBinderPtr& FsBinderAwb = AwbResource->GetWaveBankResource()->FsBinder;
			{
#if ATOM_TRACE_RESOURCES
				UE_LOG(LogCriWareAtomDebug, Display, TEXT("Attach WaveBank %s to Slot #%d of CueSheet %s."), *Cast<UObject>(WaveBankSource)->GetName(), Index, *Cast<UObject>(Source)->GetName());
#endif
				// attach
				if (FsBinderAwb.IsValid() && Index > 0)
				{
					// do not support inlined now (no API)
					FCriWareApi::criAtomExAcb_AttachAwbFile(CriResource, FsBinderAwb, "AWB", SlotName, nullptr, 0);
				}
				else
				{
					// for inlined or not awb-hn for index 0 can uses the handle directly
					FCriWareApi::criAtomExAcb_AttachAwbHn(CriResource, AwbResource->GetWaveBankResource()->CriResource);
				}

				// add to index map
				AttachedAwbResources.Emplace(Index, AwbResource);

				return true;
			}
		}
	}

	return false;
}

bool FAtomCueSheetResource::DetachWaveBank(uint16 Index)
{
	if (FAtomStreamableResource::IsLoaded())
	{
		const CriChar8* SlotName = FCriWareApi::criAtomExAcb_GetAwbFileSlotName(CriResource, (CriUint16)Index);

		// check if exist
		if (FCriWareApi::criAtomExAcb_IsAttachedAwbFile(CriResource, SlotName) == CRI_FALSE)
		{
			return false;
		}

		UE_LOG(LogCriWareAtom, Warning, TEXT("Detach WaveBank to Slot #%d of CueSheet %s"), Index, *Cast<UObject>(Source)->GetName());

		// detach
		FCriWareApi::criAtomExAcb_DetachAwbFile(CriResource, SlotName);

		AttachedAwbResources.Remove(Index);

		return true;
	}

	return false;
}

bool FAtomCueSheetResource::IsWavedBankLoaded(uint16 Index)
{
	if (FAtomStreamableResource::IsLoaded())
	{
		if (const FAtomResourceHandle* AwbHandlePtr = AttachedAwbResources.Find(Index))
		{
			if (FAtomResource* Resource = AwbHandlePtr->GetResource())
			{
				return Resource->IsLoaded();
			}
		}
	}

	return false;
}

CriAtomAwbHn FAtomCueSheetResource::GetNativeAwbHandle(int16 Index)
{
	if (FAtomStreamableResource::IsLoaded())
	{
		// Get AWB handle (external wave data)
		if (Index < 0 || AttachedAwbResources.Num() == 0) // inlined with AcbData
		{
			return FCriWareApi::criAtomExAcb_GetOnMemoryAwbHandle(CriResource);
		}
		else // from an external awb data 
		{
			if (const FAtomResourceHandle* AwbHandlePtr = AttachedAwbResources.Find(Index))
			{
				if (FAtomResource* Resource = AwbHandlePtr->GetResource())
				{
					if (FAtomWaveBankResource* WaveBankResource = Resource->GetWaveBankResource())
					{
						return WaveBankResource->GetNativeHandle();
					}
				}
			}
		}
	}

	return (CriAtomAwbHn)nullptr;
}

CriAtomExAcbHn FAtomCueSheetResource::GetNativeHandle() const
{
	return CriResource.Get();
}
