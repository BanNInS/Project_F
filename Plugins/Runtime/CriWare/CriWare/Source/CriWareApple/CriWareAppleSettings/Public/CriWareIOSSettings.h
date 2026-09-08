/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : Module Settings for CriWareIOS
* File     : CriWareIOSSettings.h
*
****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"

#include "CriWareAppleSettings.h"

#include "CriWareIOSSettings.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareIOSSettings, Log, All);

/**
 * Implements the settings for CriWare on IOS platform.
 */
UCLASS(config = CriWare, DefaultConfig, meta = (DisplayName = "ADX Atom iOS"))
class CRIWAREAPPLESETTINGS_API UCriWareIOSSettings 
	: public UCriWareAppleSettings
{
	GENERATED_BODY()

public:

	UCriWareIOSSettings(const FObjectInitializer& ObjectInitializer);

	//~ Begin UDeveloperSettings interface
	virtual FName GetCategoryName() const override { return TEXT("CriWare"); }
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif
	//~ End UDeveloperSettings interface
};
