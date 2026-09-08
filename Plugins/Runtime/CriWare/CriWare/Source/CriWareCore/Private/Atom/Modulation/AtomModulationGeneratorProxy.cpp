
#include "AtomModulationGeneratorProxy.h"

#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationSystem.h"

namespace AtomModulation
{
	const FGeneratorId InvalidGeneratorId = INDEX_NONE;

	const Atom::FModulationParameter& FModulationGeneratorSettings::GetOutputParameter() const
	{
		// For now, all generators are normalized values (0, 1).
		return Atom::GetDefaultModulationParameter();
	}

	Atom::FModulatorTypeId FModulationGeneratorSettings::Register(Atom::FModulatorHandleId HandleId, FAtomModulationSystem& InModSystem) const
	{
		return InModSystem.RegisterModulator(HandleId, *this);
	}

	FModulatorGeneratorProxy::FModulatorGeneratorProxy(FModulationGeneratorSettings&& InSettings, FAtomModulationSystem& InModSystem)
		: TModulatorProxyRefType(InSettings.GetName(), InSettings.GetId(), InModSystem)
		, Generator(MoveTemp(InSettings.Generator))
	{
		Generator->Init(InModSystem.GetAtomRuntimeID());
	}

	FModulatorGeneratorProxy& FModulatorGeneratorProxy::operator =(FModulationGeneratorSettings&& InSettings)
	{
		if (ensure(InSettings.Generator.IsValid()))
		{
			Generator->UpdateGenerator(MoveTemp(InSettings.Generator));
		}
		else
		{
			Generator.Reset();
		}

		return *this;
	}
} // namespace