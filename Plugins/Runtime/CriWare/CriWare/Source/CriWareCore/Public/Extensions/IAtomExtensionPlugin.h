#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"
#include "Math/Interval.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "UObject/ObjectMacros.h"

#include "IAtomExtensionPlugin.generated.h"

// Forward Declarations
class FAtomRuntime;
class IAtomSourceDataOverride;
struct FAtomPlaybackInstance;

/**
 * Enumeration of Atom plugin types
 *
 */
enum class EAtomPlugin : uint8
{
	SourceDataOverride = 0,
	Runtime = 1,
	Mana = 2,

	Count
};

using TAtomSourceDataOverridePtr = TSharedPtr<IAtomSourceDataOverride, ESPMode::ThreadSafe>;

struct FAtomPluginInitializationParams
{
public:

	// Maximum number of sources that can play simultaneously.
	uint32 NumSources;

	// Number of output channels.
	uint32 NumOutputChannels;

	// Sample rate.
	uint32 SampleRate;

	// Buffer length used for each callback.
	uint32 BufferLength;

	FAtomPluginInitializationParams()
		: NumSources(0)
		, NumOutputChannels(0)
		, SampleRate(0)
		, BufferLength(0)
	{}
};

/************************************************************************/
/* IAtomPluginFactory                                                   */
/* This interface is inherited by extension plugins                     */
/* to describe specifics of a plugin such as platform support,          */
/* and display names.                                                   */
/************************************************************************/
class IAtomPluginFactory
{
public:

	/*
	 * Returns human-readable string representing the display name of this plugin.
	 * This is the name that will be used in settings and .ini files.
	 * If multiple IAtomPlugin implementations are found that return identical strings here,
	 * The first one of these loaded will be used.
	 *
	 * @return FString of the display name of this plugin.
	 */
	virtual FString GetDisplayName()
	{
		static FString DisplayName = FString(TEXT("Generic Atom Plugin"));
		return DisplayName;
	}

	/*
	 * Returns whether this plugin supports use on the specified platform.
	 * @param Platform an enumerated platform (i.e. Windows, Playstation4, etc.)
	 * @return true if this plugin supports use on Platform, false otherwise.
	 */
	virtual bool SupportsPlatform(const FString& PlatformName) = 0;

	/*
	 * Returns whether this plugin sends audio to an external renderer.
	 * if this returns true, the Atom engine will not mix the result of the audio process callback
	 * from the plugin into the audio output.
	 *
	 * @return true only if the plugin will handle sending audio to the DAC itself.
	 */
	virtual bool IsExternalSend()
	{
		return false;
	}
};

/** This is a class which should be overridden to provide users with settings to use for individual sounds */
UCLASS(config = CriWare, abstract, EditInLineNew, BlueprintType)
class CRIWARECORE_API UAtomSourceDataOverridePluginSourceSettingsBase : public UObject
{
	GENERATED_BODY()
};

/************************************************************************/
/* IAtomSourceDataOverrideFactory										*/
/* Implement this modular feature to make your SourceDataOverride plugin*/
/* visible to the engine.                                               */
/************************************************************************/
class IAtomSourceDataOverrideFactory : public IAtomPluginFactory, public IModularFeature
{
public:

	/** Virtual destructor */
	virtual ~IAtomSourceDataOverrideFactory()
	{
	}

	// IModularFeature
	static FName GetModularFeatureName()
	{
		static FName AtomExtFeatureName = FName(TEXT("AtomSourceDataOverridePlugin"));
		return AtomExtFeatureName;
	}

	//~ Begin IAtomPluginWithMetadata implementation */
	virtual FString GetDisplayName() override
	{
		static FString DisplayName = FString(TEXT("Generic Atom Source Data Override Plugin"));
		return DisplayName;
	}
	//~ End IAtomPluginWithMetadata implementation */

	/**
	 * @return the UClass type of your settings for source data overrides. This allows us to only pass in user settings for your plugin.
	 */
	virtual UClass* GetCustomSourceDataOverrideSettingsClass() const
	{
		return nullptr;
	}

	/**
	 * @return a new instance of your source data override plugin, owned by a shared pointer.
	 */
	virtual TAtomSourceDataOverridePtr CreateNewSourceDataOverridePlugin(FAtomRuntime* OwningRuntime) = 0;
};

/** Interface to allow a plugin to override a sound's actual position and simulate propagation (e.g. traversal around corners, etc). */
class IAtomSourceDataOverride
{
public:

	/** Virtual destructor */
	virtual ~IAtomSourceDataOverride()
	{
	}

	/** Initializes the source data override plugin with the given buffer length. */
	virtual void Initialize(const FAtomPluginInitializationParams InitializationParams)
	{
	}

	/** Called when a source is assigned to a voice. */
	virtual void OnInitSource(const uint32 SourceID, const FName& AtomComponentUserId, UAtomSourceDataOverridePluginSourceSettingsBase* InSettings)
	{
	}

	/** Called when a source is done playing and is released. */
	virtual void OnReleaseSource(const uint32 SourceID)
	{
	}

	/** Allows this plugin to override any source data. Called per Atom sound source before any other parameters are updated on sound sources. */
	virtual void GetSourceDataOverrides(const uint32 SourceID, const FTransform& InListenerTransform, FAtomPlaybackInstance* InOutPlaybackInstance)
	{
	}

	/** Called when all sources have finished processing. */
	virtual void OnAllSourcesProcessed()
	{
	}
};
