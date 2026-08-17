/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2020-2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : Core sub modules listener.
 * File     : CriWareCoreSubModules.cpp
 *
 ****************************************************************************/

#include "CriWareCoreSubModules.h"

//~ FCriWareCoreSubModules implementation

void FCriWareCoreSubModules::OnPreInitializeLibraries()
{
	for (int i = 0; i < ISubModules.Num(); i++)
	{ 
		ISubModules[i]->PreInitializeLibraries();
	}
}

void FCriWareCoreSubModules::OnPostInitializeLibraries()
{
	for (int i = 0; i < ISubModules.Num(); i++)
	{
		ISubModules[i]->PostInitializeLibraries();
	}
}

void FCriWareCoreSubModules::OnPreFinalizeLibraries()
{
	for (int i = 0; i < ISubModules.Num(); i++)
	{
		ISubModules[i]->PreFinalizeLibraries();
	}
}

void FCriWareCoreSubModules::OnPostFinalizeLibraries()
{
	for (int i = 0; i < ISubModules.Num(); i++)
	{
		ISubModules[i]->PostFinalizeLibraries();
	}
}
