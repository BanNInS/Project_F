
#pragma once

#include "Algo/Transform.h"
#include "Templates/Function.h"

#include "Atom/AtomRuntimeManager.h"

#include "Atom/Modulation/AtomModulation.h"
#include "Atom/Modulation/AtomModulationParameter.h"
#include "Atom/Modulation/AtomModulationPatch.h"
#include "AtomModulationControlBusProxy.h"
#include "AtomModulationProxy.h"

namespace AtomModulation
{
	// Forward Declarations
	class FAtomModulationSystem;

	using FPatchId = uint32;
	extern const FPatchId InvalidPatchId;

	struct FModulationInputSettings
	{
		FControlBusSettings BusSettings;
		FAtomModulationTransform Transform;
		uint8 bSampleAndHold : 1;

		FModulationInputSettings(const FAtomControlModulationInput& InInput)
			: BusSettings(InInput.GetBusChecked())
			, Transform(InInput.Transform)
			, bSampleAndHold(InInput.bSampleAndHold)
		{
		}

		FModulationInputSettings(FAtomControlModulationInput&& InInput)
			: BusSettings(InInput.GetBusChecked())
			, Transform(MoveTemp(InInput.Transform))
			, bSampleAndHold(InInput.bSampleAndHold)
		{
		}
	};

	/** Modulation input instance */
	class FModulationInputProxy
	{
	public:
		FModulationInputProxy() = default;
		FModulationInputProxy(FModulationInputSettings&& InSettings, FAtomModulationSystem& OutModSystem);

		FBusHandle BusHandle;

		FAtomModulationTransform Transform;
		bool bSampleAndHold = false;
	};

	/** Patch applied as the final stage of a modulation chain prior to output on the sound level (Always active, never removed) */
	struct FModulationOutputProxy
	{
		FModulationOutputProxy() = default;
		FModulationOutputProxy(float InDefaultValue, const Atom::FModulationMixFunction& InMixFunction);

		/** Whether patch has been initialized or not */
		bool bInitialized = false;

		/** Cached value of sample-and-hold input values */
		float SampleAndHoldValue = 1.0f;

		/** Function used to mix values together */
		Atom::FModulationMixFunction MixFunction;

		/** Default value if no inputs are provided */
		float DefaultValue = 1.0f;
	};

	struct FModulationPatchSettings : public TModulatorBase<FPatchId>, public Atom::IModulatorSettings
	{
		TArray<FModulationInputSettings> InputSettings;
		bool bBypass = true;

		Atom::FModulationParameter OutputParameter;

		FModulationPatchSettings() = default;
		FModulationPatchSettings(const FModulationPatchSettings& InPatchSettings) = default;

		FModulationPatchSettings(const UAtomModulationPatch& InPatch)
			: TModulatorBase<FPatchId>(InPatch.GetFName(), InPatch.GetUniqueID())
			, bBypass(InPatch.PatchSettings.bBypass)
			, OutputParameter(InPatch.GetOutputParameter())
		{
			for (const FAtomControlModulationInput& Input : InPatch.PatchSettings.Inputs)
			{
				if (Input.Bus)
				{
					FModulationInputSettings NewInputSettings(Input);

					// Required to avoid referencing external UObject from settings on non-audio/game threads
					NewInputSettings.Transform.CacheCurve();

					InputSettings.Add(MoveTemp(NewInputSettings));
				}
			}
		}

		virtual TUniquePtr<IModulatorSettings> Clone() const override
		{
			return TUniquePtr<IModulatorSettings>(new FModulationPatchSettings(*this));
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

	class FModulationPatchProxy
	{
	public:
		FModulationPatchProxy() = default;
		FModulationPatchProxy(FModulationPatchSettings&& InSettings, FAtomModulationSystem& InModSystem);

		/** Whether or not the patch is bypassed (effectively just returning the default value) */
		bool IsBypassed() const;

		/** Returns the value of the patch */
		float GetValue() const;

		/** Updates the patch value */
		void Update();

	protected:
		void Init(FModulationPatchSettings&& InSettings, FAtomModulationSystem& InModSystem);

	private:
		/** Default value of patch */
		float DefaultValue = 1.0f;

		/** Current value of the patch */
		float Value = 1.0f;

		/** Optional modulation inputs */
		TArray<FModulationInputProxy> InputProxies;

		/** Final output modulation post input combination */
		FModulationOutputProxy OutputProxy;

		/** Bypasses the patch and doesn't update modulation value */
		bool bBypass = true;

		friend class FAtomModulationSystem;
	};

	class FModulationPatchRefProxy : public TModulatorProxyRefType<FPatchId, FModulationPatchRefProxy, FModulationPatchSettings>, public FModulationPatchProxy
	{
	public:
		FModulationPatchRefProxy();
		FModulationPatchRefProxy(FModulationPatchSettings&& InSettings, FAtomModulationSystem& OutModSystem);

		FModulationPatchRefProxy& operator=(FModulationPatchSettings&& InSettings);
	};

	using FPatchProxyMap = TMap<FPatchId, FModulationPatchRefProxy>;
	using FPatchHandle = TProxyHandle<FPatchId, FModulationPatchRefProxy, FModulationPatchSettings>;
} // namespace
