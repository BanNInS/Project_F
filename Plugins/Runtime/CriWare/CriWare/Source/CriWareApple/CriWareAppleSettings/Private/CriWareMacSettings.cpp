/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : Apple MacOS Settings for CriWare
* File     : CriWareMacSettings.cpp
*
****************************************************************************/

#include "CriWareMacSettings.h"

DEFINE_LOG_CATEGORY(LogCriWareMacSettings);

#define LOCTEXT_NAMESPACE "CriWareMacSettings"

UCriWareMacSettings::UCriWareMacSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionName = TEXT("macOS - ADX Atom");
}

//~ UDeveloperSettings Interface
#if WITH_EDITOR
FText UCriWareMacSettings::GetSectionText() const
{
	return LOCTEXT("TargetSettingsName", "macOS - ADX Atom");
}

FText UCriWareMacSettings::GetSectionDescription() const
{
	return LOCTEXT("TargetSettingsDescription", "Settings for macOS target platform.");
}
#endif

//~ UObject Interface
#if PLATFORM_MAC && WITH_EDITOR
void UCriWareMacSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	auto CoreSettings = GetDefault<UCriWareCoreSettings>();

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (MemberPropertyName != NAME_None && MemberPropertyName != GET_MEMBER_NAME_CHECKED(UCriWareAppleSettings, CookOverrides))
	{
		// reactivate the active Atom runtime
		CoreSettings->ReactivateActiveRuntime();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

#undef LOCTEXT_NAMESPACE
