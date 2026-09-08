/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Android plugin for Unreal Engine
 * Module   : Adx2 Atom for Android
 * File     : CriWareAtomAndroid.cpp
 *
 ****************************************************************************/

#include "CriWareAtomAndroid.h"

#include "CriWareCore.h"
#include "Atom/AtomRuntime.h"

#include "Atom/AtomRuntimeAndroid.h"

/*
 * FCriWareAtomAndroid Implementation
 *****************************************************************************/

FCriWareAtomAndroid::FCriWareAtomAndroid()
	: ICriWarePlatformAtom()
{
	// Register this class to CriWare module for Atom platform.
	FCriWareCore::GetPlatform().RegisterPlatformAtomInterface(this);
}

FCriWareAtomAndroid::~FCriWareAtomAndroid()
{
	FCriWareCore::GetPlatform().UnregisterPlatformAtomInterface();
}

FAtomRuntime* FCriWareAtomAndroid::CreateAtomRuntime()
{
	return new FAtomRuntimeAndroid(this);
}
