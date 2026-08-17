/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE AudioLInk plugin for Unreal Engine
 * Module   : IModuleInterface Class for CriWareAudioLinkEditor module
 * File     : CriWareAudioLinkEditorModule.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FCriWareAudioLinkEditorModule
{
public:

	FCriWareAudioLinkEditorModule();
	~FCriWareAudioLinkEditorModule();

private:

	/** The collection of registered customized class names. */
	TArray<FName> CustomClassLayoutNames;
};

