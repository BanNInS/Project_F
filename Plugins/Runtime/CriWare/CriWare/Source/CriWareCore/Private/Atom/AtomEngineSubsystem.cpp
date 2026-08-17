/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomEngineSubsystem.cpp
 *
 ****************************************************************************/

#include "Atom/AtomEngineSubsystem.h"

#include "Atom/AtomRuntime.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomEngineSubsystem)

UAtomEngineSubsystem::UAtomEngineSubsystem()
	: UDynamicSubsystem()
{
}

FAtomRuntimeId UAtomEngineSubsystem::GetAtomRuntimeID() const
{
	const UAtomSubsystemCollectionRoot* SubsystemRoot = Cast<UAtomSubsystemCollectionRoot>(GetOuter());
	check(SubsystemRoot);

	return SubsystemRoot->GetAtomRuntimeID();
}

FAtomRuntimeHandle UAtomEngineSubsystem::GetAtomRuntimeHandle() const
{
	const UAtomSubsystemCollectionRoot* SubsystemRoot = Cast<UAtomSubsystemCollectionRoot>(GetOuter());
	check(SubsystemRoot);

	if (FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get())
	{
		return AtomRuntimeManager->GetAtomRuntime(SubsystemRoot->GetAtomRuntimeID());
	}

	return FAtomRuntimeHandle();
}

Atom::FMixerSourceManager* UAtomEngineSubsystem::GetMutableSourceManager()
{
	FAtomRuntime* AtomRuntime = GetMutableAtomRuntime();
	if (AtomRuntime)
	{
		return AtomRuntime->GetSourceManager();
	}
	return nullptr;
}

const Atom::FMixerSourceManager* UAtomEngineSubsystem::GetSourceManager() const
{
	const FAtomRuntime* AtomRuntime = GetAtomRuntime();
	if (AtomRuntime)
	{
		return AtomRuntime->GetSourceManager();
	}
	return nullptr;
}

FAtomRuntime* UAtomEngineSubsystem::GetMutableAtomRuntime()
{
	FAtomRuntime* AtomRuntime = GetAtomRuntimeHandle().GetAtomRuntime();
	return AtomRuntime;
}

const FAtomRuntime* UAtomEngineSubsystem::GetAtomRuntime() const
{
	const FAtomRuntime* AtomRuntime = GetAtomRuntimeHandle().GetAtomRuntime();
	return AtomRuntime;
}
