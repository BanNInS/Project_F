#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ScriptMacros.h"

#include "AtomModulation.h"
#include "AtomModulationParameter.h"
#include "Atom/AtomAisacPatch.h"

#include "AtomModulationControlBus.generated.h"

// Forward Definitions
class UAtomModulationGenerator;
class UAtomModulatorBase;

struct FPropertyChangedEvent;

/** Class to assign settings allowing modulation AISAC control override. */
UCLASS(BlueprintType, hidecategories = Object, editinlinenew, MinimalAPI)
class UAtomModulationControlBus : public UAtomModulatorBase
{
	GENERATED_BODY()

public:

	UAtomModulationControlBus(const FObjectInitializer& ObjectInitializer);

	/** If true, bypasses control bus from being modulated by parameters, patches, or mixed (control bus remains active and computed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = General)
	bool bBypass;

#if WITH_EDITORONLY_DATA
	/** If true, Address field is used in place of object name for address used when applying mix changes using filtering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mix)
	bool bOverrideAddress;
#endif // WITH_EDITORONLY_DATA

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = General)
	TObjectPtr<UAtomModulationParameter> Parameter;

	/** Address to use when applying mix changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Mix, meta = (EditCondition = "bOverrideAddress"))
	FString Address;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Generators)
	TArray<TObjectPtr<UAtomModulationGenerator>> Generators;

	/* UObject Implementation */
#if WITH_EDITOR
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) override;
	virtual void PostInitProperties() override;
	virtual void PostRename(UObject* OldOuter, const FName OldName) override;
#endif // WITH_EDITOR

	virtual void BeginDestroy() override;

	/* USoundModulatorBase Implementation */
	virtual TUniquePtr<Atom::IModulatorSettings> CreateProxySettings() const override;
	//virtual TUniquePtr<Audio::IProxyData> CreateNewProxyData(const Audio::FProxyDataInitParams& InitParams) override;
	virtual const Atom::FModulationParameter& GetOutputParameter() const override;

	const Atom::FModulationMixFunction GetMixFunction() const;

	float GetDefaultNormalizedValue() const { return Parameter ? Parameter->Settings.ValueNormalized : 1.0f; }
};
