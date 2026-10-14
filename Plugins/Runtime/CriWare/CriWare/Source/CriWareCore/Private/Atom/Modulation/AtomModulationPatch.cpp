
#include "Atom/Modulation/AtomModulationPatch.h"

#include "Atom/AtomRuntime.h"
#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationStatics.h"
#include "Atom/Modulation/AtomModulationSystem.h"
#include "Atom/Modulation/AtomModulationControlBus.h"
#include "Atom/Modulation/AtomModulationParameter.h"
#include "Atom/Modulation/AtomModulationPatchProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomModulationPatch)

#define LOCTEXT_NAMESPACE "AtomModulationPatch"

UAtomModulationPatch::UAtomModulationPatch(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

/*TSharedPtr<Atom::IProxyData> UAtomModulationPatch::CreateProxyData(const Atom::FProxyDataInitParams& InitParams)
{
	using namespace AtomModulation;
	return MakeShared<FSoundModulatorAssetProxy>(*this);
}*/

TUniquePtr<Atom::IModulatorSettings> UAtomModulationPatch::CreateProxySettings() const
{
	using namespace AtomModulation;
	return TUniquePtr<Atom::IModulatorSettings>(new FModulationPatchSettings(*this));
}

const Atom::FModulationParameter& UAtomModulationPatch::GetOutputParameter() const
{
	return AtomModulation::GetOrRegisterParameter(PatchSettings.OutputParameter, GetName(), GetClass()->GetName());
}

#if WITH_EDITOR
void UAtomModulationPatch::PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent)
{
	Atom::IterateOverAllModulationSystems([this](AtomModulation::FAtomModulationSystem& OutModSystem)
	{
		OutModSystem.UpdateModulator(*this);
	});

	Super::PostEditChangeProperty(InPropertyChangedEvent);
}

void UAtomModulationPatch::PostEditChangeChainProperty(FPropertyChangedChainEvent& InPropertyChangedEvent)
{
	Atom::IterateOverAllModulationSystems([this](AtomModulation::FAtomModulationSystem& OutModSystem)
	{
		OutModSystem.UpdateModulator(*this);
	});

	Super::PostEditChangeChainProperty(InPropertyChangedEvent);
}

#if WITH_EDITORONLY_DATA
void UAtomModulationPatch::Serialize(FArchive& Ar)
{
	if (Ar.IsLoading())
	{
		for (FAtomControlModulationInput& Input : PatchSettings.Inputs)
		{
			Input.Transform.VersionTableData();
		}
	}

	Super::Serialize(Ar);
}
#endif // WITH_EDITORONLY_DATA

void UAtomModulationPatch::PreSave(FObjectPreSaveContext InSaveContext)
{
	Super::PreSave(InSaveContext);
}
#endif // WITH_EDITOR

FAtomControlModulationInput::FAtomControlModulationInput()
	: bSampleAndHold(0)
{
}

const UAtomModulationControlBus* FAtomControlModulationInput::GetBus() const
{
	return Bus;
}

const UAtomModulationControlBus& FAtomControlModulationInput::GetBusChecked() const
{
	check(Bus);
	return *Bus;
}

#undef LOCTEXT_NAMESPACE // AtomModulationPatch
