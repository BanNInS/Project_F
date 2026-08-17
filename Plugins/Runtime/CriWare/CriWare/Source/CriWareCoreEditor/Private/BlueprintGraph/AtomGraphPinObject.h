
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "AtomGraphPinObject.generated.h"

/** Base class to hold a property to permit a customized selection. (Local Use only) */
UCLASS(Transient, Abstract, MinimalApi)
class UAtomGraphPinObject
	: public UObject
{
	GENERATED_BODY()

public:

	virtual UObject* GetObject() const { return nullptr; }
	virtual void SetObject(UObject* Object) {}
	virtual UClass* GetObjectClass() const { return nullptr; };
};