/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWarePlatformUtils.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

// Forward definitions.
struct FCriWareCookOverridesSettings;

class CRIWARECORE_API FCriWarePlatformUtils
{
public:

	// null platformname means to use current platform
	static const FCriWareCookOverridesSettings* GetCookOverrides(const TCHAR* PlatformName = nullptr, bool bForceRecache = false);

	// 
	static void RecacheCookOverrides();

	static bool IsCurrentPlatformUsingAtomStreamCaching();
	static bool IsCurrentPlatformUsingManaStreamCaching();

private:
	// dont use deprected
	//static const FCriWareCookOverridesSettings* GetRuntimeCriWareCookOverridesForCurrentPlatform();
};
