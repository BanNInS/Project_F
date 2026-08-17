/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Linux plugin for Unreal Engine 4
 * Module   : Adx2 Atom for Linux
 * File     : CriWareAtomLinux.cpp
 *
 ****************************************************************************/

#include "CriWareAtomLinux.h"

#include "CriWareCore.h"
#include "Atom/AtomRuntime.h"

#include "Atom/AtomRuntimeLinux.h"

/*
 * FCriWareAtomLinux Implementation
 *****************************************************************************/

FCriWareAtomLinux::FCriWareAtomLinux()
	: ICriWarePlatformAtom()
{
	// Register this class to CriWare module for Atom platform.
	FCriWareCore::GetPlatform().RegisterPlatformAtomInterface(this);
}

FCriWareAtomLinux::~FCriWareAtomLinux()
{
	FCriWareCore::GetPlatform().UnregisterPlatformAtomInterface();
}

FAtomRuntime* FCriWareAtomLinux::CreateAtomRuntime()
{
	return new FAtomRuntimeLinux(this);
}
