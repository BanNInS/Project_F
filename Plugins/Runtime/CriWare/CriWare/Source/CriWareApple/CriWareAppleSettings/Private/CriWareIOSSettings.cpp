/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : IOS Settings for CriWare
* File     : CriWareIOSSettings.cpp
*
****************************************************************************/

#include "CriWareIOSSettings.h"

DEFINE_LOG_CATEGORY(LogCriWareIOSSettings);

#define LOCTEXT_NAMESPACE "CriWareIOSSettings"

UCriWareIOSSettings::UCriWareIOSSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionName = TEXT("iOS - ADX Atom");
}

//~ UDeveloperSettings interface
#if WITH_EDITOR
FText UCriWareIOSSettings::GetSectionText() const
{
	return LOCTEXT("TargetSettingsName", "iOS - ADX Atom");
}

FText UCriWareIOSSettings::GetSectionDescription() const
{
	return LOCTEXT("TargetSettingsDescription", "Settings for iOS target platform.");
}
#endif

#undef LOCTEXT_NAMESPACE
