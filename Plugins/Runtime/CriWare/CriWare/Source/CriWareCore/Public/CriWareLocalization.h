/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWareLocalization.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#if !defined(CRIWARE_ENABLE_LOCALIZATION)
#define CRIWARE_ENABLE_LOCALIZATION 1
#endif

#if CRIWARE_ENABLE_LOCALIZATION
 /** The global namespace that must be defined/undefined to wrap uses of the NS-prefixed macros below */
#undef LOCTEXT_NAMESPACE

 /**
 * Creates an FText. All parameters, that are not string literals, will be passed through the localization system.
 * The global LOCTEXT_NAMESPACE macro must be first set to a string literal to specify this localization key's namespace.
 */
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
#define CRI_LOCTEXT( InNamespace, InKey, InText ) FText::AsLocalizable_Advanced( TEXT(LOCTEXT_NAMESPACE), InKey, InText );
#else
#define CRI_LOCTEXT( InKey, InText ) FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText( InText, TEXT(LOCTEXT_NAMESPACE), InKey )
#endif

 /**
 * Creates an FText. All parameters that are not string literals, will be passed through the localization system.
 */
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
#define CRI_NSLOCTEXT( InNamespace, InKey, InText ) FText::AsLocalizable_Advanced( InNamespace, InKey, InText );
#else
#define CRI_NSLOCTEXT( InNamespace, InKey, InText ) FInternationalization::ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText( InText, InNamespace, InKey )
#endif

/**
* Macro to localize a class in CriWare plugin. 
* Localization of unreal properties metadata with LOCTEXT markups and reflection.
*/
#define	CRI_LOCCLASS(InClass) {\
	static bool InitLocalizedMetaData = true;\
	if (InitLocalizedMetaData) {\
		InitLocalizedMetaData = false;\
		auto& CriWareLocRes = FCriWareLocalization::GetLocalizationResources();\
		CriWareLocRes.LocalizeClassFromUStructDeclaration(InClass, TEXT(LOCTEXT_NAMESPACE));\
	}\
}

/**
* Macro to localize a structure in CriWare plugin (force usage of a key prefix).
* Localization of unreal properties metadata with LOCTEXT markups and reflection.
*/
#define	CRI_LOCSTRUCT(InStruct) {\
	static bool InitLocalizedMetaData = true;\
	if (InitLocalizedMetaData) {\
		InitLocalizedMetaData = false;\
		auto& CriWareLocRes = FCriWareLocalization::GetLocalizationResources();\
		CriWareLocRes.LocalizeClassFromUStructDeclaration(InStruct, TEXT(LOCTEXT_NAMESPACE), true);\
	}\
}
#else
#define	CRI_LOCCLASS(InClass)
#define	CRI_LOCSTRUCT(InStruct)
#endif

#if CRIWARE_ENABLE_LOCALIZATION
class CRIWARECORE_API FCriWareLocalization
{
private:
	bool bResourcesLoaded = false;

private:
	FCriWareLocalization();

public:
	/* Load CriWare Localized Resources Files if not already loaded, then return singleton. */
	static FCriWareLocalization& GetLocalizationResources();

#if WITH_EDITOR
	/* Use DisplayName and ToolTip defined in UStruct or UClass header as Key for Localization. 
	 * Note: You can enable use of Ustruct.Name as prefix to avoid duplicated keys in same localization namespace.
	 */
	void LocalizeClassFromUStructDeclaration(UStruct* InStruct, const TCHAR* InNamespace, bool bUseStructPrefixInKey = false);
#endif
};
#endif
