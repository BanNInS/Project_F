/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWarePlatformUtils.cpp
 *
 ****************************************************************************/

#include "CriWarePlatformUtils.h"

#include "Misc/DataDrivenPlatformInfoRegistry.h"
#include "Misc/ConfigCacheIni.h"

//#ifdef PLATFORM_WINGDK

//#elif IS_MONOLITHIC && PLATFORM_WINDOWS
//#include "CriWareWindowsSettings.h"
//#endif

#include "CriWareCoreSettings.h"

bool FCriWarePlatformUtils::IsCurrentPlatformUsingAtomStreamCaching()
{
	// 	const FCriWareCookOverridesSettings* Settings = GetCookOverrides();
	// 	return Settings && Settings->bUseAtomStreamCaching;
	return true;
}

bool FCriWarePlatformUtils::IsCurrentPlatformUsingManaStreamCaching()
{
	// 	const FCriWareCookOverridesSettings* Settings = GetCookOverrides();
	// 	return Settings && Settings->bUseManaStreamCaching;
	return true;
}

static FCriticalSection CookOverridesCriticalSection;

static void CacheCriWareCookOverrides(FCriWareCookOverridesSettings& OutOverrides, const TCHAR* InPlatformName = nullptr)
{
	// if the platform was passed in, use it, otherwise, get the runtime platform's name for looking up DDPI
	FString PlatformName = InPlatformName ? FString(InPlatformName) : FString(FPlatformProperties::IniPlatformName());
	const FString ModuleName = TEXT("CriWare") + PlatformName + TEXT("Settings");
	const FString CategoryName = TEXT("/Script/") + ModuleName + TEXT(".") + ModuleName;

	// TODO: check valid cri platform 
	// if we don't support platform overrides, then return 
	if (CategoryName.Len() == 0)
	{
		OutOverrides = FCriWareCookOverridesSettings();
		return;
	}

	FConfigFile PlatformFile;
	FConfigCacheIni::LoadLocalIniFile(PlatformFile, TEXT("Engine"), true, *PlatformName);

	FString CookOverridesStr;
	PlatformFile.GetString(*CategoryName, TEXT("CookOverrides"), CookOverridesStr);
	CookOverridesStr.ReplaceInline(TEXT("="), TEXT(" "));
	FParse::Bool(*CookOverridesStr, TEXT("bUseAtomStreamCaching"), OutOverrides.bUseAtomStreamCaching);
	FParse::Bool(*CookOverridesStr, TEXT("bUseManaStreamCaching"), OutOverrides.bUseManaStreamCaching);
}

static inline FString GetCookOverridePlatformName(const TCHAR* PlatformName)
{
	return PlatformName ? FString(PlatformName) : FString(FPlatformProperties::IniPlatformName());
}

static FCriWareCookOverridesSettings& GetCacheableOverridesByPlatform(const TCHAR* InPlatformName, bool& bNeedsToBeInitialized)
{
	FScopeLock ScopeLock(&CookOverridesCriticalSection);

	// registry of overrides by platform name, for cooking, etc that may need multiple platforms worth
	static TMap<FString, FCriWareCookOverridesSettings> OverridesByPlatform;

	// make sure we don't reallocate the memory later
	if (OverridesByPlatform.Num() == 0)
	{
		// give enough space for all known platforms
		OverridesByPlatform.Reserve(FDataDrivenPlatformInfoRegistry::GetNumDataDrivenIniFiles());
	}

	FString PlatformName = GetCookOverridePlatformName(InPlatformName);
	// return one, or make one
	FCriWareCookOverridesSettings* ExistingOverrides = OverridesByPlatform.Find(PlatformName);
	if (ExistingOverrides != nullptr)
	{
		bNeedsToBeInitialized = false;
		return *ExistingOverrides;
	}

	bNeedsToBeInitialized = true;
	return OverridesByPlatform.Add(PlatformName, FCriWareCookOverridesSettings());
}

const FCriWareCookOverridesSettings* FCriWarePlatformUtils::GetCookOverrides(const TCHAR* PlatformName /* = nullptr */, bool bForceRecache /* = false */)
{
	bool bNeedsToBeInitialized;
	FCriWareCookOverridesSettings& Overrides = GetCacheableOverridesByPlatform(PlatformName, bNeedsToBeInitialized);

#if WITH_EDITOR
	// In editor situations, the settings can change at any time, so we need to retrieve them.

	if (GIsEditor && !IsRunningCommandlet())
	{
		static double LastCacheTime = 0.0;
		double CurrentTime = FPlatformTime::Seconds();
		double TimeSinceLastCache = CurrentTime - LastCacheTime;

		if (bForceRecache /* || TimeSinceLastCache > CookOverrideCachingIntervalCvar*/)
		{
			bNeedsToBeInitialized = true;
			LastCacheTime = CurrentTime;
		}
	}
#endif

	if (bNeedsToBeInitialized)
	{
		FScopeLock ScopeLock(&CookOverridesCriticalSection);
		CacheCriWareCookOverrides(Overrides, PlatformName);
	}

	return &Overrides;
}

void FCriWarePlatformUtils::RecacheCookOverrides()
{
	FScopeLock ScopeLock(&CookOverridesCriticalSection);
	bool bNeedsToBeInitialized;
	CacheCriWareCookOverrides(GetCacheableOverridesByPlatform(nullptr, bNeedsToBeInitialized));
}
