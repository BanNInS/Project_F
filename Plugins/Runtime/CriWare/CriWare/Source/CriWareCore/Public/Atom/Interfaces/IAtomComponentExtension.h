#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UObject/ScriptInterface.h"
#include "UObject/Object.h"
#include "UObject/UObjectGlobals.h"

#include "Atom/AtomComponent.h"

#include "IAtomComponentExtension.generated.h"

class UAtomComponentExtension;

/**
 * Base class for settings optionaly used by UAtomComponentExtension.
 */
UCLASS(abstract, MinimalApi)
class UAtomComponentExtensionSettings : public UObject
{
	GENERATED_BODY()

public:

	virtual ~UAtomComponentExtensionSettings() = default;

	/** return the associated AtomCompoenent derivate class */
	virtual TSubclassOf<UAtomComponent> GetAtomComponentClass() const PURE_VIRTUAL(UAtomComponentExtensionSettings::GetAtomComponentClass, return nullptr;);

#if WITH_EDITOR
	virtual FName GetDisplayName() const PURE_VIRTUAL(UAtomComponentExtensionSettings::GetDisplayName, return NAME_None;);
#endif

	/** Convenient function to retreive the settings class associated with given AtomComponent class. */
	static TSubclassOf<UAtomComponentExtensionSettings> FindExtensionSettingsClassForAtomComponent(TSubclassOf<UAtomComponent> InAtomComponentClass)
	{
		if (InAtomComponentClass)
		{
			// Loop over CDOs to find the associated extenstion setting to create.
			for (TObjectIterator<UAtomComponentExtensionSettings> It(EObjectFlags::RF_NoFlags); It; ++It)
			{
				if (It->HasAnyFlags(RF_ClassDefaultObject))
				{
					// It is the class default object instance of a child class of parent class
					if (UClass* ChildClass = It->GetClass())
					{
						if (ChildClass != UAtomComponentExtensionSettings::StaticClass())
						{
							if (It->GetAtomComponentClass() == InAtomComponentClass)
							{
								return ChildClass;
							}
						}
					}
				}
			}
		}

		return nullptr;
	}
};

/**
 * Interface to implement for generating safe extensions for synth and external components.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UAtomComponentExtension : public UInterface
{
	GENERATED_BODY()
};

class IAtomComponentExtension
{
	GENERATED_BODY()

public:

	/** Apply some settings to this extension. */
	virtual void ApplyExtensionSettings(UAtomComponentExtensionSettings* ExtensionSettings) {}
};
