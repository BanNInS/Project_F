
#include "AtomMixerAtomRuntimePlugin.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"

#include "CriWareAtomMixerPrivate.h"

FAtomMixerAtomRuntimePlugin::FAtomMixerAtomRuntimePlugin(FAtomRuntime* OwningAtomRuntime)
	: IAtomRuntimePlugin()
	, AtomRuntime(OwningAtomRuntime)
{
}

void FAtomMixerAtomRuntimePlugin::Initialize(const FAtomPluginInitializationParams& InitializationParams)
{

}

void FAtomMixerAtomRuntimePlugin::Finalize()
{

}

// optional
void FAtomMixerAtomRuntimePlugin::OnInitSource(const uint32 SourceID, const FAtomSource* Source)
{

}

// optional
void FAtomMixerAtomRuntimePlugin::OnReleaseSource(const uint32 SourceID)
{
	
}

void FAtomMixerAtomRuntimePlugin::ProcessPlugin(const double InElapsed)
{
	
}

