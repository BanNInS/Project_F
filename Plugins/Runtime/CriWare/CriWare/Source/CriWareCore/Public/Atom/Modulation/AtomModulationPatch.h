
#pragma once

#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ObjectSaveContext.h"
#include "WaveTableTransform.h"

#include "Extensions/IAtomProxyInitializer.h"
#include "AtomModulationParameter.h"

#include "AtomModulationPatch.generated.h"

#define CRI_API CRIWARECORE_API

// Forward Declarations
class UAtomModulationControlBus;

PRAGMA_DISABLE_DEPRECATION_WARNINGS
USTRUCT(BlueprintType)
struct FAtomModulationTransform : public FWaveTableTransform
{
	GENERATED_BODY()
};
PRAGMA_ENABLE_DEPRECATION_WARNINGS

USTRUCT(BlueprintType)
struct FAtomControlModulationInput
{
	GENERATED_BODY()

public:

	CRI_API FAtomControlModulationInput();

	/** Get the modulated input value on parent patch initialization and hold that value for its lifetime */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (DisplayName = "Sample-And-Hold", DisplayPriority = "30"))
	uint8 bSampleAndHold : 1;

	/** Transform to apply to the input prior to mix phase */
	UPROPERTY(EditAnywhere, Category = Input, meta = (DisplayPriority = "20"))
	FAtomModulationTransform Transform;

	/** The input bus */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input, meta = (DisplayPriority = "10"))
	TObjectPtr<UAtomModulationControlBus> Bus = nullptr;

	CRI_API const UAtomModulationControlBus* GetBus() const;
	CRI_API const UAtomModulationControlBus& GetBusChecked() const;
};

USTRUCT(BlueprintType)
struct FAtomControlModulationPatch
{
	GENERATED_BODY()

public:

	/** Whether or not patch is bypassed (patch is still active, but always returns output parameter default value when modulated) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inputs)
	bool bBypass = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Output, meta = (DisplayName = "Parameter"))
	TObjectPtr<UAtomModulationParameter> OutputParameter = nullptr;

	/** Modulation inputs */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inputs)
	TArray<FAtomControlModulationInput> Inputs;
};

UCLASS(MinimalAPI, config = Engine, editinlinenew, BlueprintType)
class UAtomModulationPatch : public UAtomModulatorBase
{
	GENERATED_BODY()

public:

	UAtomModulationPatch(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Modulation, meta = (ShowOnlyInnerProperties))
	FAtomControlModulationPatch PatchSettings;

	/* UAtomModulatorBase Implementation */
	//CRI_API virtual TSharedPtr<Atom::IProxyData> CreateProxyData(const Atom::FProxyDataInitParams& InitParams) override;
	CRI_API virtual const Atom::FModulationParameter& GetOutputParameter() const override;

	CRI_API virtual TUniquePtr<Atom::IModulatorSettings> CreateProxySettings() const override;

#if WITH_EDITORONLY_DATA
	CRI_API virtual void Serialize(FArchive& Ar) override;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	CRI_API virtual void PreSave(FObjectPreSaveContext InSaveContext) override;
	CRI_API virtual void PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) override;
	CRI_API virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& InPropertyChangedEvent) override;
#endif // WITH_EDITOR
};

#undef CRI_API
