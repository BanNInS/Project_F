#pragma once

#include "CoreTypes.h"

#include "IAtomExtensionPlugin.h"

// Forward Definitions
struct FAtomPluginInitializationParams;
class IManaRuntime;
class UViewportStatsSubsystem;
class FCommonViewportClient;

using TManaRuntimePtr = TSharedPtr<IManaRuntime, ESPMode::ThreadSafe>;

/************************************************************************/
/* IManaRuntimeFactory                                              */
/*                                                                      */
/************************************************************************/
class IManaRuntimeFactory : public IModularFeature
{
public:
	/** Virtual destructor */
	virtual ~IManaRuntimeFactory()
	{
	}

	// IModularFeature
	static FName GetModularFeatureName()
	{
		static FName AudioExtFeatureName = FName(TEXT("ManaRuntimePlugin"));
		return AudioExtFeatureName;
	}

	virtual const FName& GetDisplayName() const = 0;

	virtual TManaRuntimePtr CreateNewManaRuntimePlugin(FAtomRuntime* OwningRuntime) = 0;

	/**
	* @return the UClass type of your settings for Mana. This allows us to only pass in user settings for your plugin.
	*/
	virtual UClass* GetCustomModulationSettingsClass() const
	{
		return nullptr;
	}
};

/**
 * Interface to access the mana runtime from core plugin
 */
class CRIWARECORE_API IManaRuntime
	: public TSharedFromThis<IManaRuntime>
{
public:

	/** Virtual destructor */
	virtual ~IManaRuntime() = default;

	/** Initialize plugin after Atom runtime initialization. */
	virtual void Initialize(const FAtomPluginInitializationParams& InitializationParams) = 0;

	/** Finalize  plugin before Atom runtime finalization. */
	virtual void Finalize() = 0;

#if !UE_BUILD_SHIPPING
	/** Request to post help from active plugin (non-shipping builds only) */
	virtual bool OnPostHelp(FCommonViewportClient* ViewportClient, const TCHAR* Stream) = 0;

	/** Render stats pertaining debug informations (non-shipping builds only) */
	virtual int32 OnRenderStat(UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y) = 0;

	/** Toggle showing render stats pertaining to plugin (non-shipping builds only) */
	virtual bool OnToggleStat(FCommonViewportClient* ViewportClient, const TCHAR* Stream) = 0;
#endif //!UE_BUILD_SHIPPING

	/** Processes on Atom thread after processing Atom. */
	virtual void ProcessPlugin(const double InElapsed) = 0;

	/** Atom runtime call this when creating dbas settings. If returns false, Atom will handle Dbas automatically. */
	virtual bool GetAtomDbasSettings(int& InOutDbasMaxBPS, int& InOutDBasMaxStream) = 0;

	/** CriWareCore call this when setting up FileSystem. If returns false, Atom will handle FileSystem automatically. */
	virtual bool GetFileSystemSettings(int& InOutNumBinders, int& InOutMaxBindings, int& InOutNumLoaders) = 0;
};
