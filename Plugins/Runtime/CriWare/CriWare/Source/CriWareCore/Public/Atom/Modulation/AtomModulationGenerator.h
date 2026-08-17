#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"

#include "Atom/AtomRuntimeManager.h"
#include "AtomModulation.h"

#include "AtomModulationGenerator.generated.h"

#define CRI_API CRIWARECORE_API

namespace AtomModulation
{
	// Forward Defintions
	class IGenerator;
	using FGeneratorPtr = TUniquePtr<IGenerator>;

	// Hacky base class to avoid 8 bytes of padding after the vtable
	class IGeneratorFixLayout
	{
	public:
		virtual ~IGeneratorFixLayout() = default;
	};

	class IGenerator : public IGeneratorFixLayout
	{
	public:

		virtual ~IGenerator() = default;

		/** Pumps commands from Audio Thread to the generator's modulation processing thread.*/
		void PumpCommands();

		// Clone the generator
		virtual FGeneratorPtr Clone() const = 0;

		/** Allows child generator class to override default copy/update behavior when receiving an updated generator call
			* from the Atom thread. Useful for ignoring updates while a generator is running or deferring the transition
			* to the new generator state to the modulation processing thread. This enables interpolating between existing
			* and new generator state, properties, avoiding discontinuities, etc.
			* @param InGenerator - The constructed version of the generator being sent from the Atom Thread
			*/
		virtual void UpdateGenerator(FGeneratorPtr&& InGenerator) = 0;

		/** Returns current value of the generator. */
		virtual float GetValue() const = 0;

		/** (Optional) Initializer step where the generator is provided the associated parent Atom runtime's Id. */
		virtual void Init(FAtomRuntimeId InRuntimeID) {}

		/** Returns whether or not the generator is bypassed. */
		virtual bool IsBypassed() const = 0;

		/** Updates the generators value at the audio block rate on the modulation processing thread. */
		virtual void Update(double InElapsed) = 0;

#if !UE_BUILD_SHIPPING
		virtual void GetDebugValues(TArray<FString>& OutDebugValues) const = 0;

		virtual void GetDebugCategories(TArray<FString>& OutDebugCategories) const = 0;

		// Required for instance look-up in factory registration
		virtual const FString& GetDebugName() const = 0;
#endif // !UE_BUILD_SHIPPING

	protected:
		CRI_API void AtomThreadCommand(TUniqueFunction<void()>&& InCommand);

		FAtomRuntimeId AtomRuntimeID = INDEX_NONE;

	private:
		TQueue<TUniqueFunction<void()>> CommandQueue;
	};
} // namespace

/**
 * Base class for modulators that algorithmically generate values that can effect
 * various endpoints (ex. Aisac Controler & Parameter Destinations)
 */
UCLASS(MinimalAPI, HideCategories = Object, abstract)
class UAtomModulationGenerator
	: public UAtomModulatorBase
{
	GENERATED_BODY()

public:

	/** Create and return pointer to new instance of generator to be processed on the AudioRenderThread. */
	virtual AtomModulation::FGeneratorPtr CreateInstance() const { return nullptr; }


	//~ Begin Implementation */
	//CRI_API virtual TUniquePtr<Atom::IProxyData> CreateNewProxyData(const Atom::FProxyDataInitParams& InitParams) override;
	CRI_API virtual TUniquePtr<Atom::IModulatorSettings> CreateProxySettings() const override;

	CRI_API virtual float GetModulationValue() { return 0.0f; }

#if WITH_EDITOR
	CRI_API virtual void PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) override;
#endif // WITH_EDITOR

	CRI_API virtual void BeginDestroy() override;
};

#undef CRI_API
