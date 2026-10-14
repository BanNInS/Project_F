/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Windows plugin for Unreal Engine 4
 * Module   : Adx2 Atom for Windows
 * File     : CriWareAtomWindows.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "CriWarePlatform.h"

class FCriWareAtomWindows
	: public ICriWarePlatformAtom
{
public:

	FCriWareAtomWindows();
	virtual ~FCriWareAtomWindows();

	virtual FAtomRuntime* CreateAtomRuntime();
};
