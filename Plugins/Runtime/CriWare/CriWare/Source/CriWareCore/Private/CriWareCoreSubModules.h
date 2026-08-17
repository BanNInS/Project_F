/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2020-2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : Core's sub modules listener.
 * File     : CriWareCoreSubModules.h
 *
 ****************************************************************************/

#pragma once

#include "Containers/Array.h"

/** Interface for CriWareCore sub modules and expansions. */
class ICriWareCoreSubModule
{
protected:
	virtual ~ICriWareCoreSubModule() {}

public:
	/** Override this to initialize stuff before criware runtime initializes Cri native libraries. */
	virtual void PreInitializeLibraries() {}

	/** Override this to initialize stuff after criware runtime initialized Cri native libraries. */
	virtual void PostInitializeLibraries() {}

	/** Override this to finalize stuff before criware runtime finalizes Cri native libraries. */
	virtual void PreFinalizeLibraries() {}

	/** Override this to finalize stuff after criware runtime finalized Cri native libraries. */
	virtual void PostFinalizeLibraries() {}
};

/** Management class for CriWareRuntime sub modules and expansions. */
class FCriWareCoreSubModules
{
	friend class UCriWareInitializer;

public:
	/** Register a sub module to receive initalization/finalization events from CriWareCore module. */
	FORCEINLINE void RegisterSubModule(ICriWareCoreSubModule* CriWareInterface) {
		ISubModules.Add(CriWareInterface);
	}

	/** Unregister a sub module to stop to receive initalization/finalization events from CriWareCore module. */
	FORCEINLINE void UnregisterSubModule(ICriWareCoreSubModule* CriWareInterface) {
		ISubModules.Remove(CriWareInterface);
	}

private:
	void OnPreInitializeLibraries();
	void OnPostInitializeLibraries();
	void OnPreFinalizeLibraries();
	void OnPostFinalizeLibraries();

private:
	TArray<ICriWareCoreSubModule*> ISubModules;
};
