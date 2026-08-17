
#pragma once

#include "Factories/Factory.h"

#include "AtomSoundscapeColorFactory.generated.h"

UCLASS(hidecategories = Object, MinimalAPI)
class UAtomSoundscapeColorFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	virtual bool ShouldShowInNewMenu() const override
	{
		return true;
	}
	//~ End UFactory Interface	
};

