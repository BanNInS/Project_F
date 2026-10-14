
#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h"
#include "HAL/CriticalSection.h"
#include "UObject/NameTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "Templates/UniquePtr.h"
#if WITH_EDITORONLY_DATA
#include "UObject/ObjectSaveContext.h"
#include "Interfaces/ITargetPlatform.h"
#endif

#include "IAtomEndpoint.generated.h"

namespace Atom
{
	/**
	 * This helper function is used to downcast abstract objects during callbacks.
	 * Since implementing this API requires frequent downcasting of opaque data, and RTTI is not
	 * enabled by default in our codebase, This is useful for avoiding programmer error.
	 */
	template<typename ToType, typename FromType>
	ToType& DowncastProxyRef(FromType& InRef)
	{
	#if PLATFORM_WINDOWS
		constexpr bool bIsToTypeChildClass = std::is_base_of<FromType, ToType>::value;
		static_assert(bIsToTypeChildClass, "Tried to cast a reference to an unrelated type.");
	#endif

		check(&InRef != nullptr);

		return *static_cast<ToType*>(&InRef);
	}
} // namespace

 /**
  * This interface should be used to provide a non-uclass version of the data described in
  * your implementation of UAtomEndpointSettingsBase.
  */
class IAtomEndpointSettingsProxy
{
public:
	virtual ~IAtomEndpointSettingsProxy() {}
};

/**
 * This opaque class should be used for specifying settings for how audio should be
 * send to an external endpoint.
 */
UCLASS(config = CriWare, abstract, EditInlineNew, BlueprintType, MinimalAPI)
class UAtomEndpointSettingsBase : public UObject
{
	GENERATED_BODY()

public:

	CRIWARECORE_API virtual TUniquePtr<IAtomEndpointSettingsProxy> GetProxy() const PURE_VIRTUAL(UAtomEndpointSettingsBase::GetProxy, return nullptr;);

#if WITH_EDITORONLY_DATA
	CRIWARECORE_API virtual bool IsValidForPlatform(const FString& PlatformName) const PURE_VIRTUAL(UAtomEndpointSettingsBase::IsValidForPlatform, return true;);
	CRIWARECORE_API virtual FString GetSettingsName() const PURE_VIRTUAL(UAtomEndpointSettingsBase::IsValidForPlatform, return TEXT("Default Atom Endpoint Settings"););

	//~ Begin UObject Interface
	CRIWARECORE_API virtual void PreSave(FObjectPreSaveContext SaveContext) override
	{
		Super::PreSave(SaveContext);

		bIsValidPlatform = true;

		if (SaveContext.IsCooking())
		{
			bIsValidPlatform = IsValidForPlatform(SaveContext.GetTargetPlatform()->PlatformName());
		}
	}

	CRIWARECORE_API virtual bool IsEditorOnly() const override
	{
		return !bIsValidPlatform;
	}
	//= End UObject Interface

private:

	bool bIsValidPlatform = true;
#endif
};
