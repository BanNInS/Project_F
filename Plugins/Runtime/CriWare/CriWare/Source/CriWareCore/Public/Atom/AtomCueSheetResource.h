/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomCueSheetResource.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "AtomResource.h"
#include "AtomCueSheet.h"

// Forward Declarations
struct FAtomCueInfo;

/*
 * Atom resource for Atom cue sheet.
 */
class CRIWARECORE_API FAtomCueSheetResource
	: public FAtomStreamableResource<FCriAtomExAcbPtr>
{
public:
	FAtomCueSheetResource(UAtomCueSheet* InSource)
		: FAtomStreamableResource<FCriAtomExAcbPtr>(InSource)
	{}

	virtual FAtomCueSheetResource* GetCueSheetResource() override { return this; }
	virtual void IterateOverSoundResourceDependencies(const FIterateDependenciesCallback& Callback) const override;

	virtual bool AsyncLoad(TFunction<void(EAtomResourceLoadResult)> OnLoadCompleted, ENamedThreads::Type CallbackThread, EAtomResourceScope Scope) override;
	virtual void Unload() override;
	virtual void Teardown() override;
	virtual bool IsLoaded() const override;

	/** Retrieves the number of cues in this cue sheet resource. */
	int32 GetNumCues() const;
	
	/** Retrieves cue information by its index in this cue sheet resource. */
	bool GetCueInfoByIndex(int32 CueIndex, FAtomCueInfo& OutCueInfo) const;

	/** 
	 * Retrieves first wave information of a cue.
	 * By default this function reads wave info from cue data.
	 * Use flags to try to read data from attached wave banks.
	 * 
	 * @param bReadFromWaveBank  When streamed sound, tries to read data from attached wave bank else use cue info.
	 * @param bForceFromWaveBank  Implies bReadFromWaveBank set to true. When streamed sound, tries to read data from attached wave bank else return false.
	 * 
	 * @return if the wave information was retreived.  
	 */
	bool GetWaveformInfo(int32 CueID, FAtomWaveInfo& OutWaveInfo, bool bReadFromWaveBank = false, bool bForceFromWaveBank = false);

	/** Retrieves the number of slot to native wave bank used by this cue sheet resource. */
	uint16 GetNumWaveBankSlots() const;

	/** Retrieves the name of slot to native wave bank used by this cue sheet resource.*/
	FString GetWaveBankSlotName(uint16 Index) const;

	bool AttachWaveBank(IAtomSoundResource* AtomWaveBankSource, uint16 Index, bool bCanReplace = false, bool bAsyncLoad = false);

	bool DetachWaveBank(uint16 Index);

	bool IsWavedBankLoaded(uint16 Index);

	/** Retruns the native wave bank handle pointer used by this cue sheet to access to other native functions. (Use this with care) */
	CriAtomAwbHn GetNativeAwbHandle(int16 Index = INDEX_NONE);

	/** Retruns the native handle pointer to access to other native functions. (Use this with care) */
	CriAtomExAcbHn GetNativeHandle() const;

protected:

	bool AttachWaveBankInternal(IAtomSoundResource* WaveBankSource, uint16 Index, bool bCanReplace, bool bAsyncLoad);

	TMap<uint16, FAtomResourceHandle> AttachedAwbResources;

	FCriticalSection SDKCriticalSection;
};
