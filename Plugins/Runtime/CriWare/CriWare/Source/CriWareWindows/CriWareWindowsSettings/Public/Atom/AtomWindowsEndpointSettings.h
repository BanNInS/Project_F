/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : Module Settings for CriWare on Windows
* File     : AtomWindowsEndpointSettings.h
*
****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Templates/UniquePtr.h"

#include "Atom/Atom.h"
#include "Extensions/IAtomEndpoint.h"

#include "AtomWindowsEndpointSettings.generated.h"

UENUM()
enum class EAtomWindowsHardwareDevices : uint8
{
	HardwareDevice1 = 0,
	HardwareDevice2,
	HardwareDevice3,
	HardwareDevice4
};

class FAtomWindowsEndpointSettings : public IAtomEndpointSettingsProxy
{
public:
	bool bOverrideHardwareDevice = false;
	EAtomWindowsHardwareDevices HardwareDeviceOverrides = EAtomWindowsHardwareDevices::HardwareDevice1;
	bool bSendToVibrationOutput = false;
};

UCLASS()
class CRIWAREWINDOWSSETTINGS_API UAtomWindowsEndpointSettings : public UAtomEndpointSettingsBase
{
	GENERATED_BODY()

public:

	UAtomWindowsEndpointSettings(const FObjectInitializer& ObjectInitializer) 
		: Super(ObjectInitializer)
	{
	}

	/** Wheter to override the hardware device mapping set in project properties for Atom Windows. */
	UPROPERTY(EditAnyWhere, Category = "Atom|SoundRenderer", meta = (InlineEditConditionToggle))
	bool bOverrideHardwareDevice = false;

	/** The hardware device to override mapping set in project properties for Atom Windows. */
	UPROPERTY(EditAnyWhere, Category = "Atom|SoundRenderer", meta = (EditCondition = "bOverrideHardwareDevice"))
	EAtomWindowsHardwareDevices HardwareDeviceOverrides = EAtomWindowsHardwareDevices::HardwareDevice1;

	/** Outputs the sound as vibration if supported by device. */
	UPROPERTY(EditAnyWhere, Category = "Atom|SoundRenderer")
	bool bSendToVibrationOutput = false;

	virtual TUniquePtr<IAtomEndpointSettingsProxy> GetProxy() const override
	{
		FAtomWindowsEndpointSettings* Settings = new FAtomWindowsEndpointSettings();
		Settings->bOverrideHardwareDevice = bOverrideHardwareDevice;
		Settings->HardwareDeviceOverrides = HardwareDeviceOverrides;
		Settings->bSendToVibrationOutput = bSendToVibrationOutput;
		return TUniquePtr<IAtomEndpointSettingsProxy>(Settings);
	}

#if WITH_EDITORONLY_DATA
	virtual bool IsValidForPlatform(const FString& PlatformName) const override
	{
		return PlatformName == TEXT("Windows");
	}

	virtual FString GetSettingsName() const override
	{
		return TEXT("Atom Endpoint Settings Windows");
	}
#endif
};
