/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine 4
* Module   : Windows Settings for CriWare
* File     : CriWareWindowsSettings.cpp
*
****************************************************************************/

#include "CriWareWindowsSettings.h"

#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CoreDelegates.h"
#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif

#include "CriWare.h"
#include "CriWareApi.h"
#include "CriWareCoreSettings.h"

DEFINE_LOG_CATEGORY(LogCriWareWindowsSettings);

#define LOCTEXT_NAMESPACE "CriWareWindowsSettings"

UCriWareWindowsSettings::UCriWareWindowsSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SectionName = TEXT("Windows - ADX Atom");

	//     Common WASAPI
	WASAPISettings.bWASAPI_IsExclusive = false;
	WASAPISettings.WASAPI_BitsPerSample = 24;
	WASAPISettings.WASAPI_NumChannels = CRIATOM_DEFAULT_INPUT_MAX_CHANNELS;
	WASAPISettings.WASAPI_SamplingRate = CRIATOM_DEFAULT_OUTPUT_SAMPLING_RATE;

	for (int Index = (uint8)EAtomSoundRendererType::Default; Index <= (uint8)EAtomSoundRendererType::Aux; Index++)
	{
		EndpointMappings[Index] = FName(GetHardwareDeviceOptions()[0]);
	}

	// Since property values are set by customize, add a space to CDO values to ensure they are saved to config file even if reverted.
	FName DefaultDeviceName = *(LOCTEXT("AtomDefaultDeviceLabel", "Default Device").ToString() + TEXT(" "));
	Hardware1 = Hardware2 = Hardware3 = Hardware4 = DefaultDeviceName;
}

void UCriWareWindowsSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);

#if PLATFORM_WINDOWS

	

#endif //PLATFORM_WINDOWS
}

//~ UDeveloperSettings interface
#if WITH_EDITOR
FText UCriWareWindowsSettings::GetSectionText() const
{
	return LOCTEXT("TargetSettingsName", "Windows - ADX Atom");
}

FText UCriWareWindowsSettings::GetSectionDescription() const
{
	return LOCTEXT("TargetSettingsDescription", "Settings for Windows target platform.");
}
#endif

//~ UObject interface
#if WITH_EDITOR

void UCriWareWindowsSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	auto CoreSettings = GetDefault<UCriWareCoreSettings>();

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

#if PLATFORM_WINDOWS
	if (MemberPropertyName != NAME_None && MemberPropertyName != GET_MEMBER_NAME_CHECKED(UCriWareWindowsSettings, CookOverrides))
	{
		// reactivate the active Atom runtime
		CoreSettings->ReactivateActiveRuntime();
	}
#endif

	Super::PostEditChangeProperty(PropertyChangedEvent);

	CoreSettings->CriWareCoreSettingsChanged.Broadcast();
}

void UCriWareWindowsSettings::PostInitProperties()
{
	Super::PostInitProperties();
}

#endif

TArray<FString> UCriWareWindowsSettings::GetHardwareDeviceOptions() const
{
	return { "Hardware Device 1", "Hardware Device 2", "Hardware Device 3", "Hardware Device 4" };
}

#undef LOCTEXT_NAMESPACE
