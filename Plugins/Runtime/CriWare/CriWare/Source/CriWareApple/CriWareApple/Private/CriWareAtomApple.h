/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Apple plugin for Unreal Engine
 * Module   : Adx2 Atom for Apple
 * File     : CriWareAtomApple.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "CriWarePlatform.h"

class FCriWareAtomApple
	: public ICriWarePlatformAtom
{
public:

	FCriWareAtomApple();
	virtual ~FCriWareAtomApple();

	virtual FAtomRuntime* CreateAtomRuntime();
};
