/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Windows plugin for Unreal Engine 4
 * Module   : Adx2 Atom for Windows
 * File     : CriWareAtomWindows.cpp
 *
 ****************************************************************************/

#include "CriWareAtomWindows.h"

#include "CriWareCore.h"
#include "Atom/AtomRuntime.h"

#include "Atom/AtomRuntimeWindows.h"

/*
 * FCriWareAtomWindows Implementation
 *****************************************************************************/

FCriWareAtomWindows::FCriWareAtomWindows()
	: ICriWarePlatformAtom()
{
	// Register this class to CriWare module for Atom platform.
	FCriWareCore::GetPlatform().RegisterPlatformAtomInterface(this);
}

FCriWareAtomWindows::~FCriWareAtomWindows()
{
	FCriWareCore::GetPlatform().UnregisterPlatformAtomInterface();
}

FAtomRuntime* FCriWareAtomWindows::CreateAtomRuntime()
{
	return new FAtomRuntimeWindows(this);
}
