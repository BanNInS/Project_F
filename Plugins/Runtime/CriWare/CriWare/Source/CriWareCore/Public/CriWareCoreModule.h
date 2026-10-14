/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : Module Interface of CriWareCore
 * File     : CriWareCoreModule.h
 *
 ****************************************************************************/

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"		// For inline LoadModuleChecked()

// Forward Declarations
class FCriWareCore;

struct FAtomAudioLinkSynchronizer;
using FAtomAudioLinkSynchronizerPtr = TSharedPtr<FAtomAudioLinkSynchronizer>;

/**
 * The public interface to this module
 */
class ICriWareCoreModule : public IModuleInterface
{
public:
	/** Delegate to create an object that implements IAtomSoundBaseRendererInterface. */
	DECLARE_DELEGATE_RetVal(UObject*, FOnCreateAtomSoundBaseRenderer);

    /**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline ICriWareCoreModule& Get()
	{
		return FModuleManager::LoadModuleChecked<ICriWareCoreModule>("CriWareCore");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("CriWareCore");
	}

	/**
	 * Gets the Core object if created.
	 *
	 * @return the core object as shared pointer.
	 */
	virtual TSharedPtr<FCriWareCore> GetCriWareCore() = 0;

	/**
	 * Register a delegate to create an object that implements IAtomSoundBaseRendererInterface.
	 */
	virtual void RegisterCreateAtomSoundBaseRenderer(const FOnCreateAtomSoundBaseRenderer& Delegate) = 0;

	/**
	 * Unregisters the delegate passed in with RegisterCreateAtomSoundBaseRenderer.
	 */
	virtual void UnregisterCreateAtomSoundBaseRenderer() = 0;

	/**
	 * Creates an object that implements IAtomSoundBaseRendererInterface.
	 */
	virtual UObject* CreateAtomSoundBaseRenderer() = 0;
};
