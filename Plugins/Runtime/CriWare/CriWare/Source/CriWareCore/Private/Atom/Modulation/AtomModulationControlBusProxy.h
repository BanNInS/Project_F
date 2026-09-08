#pragma once

#include "CoreMinimal.h"

#include "Atom/AtomRuntimeManager.h"
#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationControlBus.h"
#include "Atom/Modulation/AtomModulationParameter.h"
#include "AtomModulationProxy.h"
#include "AtomModulationGeneratorProxy.h"
#include "Atom/Mixer/AtomMixerTrace.h"

namespace AtomModulation
{
	// Forward Declarations
	class FAtomModulationSystem;

	using FBusId = uint32;
	extern const FBusId InvalidBusId;

	struct FControlBusSettings : public TModulatorBase<FBusId>, public Atom::IModulatorSettings
	{
		bool bBypass;
		float DefaultValue;

		TArray<FModulationGeneratorSettings> GeneratorSettings;
		Atom::FModulationMixFunction MixFunction;
		Atom::FModulationParameter OutputParameter;

		FControlBusSettings(const UAtomModulationControlBus& InBus)
			: TModulatorBase<FBusId>(InBus.GetFName(), InBus.GetUniqueID())
			, bBypass(InBus.bBypass)
			, DefaultValue(InBus.GetDefaultNormalizedValue())
			, MixFunction(InBus.GetMixFunction())
			, OutputParameter(InBus.GetOutputParameter())
		{
			for (const UAtomModulationGenerator* Generator : InBus.Generators)
			{
				if (Generator)
				{
					FModulationGeneratorSettings Settings(*Generator);
					GeneratorSettings.Add(MoveTemp(Settings));
				}
			}
		}

		virtual TUniquePtr<Atom::IModulatorSettings> Clone() const override
		{
			return TUniquePtr<Atom::IModulatorSettings>(new FControlBusSettings(*this));
		}

		virtual Atom::FModulatorId GetModulatorId() const override
		{
			return static_cast<Atom::FModulatorId>(GetId());
		}

		virtual const Atom::FModulationParameter& GetOutputParameter() const override
		{
			return OutputParameter;
		}

		virtual Atom::FModulatorTypeId Register(Atom::FModulatorHandleId HandleId, FAtomModulationSystem& InModulation) const override;
	};

	class FControlBusProxy : public TModulatorProxyRefType<FBusId, FControlBusProxy, FControlBusSettings>
	{
	public:
		FControlBusProxy();
		FControlBusProxy(FControlBusSettings&& InSettings, FAtomModulationSystem& InModSystem);

		~FControlBusProxy();

		FControlBusProxy& operator =(FControlBusSettings&& InSettings);

		float GetDefaultValue() const;
		const TArray<FGeneratorHandle>& GetGeneratorHandles() const;
		float GetGeneratorValue() const;
		float GetMixValue() const;
		float GetValue() const;
		FName GetParameterName() const;
		bool IsBypassed() const;
		void MixIn(const float InValue);
		void MixGenerators();
		void Reset();

#if ATOM_PROFILERTRACE_ENABLED
		void OnTraceStarted(FAtomModulationSystem& InModSystem) const;
#endif // ATOM_PROFILERTRACE_ENABLED

	private:
		void Init(FControlBusSettings&& InSettings);
		float Mix(float ValueA) const;

		float DefaultValue;

		// Cached values
		float GeneratorValue;
		float MixValue;

		bool bBypass;

		Atom::FModulationMixFunction MixFunction;
		TArray<FGeneratorHandle> GeneratorHandles;

#if !UE_BUILD_SHIPPING
		FName ParameterName;
#endif // !UE_BUILD_SHIPPING 
	};

	using FBusProxyMap = TMap<FBusId, FControlBusProxy>;
	using FBusHandle = TProxyHandle<FBusId, FControlBusProxy, FControlBusSettings>;
} // namespace