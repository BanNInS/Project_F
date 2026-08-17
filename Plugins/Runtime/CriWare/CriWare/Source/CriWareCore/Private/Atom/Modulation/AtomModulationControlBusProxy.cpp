
#include "AtomModulationControlBusProxy.h"

#include "Engine/World.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationLogs.h"
#include "Atom/Modulation/AtomModulationSystem.h"
#include "Atom/Modulation/AtomModulationGenerator.h"

#if ATOM_PROFILERTRACE_ENABLED
UE_TRACE_EVENT_BEGIN(CriWareAtom, ControlBusActivate)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, ControlBusID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(UE::Trace::WideString, Name)
UE_TRACE_EVENT_FIELD(UE::Trace::WideString, ParamName)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, ControlBusDeactivate)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, ControlBusID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, GeneratorRegisterBus)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, SourceID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(uint32, ModulatingSourceID)
UE_TRACE_EVENT_FIELD(UE::Trace::WideString, BusName)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(CriWareAtom, GeneratorActivate)
UE_TRACE_EVENT_FIELD(uint32, RuntimeID)
UE_TRACE_EVENT_FIELD(uint32, SourceID)
UE_TRACE_EVENT_FIELD(double, Timestamp)
UE_TRACE_EVENT_FIELD(UE::Trace::WideString, Name)
UE_TRACE_EVENT_END()
#endif // ATOM_PROFILERTRACE_ENABLED

namespace AtomModulation
{
	const FBusId InvalidBusId = INDEX_NONE;

	Atom::FModulatorTypeId FControlBusSettings::Register(Atom::FModulatorHandleId HandleId, FAtomModulationSystem& InModulation) const
	{
#if ATOM_PROFILERTRACE_ENABLED
		for (const FModulationGeneratorSettings& GeneratorSetting : GeneratorSettings)
		{
			UE_TRACE_LOG(CriWareAtom, GeneratorRegisterBus, AtomChannel)
				<< GeneratorRegisterBus.RuntimeID(InModulation.GetAtomRuntimeID())
				<< GeneratorRegisterBus.SourceID(GetId())
				<< GeneratorRegisterBus.Timestamp(FPlatformTime::Cycles64())
				<< GeneratorRegisterBus.ModulatingSourceID(GeneratorSetting.GetId())
				<< GeneratorRegisterBus.BusName(*(GetName().ToString()));

			UE_TRACE_LOG(CriWareAtom, GeneratorActivate, AtomChannel)
				<< GeneratorActivate.RuntimeID(InModulation.GetAtomRuntimeID())
				<< GeneratorActivate.SourceID(GeneratorSetting.GetId())
				<< GeneratorActivate.Timestamp(FPlatformTime::Cycles64())
				<< GeneratorActivate.Name(*GeneratorSetting.GetName().ToString());
		}
#endif

		return InModulation.RegisterModulator(HandleId, *this);
	}

	FControlBusProxy::FControlBusProxy()
		: DefaultValue(0.0f)
		, GeneratorValue(1.0f)
		, MixValue(NAN)
		, bBypass(false)
	{
	}

	FControlBusProxy::FControlBusProxy(FControlBusSettings&& InSettings, FAtomModulationSystem& InModSystem)
		: TModulatorProxyRefType(InSettings.GetName(), InSettings.GetId(), InModSystem)
	{
		Init(MoveTemp(InSettings));
	}

	FControlBusProxy::~FControlBusProxy()
	{
#if ATOM_PROFILERTRACE_ENABLED
		if (ModSystem)
		{
			UE_TRACE_LOG(CriWareAtom, ControlBusDeactivate, AtomChannel)
				<< ControlBusDeactivate.RuntimeID(static_cast<uint32>(ModSystem->AtomRuntimeID))
				<< ControlBusDeactivate.ControlBusID(static_cast<uint32>(GetId()))
				<< ControlBusDeactivate.Timestamp(FPlatformTime::Cycles64());
		}
#endif // ATOM_PROFILERTRACE_ENABLED
	}

	FControlBusProxy& FControlBusProxy::operator =(FControlBusSettings&& InSettings)
	{
		Init(MoveTemp(InSettings));
		return *this;
	}

	float FControlBusProxy::GetDefaultValue() const
	{
		return DefaultValue;
	}

	const TArray<FGeneratorHandle>& FControlBusProxy::GetGeneratorHandles() const
	{
		return GeneratorHandles;
	}

	float FControlBusProxy::GetGeneratorValue() const
	{
		return GeneratorValue;
	}

	float FControlBusProxy::GetMixValue() const
	{
		return MixValue;
	}

	float FControlBusProxy::GetValue() const
	{
		const float DefaultMixed = Mix(DefaultValue);
		return FMath::Clamp(DefaultMixed * GeneratorValue, 0.0f, 1.0f);
	}

