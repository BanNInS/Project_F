/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomEngineSubsystem.h
 *
 ****************************************************************************/

#pragma once

#include "Subsystems/Subsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "Runtime/Launch/Resources/Version.h"

#include "Atom/AtomRuntimeManager.h"

#include "AtomEngineSubsystem.generated.h"

// Forward Declarations
class FAtomRuntime;

namespace Atom
{
	class FMixerSourceManager;
} // namespace

/**
 * UAtomSubsystemCollectionRoot
 * Root UObject used to anchor UAtomEngineSubsystems to the FAtomRuntime.
 */
UCLASS(MinimalAPI)
class UAtomSubsystemCollectionRoot final
	: public UObject
{
	GENERATED_BODY()

public:

	/** Set the ID of the owning Atom runtime. */
	FORCEINLINE void SetAtomRuntimeID(FAtomRuntimeId RuntimeID) { OwningRuntimeID = RuntimeID; }

	/** Get the ID of the owning Atom runtime. */
	FAtomRuntimeId GetAtomRuntimeID() const { return OwningRuntimeID; }

protected:

	FAtomRuntimeId OwningRuntimeID = INDEX_NONE;
};

/**
 * UAtomEngineSubsystem
 * Base class for auto instanced and initialized systems that share the lifetime of the Atom runtime.
 */
UCLASS(Abstract, MinimalAPI)
class UAtomEngineSubsystem
	: public UDynamicSubsystem
{
	GENERATED_BODY()

public:

	CRIWARECORE_API UAtomEngineSubsystem();

	/**
	 * Override to get an update call during AtomRuntime::Update
	 *  Note: This call will happen on the Atom thread
	 */
	virtual void Update() {}

	/** Returns the owning Atom runtime ID */
	CRIWARECORE_API virtual FAtomRuntimeId GetAtomRuntimeID() const final;

	/** Returns the owning Atom runtime handle */
	CRIWARECORE_API virtual FAtomRuntimeHandle GetAtomRuntimeHandle() const final;

	/** Return a mutable version of the source manager associated with the owning device handle */
	CRIWARECORE_API virtual Atom::FMixerSourceManager* GetMutableSourceManager() final;

	/** Return the source manager associated with the owning device handle */
	CRIWARECORE_API virtual const Atom::FMixerSourceManager* GetSourceManager() const final;

	/** Return a mutable version of the mixer device from the owning device handle */
	CRIWARECORE_API virtual FAtomRuntime* GetMutableAtomRuntime() final;

	/** Return the mixer device from the owning device handle */
	CRIWARECORE_API virtual const FAtomRuntime* GetAtomRuntime() const final;
};

/**
 * FAtomSubsystemCollection - Subsystem collection specifically targeting UAtomEngineSubsystems
 */
class FAtomSubsystemCollection
	: public FSubsystemCollection<UAtomEngineSubsystem>
{
public:

	template<class InterfaceToCastTo>
	void ForEachSubsystem(TFunctionRef<bool(InterfaceToCastTo*)> InFunction) const
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
		FSubsystemCollection::ForEachSubsystem([Function = MoveTemp(InFunction)](USubsystem* Subsystem)
		{
			if (InterfaceToCastTo* CastedSystem = Cast<InterfaceToCastTo>(Subsystem))
			{
				Function(CastedSystem);
			}
		});
#else
		const TArray<USubsystem*>& AllSubsystems = GetSubsystemArrayInternal(UAtomEngineSubsystem::StaticClass());
		for (USubsystem* Subsystem : AllSubsystems)
		{
			if (InterfaceToCastTo* CastedSystem = Cast<InterfaceToCastTo>(Subsystem))
			{
				InFunction(CastedSystem);
			}
		}
#endif
	}
};
