/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine 4
* Module   : Linux Settings for CriWare
* File     : CriWareLinuxSettings.cpp
*
****************************************************************************/

#include "CriWareLinuxSettings.h"

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

DEFINE_LOG_CATEGORY(LogCriWareLinuxSettings);

#define LOCTEXT_NAMESPACE "CriWareLinuxSettings"

UCriWareLinuxSettings::UCriWareLinuxSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionName = TEXT("Linux - ADX Atom");
}

void UCriWareLinuxSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);

#if PLATFORM_LINUX

	

#endif //PLATFORM_LINUX
}

//~ UDeveloperSettings interface
#if WITH_EDITOR
FText UCriWareLinuxSettings::GetSectionText() const
{
	return LOCTEXT("TargetSettingsName", "Linux - ADX Atom");
}

FText UCriWareLinuxSettings::GetSectionDescription() const
{
	return LOCTEXT("TargetSettingsDescription", "Settings for Linux target platform.");
}
#endif

//~ UObject interface
#if WITH_EDITOR
void UCriWareLinuxSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	auto CoreSettings = GetDefault<UCriWareCoreSettings>();

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

#if PLATFORM_LINUX
	if (MemberPropertyName != NAME_None && MemberPropertyName != GET_MEMBER_NAME_CHECKED(UCriWareLinuxSettings, CookOverrides))
	{
		// reactivate the active Atom runtime
		CoreSettings->ReactivateActiveRuntime();
	}
#endif

	Super::PostEditChangeProperty(PropertyChangedEvent);

	//CoreSettings->CriWareCoreSettingsChanged.Broadcast();
}

void UCriWareLinuxSettings::PostInitProperties()
{
	Super::PostInitProperties();
}
#endif

#undef LOCTEXT_NAMESPACE