	FName FControlBusProxy::GetParameterName() const
	{
#if UE_BUILD_SHIPPING
		static FName ParameterName;
#endif // !UE_BUILD_SHIPPING

		return ParameterName;
	}

	void FControlBusProxy::Init(FControlBusSettings&& InSettings)
	{
		check(ModSystem);

		GeneratorValue = 1.0f;
		MixValue = NAN;
		MixFunction = MoveTemp(InSettings.MixFunction);

#if !UE_BUILD_SHIPPING
		ParameterName = InSettings.OutputParameter.ParameterName;
#endif // !UE_BUILD_SHIPPING 

		DefaultValue = FMath::Clamp(InSettings.DefaultValue, 0.0f, 1.0f);
		bBypass = InSettings.bBypass;

		TArray<FGeneratorHandle> NewHandles;
		for (FModulationGeneratorSettings& GeneratorSettings : InSettings.GeneratorSettings)
		{
			NewHandles.Add(FGeneratorHandle::Create(MoveTemp(GeneratorSettings), ModSystem->RefProxies.Generators, *ModSystem));
		}

		// Move vs. reset and adding to original array to avoid potentially clearing handles (and thus current Generator state)
		// and destroying generators if function is called while reinitializing/updating the modulator
		GeneratorHandles = MoveTemp(NewHandles);

#if ATOM_PROFILERTRACE_ENABLED
		UE_TRACE_LOG(CriWareAtom, ControlBusActivate, AtomChannel)
			<< ControlBusActivate.RuntimeID(ModSystem->GetAtomRuntimeID())
			<< ControlBusActivate.ControlBusID(InSettings.GetId())
			<< ControlBusActivate.Timestamp(FPlatformTime::Cycles64())
			<< ControlBusActivate.Name(*(InSettings.GetName().ToString()))
			<< ControlBusActivate.ParamName(*(GetParameterName().ToString()));
#endif // ATOM_PROFILERTRACE_ENABLED
	}

	bool FControlBusProxy::IsBypassed() const
	{
		return bBypass;
	}

	float FControlBusProxy::Mix(float ValueA) const
	{
		// If mix value is NaN, it is uninitialized (effectively, the parent bus is inactive)
		// and therefore not mixable, so just return the second value.
		if (FMath::IsNaN(MixValue))
		{
			return ValueA;
		}

		float OutValue = MixValue;
		MixFunction(OutValue, ValueA);
		return OutValue;
	}

#if ATOM_PROFILERTRACE_ENABLED
	void FControlBusProxy::OnTraceStarted(FAtomModulationSystem& InModSystem) const
	{
		UE_TRACE_LOG(CriWareAtom, ControlBusActivate, AtomChannel)
			<< ControlBusActivate.RuntimeID(InModSystem.GetAtomRuntimeID())
			<< ControlBusActivate.ControlBusID(GetId())
			<< ControlBusActivate.Timestamp(FPlatformTime::Cycles64())
			<< ControlBusActivate.Name(*(GetName().ToString()))
			<< ControlBusActivate.ParamName(*(GetParameterName().ToString()));

		for (const FGeneratorHandle& GeneratorHandle : GeneratorHandles)
		{
			UE_TRACE_LOG(CriWareAtom, GeneratorRegisterBus, AtomChannel)
				<< GeneratorRegisterBus.RuntimeID(InModSystem.GetAtomRuntimeID())
				<< GeneratorRegisterBus.SourceID(GetId())
				<< GeneratorRegisterBus.Timestamp(FPlatformTime::Cycles64())
				<< GeneratorRegisterBus.ModulatingSourceID(GeneratorHandle.GetId())
				<< GeneratorRegisterBus.BusName(*(GetName().ToString()));

			UE_TRACE_LOG(CriWareAtom, GeneratorActivate, AtomChannel)
				<< GeneratorActivate.RuntimeID(InModSystem.GetAtomRuntimeID())
				<< GeneratorActivate.SourceID(GeneratorHandle.GetId())
				<< GeneratorActivate.Timestamp(FPlatformTime::Cycles64())
				<< GeneratorActivate.Name(*(GeneratorHandle.FindProxy().GetName().ToString()));
		}
	}
#endif // ATOM_PROFILERTRACE_ENABLED

	void FControlBusProxy::MixIn(const float InValue)
	{
		MixValue = Mix(InValue);
	}

	void FControlBusProxy::MixGenerators()
	{
		for (const FGeneratorHandle& Handle : GeneratorHandles)
		{
			if (Handle.IsValid())
			{
				const FModulatorGeneratorProxy& GeneratorProxy = Handle.FindProxy();
				if (!GeneratorProxy.IsBypassed())
				{
					GeneratorValue *= GeneratorProxy.GetValue();
				}
			}
		}
	}

	void FControlBusProxy::Reset()
	{
		GeneratorValue = 1.0f;
		MixValue = NAN;
	}
} // namespace
