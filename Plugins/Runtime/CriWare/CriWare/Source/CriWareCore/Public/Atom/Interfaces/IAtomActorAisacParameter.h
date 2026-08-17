
#pragma once

#include "Containers/Array.h"
#include "UObject/Object.h"
#include "UObject/Interface.h"

#include "Extensions/IAtomAisacParameterController.h"

#include "IAtomActorAisacParameter.generated.h"

// Forward Declarations
class AActor;
class UActorComponent;

/** Interface used to allow an actor to automatically populate any Atom sounds with AISAC parameters */
class IAtomActorAisacParameter : public IInterface
{
	GENERATED_BODY()

public:
	// Overrides logic for gathering AudioParameters to set by default when an AudioComponent/ActiveSound plays with a given actor as its Owner.
	UFUNCTION(BlueprintNativeEvent, Category = "Atom|Aisac", meta = (DisplayName = "Get Actor Atom Aisac Parameters"))
	CRIWARECORE_API void GetActorAisacParams(TArray<FAtomAisacParameter>& Params) const;
	virtual void GetActorAisacParams_Implementation(TArray<FAtomAisacParameter>& Params) const = 0;
};

UINTERFACE(BlueprintType, MinimalAPI)
class UAtomActorAisacParameter : public UInterface
{
	GENERATED_BODY()

public:

	static CRIWARECORE_API void Fill(const AActor* OwningActor, TArray<FAtomAisacParameter>& OutParams);

private:

	static void GetImplementers(const AActor* InActor, TArray<const AActor*>& OutActors, TArray<const UActorComponent*>& OutComponents);
};
