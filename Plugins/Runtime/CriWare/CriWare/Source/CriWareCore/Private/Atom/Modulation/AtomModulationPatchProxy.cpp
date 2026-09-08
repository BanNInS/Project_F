
#include "AtomModulationPatchProxy.h"

#include "Atom/AtomRuntime.h"
#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationStatics.h"
#include "Atom/Modulation/AtomModulationSystem.h"
#include "Atom/Modulation/AtomModulationControlBusProxy.h"
#include "Atom/Modulation/AtomModulationParameter.h"
#include "AtomModulationProxy.h"

namespace AtomModulation
{
	const FPatchId InvalidPatchId = INDEX_NONE;

	Atom::FModulatorTypeId FModulationPatchSettings::Register(Atom::FModulatorHandleId HandleId, FAtomModulationSystem& InModSystem) const
	{
		return InModSystem.RegisterModulator(HandleId, *this);
	}

	FModulationInputProxy::FModulationInputProxy(FModulationInputSettings&& InSettings, FAtomModulationSystem& OutModSystem)
		: BusHandle(FBusHandle::Create(MoveTemp(InSettings.BusSettings), OutModSystem.RefProxies.Buses, OutModSystem))
		, Transform(MoveTemp(InSettings.Transform))
		, bSampleAndHold(InSettings.bSampleAndHold)
	{
	}

	FModulationOutputProxy::FModulationOutputProxy(float InDefaultValue, const Audio::FModulationMixFunction& InMixFunction)
		: MixFunction(InMixFunction)
		, DefaultValue(InDefaultValue)
	{
	}

	FModulationPatchProxy::FModulationPatchProxy(FModulationPatchSettings&& InSettings, FAtomModulationSystem& OutModSystem)
	{
		Init(MoveTemp(InSettings), OutModSystem);
	}

	void FModulationPatchProxy::Init(FModulationPatchSettings&& InSettings, FAtomModulationSystem& OutModSystem)
	{
		bBypass = InSettings.bBypass;
		DefaultValue = InSettings.OutputParameter.DefaultValue;
		if (InSettings.OutputParameter.bRequiresConversion)
		{
			InSettings.OutputParameter.NormalizedFunction(DefaultValue);
		}

		// Cache existing proxies to avoid releasing bus state (and potentially referenced bus state) when reinitializing
		const TArray<FModulationInputProxy> CachedProxies = InputProxies;

		InputProxies.Reset();
		for (FModulationInputSettings& Input : InSettings.InputSettings)
		{
			InputProxies.Emplace(MoveTemp(Input), OutModSystem);
		}

		OutputProxy = FModulationOutputProxy(InSettings.OutputParameter.DefaultValue, MoveTemp(InSettings.OutputParameter.MixFunction));
	}

	bool FModulationPatchProxy::IsBypassed() const
	{
		return bBypass;
	}

	float FModulationPatchProxy::GetValue() const
	{
		if (bBypass)
		{
			return OutputProxy.DefaultValue;
		}

		return Value;
	}

	void FModulationPatchProxy::Update()
	{
		Value = DefaultValue;

		float& OutSampleHold = OutputProxy.SampleAndHoldValue;
		if (!OutputProxy.bInitialized)
		{
			OutSampleHold = DefaultValue;
			OutputProxy.bInitialized = true;
		}

		for (const FModulationInputProxy& Input : InputProxies)
		{
			if (Input.bSampleAndHold)
			{
				if (!OutputProxy.bInitialized && Input.BusHandle.IsValid())
				{
					const FControlBusProxy& BusProxy = Input.BusHandle.FindProxy();
					if (!BusProxy.IsBypassed())
					{
						float ModStageValue = BusProxy.GetValue();
						Input.Transform.Apply(ModStageValue);
						OutputProxy.MixFunction(OutSampleHold, ModStageValue);
					}
				}
			}
			else
			{
				if (Input.BusHandle.IsValid())
				{
					const FControlBusProxy& BusProxy = Input.BusHandle.FindProxy();
					if (!BusProxy.IsBypassed())
					{
						float ModStageValue = BusProxy.GetValue();
						Input.Transform.Apply(ModStageValue);
						OutputProxy.MixFunction(Value, ModStageValue);
					}
				}
			}
		}

		OutputProxy.MixFunction(Value, OutSampleHold);
	}

	FModulationPatchRefProxy::FModulationPatchRefProxy()
		: TModulatorProxyRefType()
		, FModulationPatchProxy()
	{
	}

	FModulationPatchRefProxy::FModulationPatchRefProxy(FModulationPatchSettings&& InSettings, FAtomModulationSystem& OutModSystem)
		: TModulatorProxyRefType(InSettings.GetName(), InSettings.GetId(), OutModSystem)
		, FModulationPatchProxy(MoveTemp(InSettings), OutModSystem)
	{
	}

	FModulationPatchRefProxy& FModulationPatchRefProxy::operator=(FModulationPatchSettings&& InSettings)
	{
		check(ModSystem);
		Init(MoveTemp(InSettings), *ModSystem);
		return *this;
	}
} // namespace
