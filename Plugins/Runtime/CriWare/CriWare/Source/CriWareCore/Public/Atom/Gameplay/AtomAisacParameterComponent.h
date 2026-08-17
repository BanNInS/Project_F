
#pragma once

#include "Atom/Gameplay/AtomGameplayComponent.h"
#include "Atom/AtomAisacPatch.h"
#include "Atom/Interfaces/IAtomActorAisacParameter.h"
#include "Extensions/IAtomAisacParameterController.h"

#include "AtomAisacParameterComponent.generated.h"

#define CRI_API CRIWARECORE_API

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareAtomAisacParameterComponent, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAisacParameterChanged, const FAtomAisacParameter&, Parameter);

// Forward Declarations
class UAtomComponent;

/**
 *	UAisacParameterComponent - Can be used to set/store audio parameters and automatically dispatch them (through ActorSoundParameterInterface) 
 *  to any sounds played by the component's Owner Actor
 */
UCLASS(MinimalAPI, BlueprintType, HideCategories=(Object, ActorComponent, Physics, Rendering, Mobility, LOD), meta=(BlueprintSpawnableComponent))
class UAtomAisacParameterComponent : public UAtomGameplayComponent, 
		  							 public IAtomAisacParameterController,
									 public IAtomActorAisacParameter
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Parameters)
	const TArray<FAtomAisacParameter>& GetParameters() const { return Parameters; }

	/** Begin IAtomActorAisacParameterInterface */
	CRI_API virtual void GetActorAisacParams_Implementation(TArray<FAtomAisacParameter>& Params) const override;
	/** End IAtomActorAisacParameterInterface */

	/** Begin IAtomAisacParameterController Interface */
	CRI_API virtual void ResetParameters() override;
	CRI_API virtual void SetTriggerParameter(const FAtomAisacControl& InControl) override;
	CRI_API virtual void SetAisacParameter(const FAtomAisacControl& InControl, float InValue) override;
	CRI_API virtual void SetParameters_Blueprint(const TArray<FAtomAisacParameter>& InParameters) override;
	CRI_API virtual void SetParameter(FAtomAisacParameter&& InValue) override;
	CRI_API virtual void SetParameters(TArray<FAtomAisacParameter>&& InValues) override;
	/** End IAtomAisacParameterController Interface */

#if WITH_EDITORONLY_DATA
	UPROPERTY(BlueprintAssignable)
	FOnAisacParameterChanged OnParameterChanged;
#endif // WITH_EDITORONLY_DATA

private:

	CRI_API void SetParameterInternal(FAtomAisacParameter&& InParam);
	CRI_API void GetAllAtomComponents(TArray<TObjectPtr<UAtomComponent>>& OutComponents) const;
	CRI_API void LogParameter(FAtomAisacParameter& InParam);

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UAtomComponent>> ActiveComponents;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	TArray<FAtomAisacParameter> Parameters;
};

#undef CRI_API
