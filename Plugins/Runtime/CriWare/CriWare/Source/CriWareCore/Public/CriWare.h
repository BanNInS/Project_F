/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWare.h
 *
 ****************************************************************************/

#pragma once

#include "UObject/Object.h"

#include "CriWareCore.h"

#include "CriWare.generated.h"

// Forward Definitions
class UAtomConfig;
class FAtomRuntime;
class FAtomRuntimeManager;
class FAtomResourceManager;
class IAtomRuntimePlugin;
using TAtomRuntimePluginPtr = TSharedPtr<IAtomRuntimePlugin, ESPMode::ThreadSafe>;

/**
 * Main UObject class for CriWare.
 * Interface to CriWare libraries and global features.
 */
UCLASS(Transient)
class CRIWARECORE_API UCriWare
	: public UObject
	, public FExec
{
	GENERATED_BODY()

public:

	virtual void Init(TWeakPtr<FCriWareCore> InCriWareCore);

	//~ Begin UObject Interface
	void FinishDestroy() override;
#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	//~ End UObject Interface

private:
	/** The Atom configuration. */
	UPROPERTY()
	TObjectPtr<const UAtomConfig> AtomConfig;

public:

	/** */
	bool Initialize();

	/** */
	bool Uninitialize();

	/** Check if initialized. */
	bool IsInitialized() const { if (TSharedPtr<FCriWareCore> Core = CriWareCore.Pin()) { return Core->IsInitialized(); } return false; }

	/** Registers AtomConfig data to Atom library. */
	bool SetAtomConfiguration(const UAtomConfig* InAtomConfig);

	/** Unregister current AtomConfig data from Atom library. */
	void ResetAtomConfiguration() { SetAtomConfiguration(nullptr); }

	/** The currently registered AtomConfig. */
	const UAtomConfig* GetAtomConfiguration() const { return AtomConfig; }

public:

	//~ Begin FExec Interface
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Out = *GLog) override;
	//~ End FExec Interface

	/** Access to current platform module interface. */
	FORCEINLINE FCriWarePlatform& GetPlatform() { return FCriWareCore::GetPlatform(); }

	/** Gets the underlying CriWareCore object. */
	FORCEINLINE TSharedPtr<FCriWareCore> GetCriWareCore() { return CriWareCore.Pin(); }

	/** Gets the current active Atom runtime. */
	FAtomRuntimeHandle GetActiveAtomRuntime() const;

	/** Gets the main Atom runtime. */
	FAtomRuntimeHandle GetMainAtomRuntime() const;

	/** Gets the Atom runtime manager. */
	FAtomRuntimeManager* GetAtomRuntimeManager() const;

	/** Gets the Atom resource manager. */
	FAtomResourceManager* GetAtomResourceManager() const;

	/** Gets the Mana Runtime if exist. */
	TAtomRuntimePluginPtr GetAtomRuntimePlugin(FName PluginName) const;

private:

	TWeakPtr<FCriWareCore> CriWareCore;

#if WITH_EDITOR
	// Reserved resources when using editor.
	int NumReservedFsBinders = 128;
	int NumReservedFsMaxBindings = 128;
	int NumReservedFsFileLoaders = 128;
	int NumReservedManaHandles = 4;
#endif
};

/** Global CriWare pointer. Can be 0 so don't use without checking. */
extern CRIWARECORE_API class UCriWare* GCriWare;

/** Global CriWare ini filename. */
extern CRIWARECORE_API FString GCriWareIni;
