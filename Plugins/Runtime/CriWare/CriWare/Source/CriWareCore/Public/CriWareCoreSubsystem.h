/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWareCoreSubsystem.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"

#include "Subsystems/AudioEngineSubsystem.h"
#include "Templates/Function.h"
#include "UObject/Class.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"

#include "CriWareCoreSubsystem.generated.h"

// Forward Definitions
class UCriWare;

UCLASS()
class CRIWARECORE_API UCriWareCoreSubsystem : public UAudioEngineSubsystem
{
	GENERATED_BODY()

public:

	virtual ~UCriWareCoreSubsystem() = default;

	//~ Begin USubsystem Interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

public:

	/** Manualy initialize CriWareCore systems. */
	UFUNCTION(BlueprintCallable, Category = "CriWare")
	static void InitializeCriWare();

	/** Manualy deinitialize CriWareCore systems. */
	UFUNCTION(BlueprintCallable, Category = "CriWare")
	static void DeinitializeCriWare();

	/** Check if initialized. */
	UFUNCTION(BlueprintCallable, Category = "CriWare")
	bool IsInitialized() const;

	/** Registers AtomConfig data to Atom library. */
	UFUNCTION(BlueprintCallable, Category = "Atom", meta = (Tooltip = "Registers AtomConfig data to Atom library."))
	bool SetAtomConfiguration(const UAtomConfig* InAtomConfig);

	/** Unregister current AtomConfig data from Atom library. */
	UFUNCTION(BlueprintCallable, Category = "Atom", meta = (Tooltip = "Unregister current AtomConfig data from Atom library."))
	void ResetAtomConfiguration();

	/** The currently registered AtomConfig. */
	UFUNCTION(BlueprintCallable, Category = "Atom", meta = (Tooltip = "The currently registered AtomConfig."))
	const UAtomConfig* GetAtomConfiguration() const;
};
