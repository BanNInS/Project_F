
#pragma once

#include "Misc/AssertionMacros.h"
#include "Misc/CoreMiscDefines.h"
#include "Templates/Casts.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"
#include "UObject/NameTypes.h"

#include <type_traits>

/**
 * Interfaces for Atom Proxy Objects 
 * These are used to spawn thread safe instances of UObjects that may be garbage collected on the game thread.
 * In shipping builds, these are effectively abstract pointers, but CHECK_ATOMPROXY_TYPES can optionally be used
 * to check downcasts.
 */

#define  IMPL_ATOMPROXY_CLASS(FClassName) \
	static FName GetAtomProxyTypeName() \
	{ \
		static FName MyClassName = #FClassName; \
		return MyClassName; \
	} \
	static constexpr bool bWasAtomProxyClassImplemented = true; \
	friend class ::Atom::IProxyData; \
	friend class ::Atom::TProxyData<FClassName>;


namespace Atom
{
	// Forward Declarations
	class IProxyData;

	/*
	 * Base class that allows us to typecheck proxy data before downcasting it in debug builds.
	*/
	class IProxyData
	{
	private:

		FName ProxyTypeName;
	public:

		virtual ~IProxyData() = default;

		template<typename ProxyType>
		bool CheckTypeCast() const
		{
			const FName DestinationTypeName = ProxyType::GetAtomProxyTypeName();
			return ensureAlwaysMsgf(ProxyTypeName == DestinationTypeName, TEXT("Tried to downcast type %s to %s!"), *ProxyTypeName.ToString(), *DestinationTypeName.ToString());
		}

		FName GetProxyTypeName() const
		{
			return ProxyTypeName;
		}

		template<typename ProxyType>
		ProxyType& GetAs()
		{
			static_assert(std::is_base_of_v<IProxyData, ProxyType>, "Tried to downcast IProxyInitData to an unrelated type!");
			if (CheckTypeCast<ProxyType>())
			{
				return static_cast<ProxyType&>(*this);
			}
			else
			{
				// This is an illegal cast, and is considered a fatal error.
				checkNoEntry();
				return *((ProxyType*)0x1);
			}
		}

		template<typename ProxyType>
		const ProxyType& GetAs() const
		{
			static_assert(std::is_base_of_v<IProxyData, ProxyType>, "Tried to downcast IProxyInitData to an unrelated type!");
			if (CheckTypeCast<ProxyType>())
			{
				return static_cast<const ProxyType&>(*this);
			}
			else
			{
				// This is an illegal cast, and is considered a fatal error.
				checkNoEntry();
				return *((ProxyType*)0x1);
			}
		}

		IProxyData(FName InProxyTypeName)
			: ProxyTypeName(InProxyTypeName)
		{}
	};

	/**
	 * This class can be implemented to create a custom, threadsafe instance of a given UObject.
	 * This is a CRTP class, and should always be subclassed with the name of the subclass.
	 */
	template <typename Type>
	class TProxyData : public IProxyData
	{
	protected:

		static constexpr bool bWasAtomProxyClassImplemented = false;

	public:

		TProxyData()
			: IProxyData(Type::GetAtomProxyTypeName())
		{
			static_assert(Type::bWasAtomProxyClassImplemented, "Make sure to include IMPL_ATOMPROXY_CLASS(ClassName) in your implementation of TProxyData.");
		}
	};

	struct FProxyDataInitParams
	{
		FName NameOfFeatureRequestingProxy;
	};
} // namespace

/*
* This can be subclassed to make a UClass an Atom proxy factory.
*/
class CRIWARECORE_API IAtomProxyDataFactory
{
public:

	virtual TSharedPtr<Atom::IProxyData> CreateProxyData(const Atom::FProxyDataInitParams& InitParams) { return nullptr; }
};

namespace Atom
{
	// SFINAE used to optionally invoke subclasses of IAtomProxyDataFactory when we can.
	template<typename UClassToUse, typename std::enable_if_t<std::is_base_of_v<IAtomProxyDataFactory, UClassToUse>, bool> = true>
	IAtomProxyDataFactory* CastToProxyDataFactory(UObject* InObject)
	{
		if (InObject)
		{
			UClassToUse* DowncastObject = Cast<UClassToUse>(InObject);
			if (ensureAlways(DowncastObject))
			{
				return static_cast<IAtomProxyDataFactory*>(DowncastObject);
			}
		}

		return nullptr;
	}

	template<typename UClassToUse, typename std::enable_if_t<!std::is_base_of_v<IAtomProxyDataFactory, UClassToUse>, bool>::Type = true>
	IAtomProxyDataFactory* CastToProxyDataFactory(UObject* InObject)
	{
		return nullptr;
	}
} // namespace
