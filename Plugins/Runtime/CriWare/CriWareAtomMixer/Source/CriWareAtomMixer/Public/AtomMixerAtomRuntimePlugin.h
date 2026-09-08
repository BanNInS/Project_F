
#pragma once

#include "CoreMinimal.h"

#include "Extensions/IAtomRuntimePlugin.h"

// Forward Definitions
class FAtomRuntime;
class FAtomSource;

class CRIWAREATOMMIXER_API FAtomMixerAtomRuntimePlugin
	: public IAtomRuntimePlugin
{
public:

	FAtomMixerAtomRuntimePlugin(FAtomRuntime* OwningAtomRuntime);
	virtual ~FAtomMixerAtomRuntimePlugin() {}

	//~ IAtomRuntimePlugin
	virtual FName GetPluginName() const override { return TEXT("CriWareAtomMixer"); }
	virtual void Initialize(const FAtomPluginInitializationParams& InitializationParams) override;
	virtual void Finalize() override;
	virtual void OnInitSource(const uint32 SourceID, const FAtomSource* Source) override;
	virtual void OnReleaseSource(const uint32 SourceID) override;
	virtual void ProcessPlugin(const double InElapsed) override;

private:

	// Atom runtime used with this plugin
	FAtomRuntime* AtomRuntime = nullptr;
};
