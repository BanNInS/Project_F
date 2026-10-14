/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE Apple plugin for Unreal Engine
 * Module   : Adx2 Atom for Apple
 * File     : CriWareAtomApple.cpp
 *
 ****************************************************************************/

#include "CriWareAtomApple.h"

#include "CriWareCore.h"
#include "Atom/AtomRuntime.h"

#include "Atom/AtomRuntimeApple.h"

/*
 * FCriWareAtomApple Implementation
 *****************************************************************************/

FCriWareAtomApple::FCriWareAtomApple()
	: ICriWarePlatformAtom()
{
	// Register this class to CriWare module for Atom platform.
	FCriWareCore::GetPlatform().RegisterPlatformAtomInterface(this);
}

FCriWareAtomApple::~FCriWareAtomApple()
{
	FCriWareCore::GetPlatform().UnregisterPlatformAtomInterface();
}

FAtomRuntime* FCriWareAtomApple::CreateAtomRuntime()
{
	return new FAtomRuntimeApple(this);
}
