/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Android plugin for Unreal Engine
 * Module   : Adx2 Atom for Android
 * File     : CriWareAtomAndroid.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "CriWarePlatform.h"

class FCriWareAtomAndroid
	: public ICriWarePlatformAtom
{
public:

	FCriWareAtomAndroid();
	virtual ~FCriWareAtomAndroid();

	virtual FAtomRuntime* CreateAtomRuntime();
};
