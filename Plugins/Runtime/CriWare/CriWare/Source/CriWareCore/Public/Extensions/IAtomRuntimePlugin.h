#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "UObject/ObjectMacros.h"

#include "IAtomExtensionPlugin.h"

#include "IAtomRuntimePlugin.generated.h"

// Forward Declarations
class FAtomRuntime;
class FAtomSource;
class IAtomRuntimePlugin;
class UViewportStatsSubsystem;
class FCommonViewportClient;

using TAtomRuntimePluginPtr = TSharedPtr<IAtomRuntimePlugin, ESPMode::ThreadSafe>;

/** This is a class which should be overridden to provide users with settings to use for individual sounds */
UCLASS(config = CriWare, Abstract, EditInlineNew, BlueprintType)
class CRIWARECORE_API UAtomRuntimePluginSettingsBase
	: public UObject
{
	GENERATED_BODY()
};

/************************************************************************/
/* IAtomRuntimePluginFactory                                            */
/*                                                                      */
/************************************************************************/
class IAtomRuntimePluginFactory
	: public IAtomPluginFactory
	, public IModularFeature
{
public:
	/** Virtual destructor */
	virtual ~IAtomRuntimePluginFactory()
	{
	}

	// IModularFeature
	static FName GetModularFeatureName()
	{
		static FName AtomExtFeatureName = FName(TEXT("AtomRuntimePlugin"));
		return AtomExtFeatureName;
	}

	static FName GetDefaultDisplayName()
	{
		static FName DisplayName = FName(TEXT("Generic Atom Runtime Plugin"));
		return DisplayName;
	}

	//~ Begin IAtomPluginFactory implementation
	virtual FString GetDisplayName() override
	{
		return GetDefaultDisplayName().ToString();
	}
	//~ End IAtomPluginFactory implementation

	virtual TAtomRuntimePluginPtr CreateNewRuntimePlugin(FAtomRuntime* OwningRuntime) = 0;

	/**
	* @return the UClass type of your settings for plugin. This allows us to only pass in user settings for your plugin.
	*/
	virtual UClass* GetCustomRuntimeSettingsClass() const
	{
		return nullptr;
	}
};

class CRIWARECORE_API IAtomRuntimePlugin
	: public TSharedFromThis<IAtomRuntimePlugin>
{
public:
	virtual FName GetPluginName() const = 0;

	/** Virtual destructor */
	virtual ~IAtomRuntimePlugin() = default;

	/** Initialize plugin after Atom runtime initialization. */
	virtual void Initialize(const FAtomPluginInitializationParams& InitializationParams) = 0;

	/** Finalize  plugin before Atom runtime finalization. */
	virtual void Finalize() = 0;

#if !UE_BUILD_SHIPPING
	/** Request to post help from active plugin (non-shipping builds only) */
	virtual bool OnPostHelp(FCommonViewportClient* ViewportClient, const TCHAR* Stream) { return false; }

	/** Render stats pertaining debug informations (non-shipping builds only) */
	virtual int32 OnRenderStat(UViewportStatsSubsystem* ViewportSubsystem, int32 X, int32 Y) { return Y; }

	/** Toggle showing render stats pertaining to plugin (non-shipping builds only) */
	virtual bool OnToggleStat(FCommonViewportClient* ViewportClient, const TCHAR* Stream) { return false; }
#endif //!UE_BUILD_SHIPPING

	/** Called when a source is assigned to a voice. */
	virtual void OnInitSource(const uint32 SourceId, const FAtomSource* Source) {}

	/** Called when a source is done playing and is released. */
	virtual void OnReleaseSource(const uint32 SourceId) {}

	/** Processes on Atom thread after processing Atom. */
	virtual void ProcessPlugin(const double InElapsed) = 0;
};
