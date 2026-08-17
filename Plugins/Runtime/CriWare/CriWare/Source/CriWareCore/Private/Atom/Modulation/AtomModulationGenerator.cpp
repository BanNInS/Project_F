
#include "Atom/Modulation/AtomModulationGenerator.h"

#include "Engine/World.h"
#include "Templates/Function.h"

#include "Atom/AtomRuntime.h"
#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationLogs.h"
#include "Atom/Modulation/AtomModulationSystem.h"
#include "Atom/Modulation/AtomModulationGeneratorProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomModulationGenerator)

namespace AtomModulation
{
	void IGenerator::AtomThreadCommand(TUniqueFunction<void()>&& InCommand)
	{
		CommandQueue.Enqueue(MoveTemp(InCommand));
	}

	void IGenerator::PumpCommands()
	{
		TUniqueFunction<void()> Cmd;
		while (!CommandQueue.IsEmpty())
		{
			CommandQueue.Dequeue(Cmd);
			Cmd();
		}
	}
} // namespace

#if WITH_EDITOR
void UAtomModulationGenerator::PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent)
{
	// Guards against slamming the modulation system with changes when using sliders.
	if (InPropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive)
	{
		Atom::IterateOverAllModulationSystems([this](AtomModulation::FAtomModulationSystem& OutModulation)
		{
			OutModulation.UpdateModulator(*this);
		});
	}

	Super::PostEditChangeProperty(InPropertyChangedEvent);
}
#endif // WITH_EDITOR

//TUniquePtr<IAtomProxyData> UAtomModulationGenerator::CreateNewProxyData(const Audio::FProxyDataInitParams& InitParams)
//{
	//using namespace AtomModulation;
	//return MakeUnique<FAtomModulatorAssetProxy>(*this);
//}

TUniquePtr<Atom::IModulatorSettings> UAtomModulationGenerator::CreateProxySettings() const
{
	using namespace AtomModulation;
	return TUniquePtr<Atom::IModulatorSettings>(new FModulationGeneratorSettings(*this));
}

void UAtomModulationGenerator::BeginDestroy()
{
	using namespace AtomModulation;

	if (UWorld* World = GetWorld())
	{
		FAtomRuntimeHandle AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorld(World);
		if (AtomRuntime.IsValid())
		{	
			AtomModulation::FAtomModulationSystem* Modulation = AtomRuntime->GetAtomModulationSystem();
			check(Modulation);
			if (Modulation)
			{	
				PRAGMA_DISABLE_DEPRECATION_WARNINGS
				Modulation->DeactivateGenerator(*this);
				PRAGMA_ENABLE_DEPRECATION_WARNINGS
			}
		}
	}

	Super::BeginDestroy();
}
