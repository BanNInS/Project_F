/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : Module Interface of CriWareMixer
 * File     : CriWareAtomMixerModule.h
 *
 ****************************************************************************/

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"		// For inline LoadModuleChecked()

/**
 * The public interface to this module
 */
class ICriWareAtomMixerModule
	: public IModuleInterface
{
public:

    /**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline ICriWareAtomMixerModule& Get()
	{
		return FModuleManager::LoadModuleChecked<ICriWareAtomMixerModule>("CriWareAtomMixer");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("CriWareAtomMixer");
	}
};
