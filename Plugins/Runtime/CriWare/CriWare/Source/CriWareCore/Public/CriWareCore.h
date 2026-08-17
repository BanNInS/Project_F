/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWareCore.h
 *
 ****************************************************************************/

#pragma once

#include "UObject/Object.h"
#include "Containers/Map.h"
#include "CriWarePlatform.h"
#include "CriWareDefines.h"
#include "Atom/AtomRuntimeManager.h"

class FAtomRuntime;
struct FCriWareAllocator;

/**
 * Main class for CriWare.
 * Responsible of libraries initialization and runtime control.
 */
class FCriWareCore
	: public TSharedFromThis<FCriWareCore>
{
public:
	FCriWareCore();
	~FCriWareCore();

	/** Initializes CriWare Libraries and sub-modules following plugin settings. */
	CRIWARECORE_API void InitalizeLibraries();

	/** Uninitializes CriWare Libraries and sub-modules following plugin settings. */
	CRIWARECORE_API void UninitializeLibraries();

	/** Convinient function to setup filesystem global settings. */
	CRIWARECORE_API void SetupFileSystem(FCriWareAllocator& FsAllocator);

	/** Check if initialized. */
	FORCEINLINE bool IsInitialized() const { return bIsInitialized; }

	/** Returns The Atom runtime manager of the CriWare module, this allows the creation and management of multiple Atom runtimes. */
	CRIWARECORE_API FAtomRuntimeManager* GetAtomRuntimeManager() const;

	/** Returns the main Atom runtime ID used by the CriWare module. */
	CRIWARECORE_API uint32 GetMainAtomRuntimeID() const;

	/** Returns the main Atom runtime used by the CriWare module. */
	CRIWARECORE_API FAtomRuntimeHandle GetMainAtomRuntime() const;
	CRIWARECORE_API FAtomRuntime* GetMainAtomRuntimeRaw() const;

	/** Returns the currently active Atom runtime */
	CRIWARECORE_API FAtomRuntimeHandle GetActiveAtomRuntime() const;

	/** On End Frame. (FCriWareCore Tick) */
	void HandleOnEndFrame();

	/** On Application Will Deactivate. */
	void HandleApplicationWillDeactivate();

	/** On Application Has Reactivated. */
	void HandleApplicationHasReactivated();

#if WITH_EDITOR
	/** Deals with anything CriWare related that should happen when PIE starts */
	CRIWARECORE_API void OnBeginPIE(bool bIsSimulating);

	/** Deals with anything CriWare related that should happen when PIE will ends */
	CRIWARECORE_API void OnPrePIEEnded(bool bIsSimulating);

	/** Deals with anything CriWare related that should happen when PIE ends */
	CRIWARECORE_API void OnEndPIE(bool bIsSimulating);
#endif

private:
	/** The Atom runtime manager */
	FAtomRuntimeManager* AtomRuntimeManager = nullptr;

	/** Atom runtime handle to the main Atom runtime. */
	FAtomRuntimeHandle MainAtomRuntimeHandle;

#if WITH_EDITOR
	/** Atom runtime handle to the runtime used when Play in Editor. */
	FAtomRuntimeHandle PIEAtomRuntimeHandle;

	FDelegateHandle PIEAtomRuntimeDestroyedHandle;
#endif

	bool bIsInitialized;

	void InitializeAtomRuntimeManager();
	void ReleaseAtomRuntimeManager();
	void UpdateAtomListeners(UWorld* World);

	//~ static interface
public:

	FORCEINLINE static FCriWarePlatform& GetPlatform() { return Platform; }

private:

	CRIWARECORE_API static FCriWarePlatform Platform;

#ifdef CRIWARE_UE_LE
	inline static void* DllHandle = nullptr;
#endif
};

// Globals

extern CRIWARECORE_API bool IsAtomThreadRunning();

/** @return True if called from the atom thread, and not merely a thread calling atom functions. */
extern CRIWARECORE_API bool IsInAtomThread();
