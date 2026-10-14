/****************************************************************************
*
* CRI Middleware SDK
*
* Copyright (c) 2021 CRI Middleware Co., Ltd.
*
* Library  : CRIWARE plugin for Unreal Engine 4
* Module   : Platform
* File     : CriWarePlatform.h
*
****************************************************************************/
#pragma once

#include "CriWareMemory.h"

class FAtomRuntime;

/* Platform specific module interfaces
 *****************************************************************************/

class ICriWarePlatformAtom
{
protected:

	virtual ~ICriWarePlatformAtom() {}

public:

	FORCEINLINE virtual FCriWareAllocator& GetMemoryAllocator(ECriWareMemoryType MemoryType) { return FCriWareAllocator::GetDefault(); }

	virtual FAtomRuntime* CreateAtomRuntime() { return nullptr; }
};

class FCriWarePlatform
{
public:

	FCriWarePlatform() : IAtom(nullptr) {}

	FORCEINLINE void RegisterPlatformAtomInterface(ICriWarePlatformAtom* AtomInterface) { IAtom = AtomInterface; }
	FORCEINLINE void UnregisterPlatformAtomInterface() { IAtom = nullptr; }
	FORCEINLINE ICriWarePlatformAtom* Atom() { return IAtom; }

private:

	ICriWarePlatformAtom* IAtom;
};
