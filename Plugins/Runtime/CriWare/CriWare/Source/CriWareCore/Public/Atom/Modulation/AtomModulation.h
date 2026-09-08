#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"

#include "CriWareDefines.h"

#include "AtomModulation.generated.h"

// Cycle stats for Atom modulation system
DECLARE_STATS_GROUP(TEXT("AtomModulation"), STATGROUP_AtomModulation, STATCAT_Advanced);

// Tracks the time for the full render block 
DECLARE_CYCLE_STAT_EXTERN(TEXT("Process Modulators"), STAT_AtomModulationProcessModulators, STATGROUP_AtomModulation, CRIWARECORE_API);

// Forward Declarations
namespace AtomModulation
{
	class FAtomModulationSystem;
}

namespace Atom
{
	using FModulatorId = uint32;
	using FModulatorTypeId = uint32;
	using FModulatorHandleId = uint32;

	using FModulationUnitConversionFunction = TFunction<void(float& /* OutValueNormalizedToUnit */)>;
	using FModulationNormalizedConversionFunction = TFunction<void(float& /* OutValueUnitToNormalized */)>;
	using FModulationMixFunction = TFunction<void(float& /* OutNormalizedA */, float /* InNormalizedB */)>;

	struct FModulationParameter
	{
		CRIWARECORE_API FModulationParameter();
		CRIWARECORE_API FModulationParameter(const FModulationParameter& InParam);
		CRIWARECORE_API FModulationParameter(FModulationParameter&& InParam);

		CRIWARECORE_API FModulationParameter& operator=(FModulationParameter&& InParam);
		CRIWARECORE_API FModulationParameter& operator=(const FModulationParameter& InParam);

		FName ParameterName;

		// Default value of parameter in unit space
		float DefaultValue = 1.0f;

		// Default minimum value of parameter in unit space
		float MinValue = 0.0f;

		// Default minimum value of parameter in unit space
		float MaxValue = 1.0f;

		// Whether or not unit conversion is required
		bool bRequiresConversion = false;

		uint32 TypeHash = INDEX_NONE;

#if WITH_EDITORONLY_DATA
		FText UnitDisplayName;

		FName ClassName;
#endif // WITH_EDITORONLY_DATA

		// Function used to mix normalized values together.
		Atom::FModulationMixFunction MixFunction;

		// Function used to convert value buffer from normalized, unitless space [0.0f, 1.0f] to unit space.
		Atom::FModulationUnitConversionFunction UnitFunction;

		// Function used to convert value buffer from unit space to normalized, unitless [0.0f, 1.0f] space.
		Atom::FModulationNormalizedConversionFunction NormalizedFunction;

		static CRIWARECORE_API const Atom::FModulationMixFunction& GetDefaultMixFunction();
		static CRIWARECORE_API const Atom::FModulationUnitConversionFunction& GetDefaultUnitConversionFunction();
		static CRIWARECORE_API const Atom::FModulationNormalizedConversionFunction& GetDefaultNormalizedConversionFunction();
	
		friend FORCEINLINE uint32 GetTypeHash(const FModulationParameter& InModulationParameter)
		{
			return InModulationParameter.TypeHash;
		}
	};

	CRIWARECORE_API bool IsModulationParameterRegistered(FName InName);
	CRIWARECORE_API void RegisterModulationParameter(FName InName, FModulationParameter&& InParameter);
	CRIWARECORE_API bool UnregisterModulationParameter(FName InName);
	CRIWARECORE_API void UnregisterAllModulationParameters();
	CRIWARECORE_API const FModulationParameter* GetModulationParameterPtr(FName InName);
	CRIWARECORE_API const FModulationParameter& GetDefaultModulationParameter();
	CRIWARECORE_API void IterateOverAllModulationSystems(TFunctionRef<void(AtomModulation::FAtomModulationSystem&)> InFunction);

	/** Interface for cached off Modulator UObject data used as default settings to
	  * be converted to instanced proxy data per Atom runtime on the AtomRenderThread.
	  * If proxy is already active, implementation is expected to ignore register call
	  * and return existing modulator proxy's type Id & set parameter accordingly.
	  */
	class IModulatorSettings
	{
	public:
		virtual ~IModulatorSettings() = default;
		virtual TUniquePtr<IModulatorSettings> Clone() const = 0;
		virtual FModulatorId GetModulatorId() const = 0;
		virtual const Atom::FModulationParameter& GetOutputParameter() const = 0;
		virtual Atom::FModulatorTypeId Register(
			Atom::FModulatorHandleId HandleId,
			AtomModulation::FAtomModulationSystem& InModulation) const = 0;
	};

	/** Handle to a modulator which interacts with the modulation API to manage lifetime
	  * of modulator proxy objects internal to modulation plugin implementation.
	  */
	struct FModulatorHandle
	{
		FModulatorHandle() = default;
		CRIWARECORE_API FModulatorHandle(FModulationParameter&& InParameter);
		CRIWARECORE_API FModulatorHandle(AtomModulation::FAtomModulationSystem& InModulation, const IModulatorSettings& InModulatorSettings, FModulationParameter&& InParameter);
		CRIWARECORE_API FModulatorHandle(const FModulatorHandle& InOther);
		CRIWARECORE_API FModulatorHandle(FModulatorHandle&& InOther);

		CRIWARECORE_API ~FModulatorHandle();

		CRIWARECORE_API FModulatorHandle& operator=(const FModulatorHandle& InOther);
		CRIWARECORE_API FModulatorHandle& operator=(FModulatorHandle&& InOther);

		CRIWARECORE_API FModulatorId GetModulatorId() const;
		CRIWARECORE_API const FModulationParameter& GetParameter() const;
		CRIWARECORE_API FModulatorTypeId GetTypeId() const;
		CRIWARECORE_API FModulatorHandleId GetHandleId() const;
		CRIWARECORE_API bool GetValue(float& OutValue) const;
		CRIWARECORE_API bool GetValueThreadSafe(float& OutValue) const;
		CRIWARECORE_API bool IsValid() const;

		friend FORCEINLINE uint32 GetTypeHash(const FModulatorHandle& InModulatorHandle)
		{
			return HashCombineFast(InModulatorHandle.HandleId, InModulatorHandle.ModulatorId);
		}

		FORCEINLINE bool operator==(const FModulatorHandle& Other) const
		{
			return HandleId == Other.HandleId && ModulatorId == Other.ModulatorId;
		}

		FORCEINLINE bool operator!=(const FModulatorHandle& Other) const
		{
			return !(*this == Other);
		}

	private:
		FModulationParameter Parameter;
		FModulatorHandleId HandleId = INDEX_NONE;
		FModulatorTypeId ModulatorTypeId = INDEX_NONE;
		FModulatorId ModulatorId = INDEX_NONE;
		TWeakPtr<AtomModulation::FAtomModulationSystem> Modulation;
	};
}

/**
 * Base class for all modulators used with ADX Atom.
 */
UCLASS(Config = CriWare, abstract, EditInLineNew, BlueprintType, MinimalAPI)
class UAtomModulatorBase
	: public UObject
{
	GENERATED_BODY()

public:

	CRIWARECORE_API virtual const Atom::FModulationParameter& GetOutputParameter() const;

	//CRIWARECORE_API virtual TUniquePtr<Atom::IProxyData> CreateNewProxyData(const Atom::FProxyDataInitParams& InitParams) override;

	CRIWARECORE_API virtual TUniquePtr<Atom::IModulatorSettings> CreateProxySettings() const;
};
