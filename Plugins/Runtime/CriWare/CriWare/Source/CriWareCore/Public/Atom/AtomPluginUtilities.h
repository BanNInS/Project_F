
#pragma once

// Forward Definitions
class IAtomSourceDataOverrideFactory;
class IManaRuntimeFactory;
class IAtomRuntimePluginFactory;
enum class EAtomPlugin : uint8;

/* Plugin Utilities
 *****************************************************************************/
struct CRIWARECORE_API AtomPluginUtilities
{
	/*
	 * These functions return a pointer to the plugin factory
	 * that matches the plugin name specified in the target
	 * platform's settings.
	 *
	 * if no matching plugin is found, nullptr is returned.
	 */
	static IAtomSourceDataOverrideFactory* GetDesiredSourceDataOverridePlugin();
	static IManaRuntimeFactory* GetDesiredManaRuntimePlugin();

	static IAtomRuntimePluginFactory* GetDesiredRuntimePlugin(FName Name);

	/*
	 * These functions return an array of all plugin factories
	 * that matches the plugin name specified in the target
	 * platform's settings.
	 *
	 * if no matching plugin is found, an empty array is returned.
	 */
	static TArray<IAtomRuntimePluginFactory*> GetAllDesiredAtomRuntimePlugins();

	/** This function returns the name of the plugin specified in the platform settings. */
	static FString GetDesiredPluginName(EAtomPlugin PluginType);

	static const FName& GetDefaultManaRuntimePluginName();
};
