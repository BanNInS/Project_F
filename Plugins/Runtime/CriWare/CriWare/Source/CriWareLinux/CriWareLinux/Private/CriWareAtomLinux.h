/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Linux plugin for Unreal Engine 4
 * Module   : Adx2 Atom for Linux
 * File     : CriWareAtomLinux.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "CriWarePlatform.h"

class FCriWareAtomLinux
	: public ICriWarePlatformAtom
{
public:

	FCriWareAtomLinux();
	virtual ~FCriWareAtomLinux();

	virtual FAtomRuntime* CreateAtomRuntime();
};
