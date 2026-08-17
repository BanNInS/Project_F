/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE AudioLInk plugin for Unreal Engine
 * Module   : IModuleInterface Class for CriWareAudioLink module
 * File     : CriWareAudioLinkModule.h
 *
 ****************************************************************************/

#pragma once

#include "Engine/Engine.h"
#include "Templates/UniquePtr.h"

#include "AtomAudioLinkFactory.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareAudioLink, Log, All);

class FCriWareAudioLinkModule
{
public:
	
	FCriWareAudioLinkModule()
	{
		Factory = MakeUnique<FAtomAudioLinkFactory>();
	}
	
	~FCriWareAudioLinkModule()
	{
		Factory.Reset();
	}

private:
	
	TUniquePtr<FAtomAudioLinkFactory> Factory;
};
