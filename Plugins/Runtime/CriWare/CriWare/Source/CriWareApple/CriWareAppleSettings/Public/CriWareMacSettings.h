/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine
* Module   : Module Settings for CriWareMac
* File     : CriWareMacSettings.h
*
****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"

#include "CriWareAppleSettings.h"

#include "CriWareMacSettings.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareMacSettings, Log, All);

/**
 * Implements the settings for CriWare on Mac platform.
 */
UCLASS(config = CriWare, DefaultConfig, meta = (DisplayName = "ADX Atom macOS"))
class CRIWAREAPPLESETTINGS_API UCriWareMacSettings
	: public UCriWareAppleSettings
{
	GENERATED_BODY()

public:

	UCriWareMacSettings(const FObjectInitializer& ObjectInitializer);

	//~ Begin UDeveloperSettings Interface
	virtual FName GetCategoryName() const override { return TEXT("CriWare"); }
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif
	//~ End UDeveloperSettings Interface

private:
	//~ Begin UObject Interface
#if PLATFORM_MAC && WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface
};
