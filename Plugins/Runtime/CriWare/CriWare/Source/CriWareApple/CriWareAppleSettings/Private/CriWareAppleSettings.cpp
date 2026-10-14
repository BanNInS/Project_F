/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : Apple Settings for CriWare
* File     : CriWareAppleSettings.cpp
*
****************************************************************************/

#include "CriWareAppleSettings.h"

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

DEFINE_LOG_CATEGORY(LogCriWareAppleSettings);

#define LOCTEXT_NAMESPACE "CriWareAppleSettings"

UCriWareAppleSettings::UCriWareAppleSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionName = TEXT("Apple - ADX Atom");
}

void UCriWareAppleSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);
}

//~ UObject interface
#if WITH_EDITOR

void UCriWareAppleSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	auto CoreSettings = GetDefault<UCriWareCoreSettings>();

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	// not editor platform - not need to reactivate Atom
	/*if (MemberPropertyName != NAME_None && MemberPropertyName != GET_MEMBER_NAME_CHECKED(UCriWareAppleSettings, CookOverrides))
	{
		// reactivate the active Atom runtime
		//CoreSettings->ReactivateActiveRuntime();
	}*/

	Super::PostEditChangeProperty(PropertyChangedEvent);

	//CoreSettings->CriWareCoreSettingsChanged.Broadcast();
}

void UCriWareAppleSettings::PostInitProperties()
{
	Super::PostInitProperties();
}

#endif

#undef LOCTEXT_NAMESPACE
