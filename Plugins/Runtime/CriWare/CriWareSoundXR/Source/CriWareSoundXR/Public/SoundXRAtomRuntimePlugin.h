

#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Extensions/IAtomRuntimePlugin.h"

/** Class used to connect Sound xR to an arbitrary active sound. */
class CRIWARESOUNDXR_API FSoundXRAtomRuntimePlugin
	: public IAtomRuntimePlugin
{
public:

	FSoundXRAtomRuntimePlugin(FAtomRuntime* OwningAtomRuntime);
	virtual ~FSoundXRAtomRuntimePlugin();

	//~ IAtomRuntimePlugin
	virtual FName GetPluginName() const override { return TEXT("CriWareSoundXR"); }
	virtual void Initialize(const FAtomPluginInitializationParams& InitializationParams) override {}
	virtual void Finalize() override {}
	virtual void ProcessPlugin(const double InElapsed) override {}

	static bool IsSoundXRRegisteredToAtom() { return bIsRegistered; }

private:

	void OnRuntimeWillActivate(FAtomRuntimeId RuntimeID);
	void OnRuntimeDeactivated(FAtomRuntimeId RuntimeID);

	// Atom runtime used with this plugin
	FAtomRuntime* AtomRuntime = nullptr;

	FDelegateHandle OnRuntimeWillActivateHandle;
	FDelegateHandle OnRuntimeDeactivatedHandle;

	static bool bIsRegistered; // only one registration possible overall runtimes
};
