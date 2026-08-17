/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomWaveBankResource.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "AtomResource.h"
#include "AtomWaveBank.h"

// Forward Declarations
//class FAtomCueSheetResource;

/*
 * Atom resource for Atom wave bank.
 */
class CRIWARECORE_API FAtomWaveBankResource
	: public FAtomStreamableResource<FCriAtomAwbPtr>
{
public:

	FAtomWaveBankResource(UAtomWaveBank* InSource)
		: FAtomStreamableResource<FCriAtomAwbPtr>(InSource)
	{}

	virtual FAtomWaveBankResource* GetWaveBankResource() override { return this; }

	virtual bool AsyncLoad(TFunction<void(EAtomResourceLoadResult)> OnLoadCompleted, ENamedThreads::Type CallbackThread, EAtomResourceScope Scope) override;
	virtual void Unload() override;
	virtual void Teardown() override;

	/** Retrieves the number of waves in this wave bank resource. */
	int32 GetNumWaves() const;

	/** Retrieves wave identifier by its index in this wave bank resource. */
	bool GetWaveIDByIndex(int32 WaveIndex, int32& OutWaveID) const;

	/** Retrieves waveform information of a wave. */
	bool GetWaveformInfo(int32 WaveID, FAtomWaveInfo& OutWaveInfo) const;

	/** Retruns the native handle pointer to access to other native functions. (Use this with care) */
	CriAtomAwbHn GetNativeHandle() const;

private:

	static bool GetWaveformInfoInternal(CriAtomAwbHn AwbHandle, CriFsBinderHn FsBinderHandle, int32 WaveID, FAtomWaveInfo& OutWaveInfo);

	friend FAtomCueSheetResource;
};
