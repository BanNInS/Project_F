/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : Android Settings for CriWare
* File     : CriWareAndroidSettings.cpp
*
****************************************************************************/

#include "CriWareAndroidSettings.h"

#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif

#include "CriWare.h"
#include "CriWareCoreSettings.h"

DEFINE_LOG_CATEGORY(LogCriWareAndroidSettings);

#define LOCTEXT_NAMESPACE "CriWareAndroidSettings"

UCriWareAndroidSettings::UCriWareAndroidSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionName = TEXT("Android - ADX Atom");
}

void UCriWareAndroidSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);

#if PLATFORM_ANDROID

	

#endif // PLATFORM_ANDROID
}

//~ UDeveloperSettings interface
#if WITH_EDITOR
FText UCriWareAndroidSettings::GetSectionText() const
{
	return LOCTEXT("TargetSettingsName", "Android - ADX Atom");
}

FText UCriWareAndroidSettings::GetSectionDescription() const
{
	return LOCTEXT("TargetSettingsDescription", "Settings for Android target platform.");
}
#endif

//~ UObject interface
#if WITH_EDITOR

void UCriWareAndroidSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	auto CoreSettings = GetDefault<UCriWareCoreSettings>();

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// not editor platform - not need to reactivate Atom
	/*if (MemberPropertyName != NAME_None && MemberPropertyName != GET_MEMBER_NAME_CHECKED(UCriWareAndroidSettings, CookOverrides))
	{
		// reactivate the active Atom runtime
		//CoreSettings->ReactivateActiveRuntime();
	}*/

	Super::PostEditChangeProperty(PropertyChangedEvent);

	//CoreSettings->CriWareCoreSettingsChanged.Broadcast();
}

void UCriWareAndroidSettings::PostInitProperties()
{
	Super::PostInitProperties();
}

#endif

#undef LOCTEXT_NAMESPACE
