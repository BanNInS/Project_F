#include "Atom/AtomPluginUtilities.h"

#include "CoreGlobals.h"
#include "CoreMinimal.h"
#include "Features/IModularFeatures.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"

#include "CriWareCoreSettings.h"
#include "Extensions/IAtomExtensionPlugin.h"
#include "Extensions/IAtomRuntimePlugin.h"
#include "Extensions/IManaRuntime.h"

/** Get the target setting name for each platform type. */
FORCEINLINE const TCHAR* GetPluginConfigName(EAtomPlugin PluginType)
{
	switch (PluginType)
	{
	case EAtomPlugin::SourceDataOverride:
		return TEXT("AtomSourceDataOverridePlugin");

	case EAtomPlugin::Runtime:
		return TEXT("AtomRuntimePlugin");

	case EAtomPlugin::Mana:
		return TEXT("ManaPlugin");

	default:
		checkf(false, TEXT("Undefined Atom plugin type."));
		return TEXT("");
	}
}

IAtomSourceDataOverrideFactory* AtomPluginUtilities::GetDesiredSourceDataOverridePlugin()
{
	FString DesiredSourceDataOverridePlugin = GetDesiredPluginName(EAtomPlugin::SourceDataOverride);

	IModularFeatures::Get().LockModularFeatureList();
	TArray<IAtomSourceDataOverrideFactory*> SourceDataOverridePluginFactories = IModularFeatures::Get().GetModularFeatureImplementations<IAtomSourceDataOverrideFactory>(IAtomSourceDataOverrideFactory::GetModularFeatureName());
	IModularFeatures::Get().UnlockModularFeatureList();

	//Iterate through all of the plugins we've discovered:
	for (IAtomSourceDataOverrideFactory* PluginFactory : SourceDataOverridePluginFactories)
	{
		//if this plugin's name matches the name found in the platform settings, use it:
		if (PluginFactory->GetDisplayName().Equals(DesiredSourceDataOverridePlugin))
		{
			return PluginFactory;
		}
	}

	return nullptr;
}

IManaRuntimeFactory* AtomPluginUtilities::GetDesiredManaRuntimePlugin()
{
	const FName& PlatformPluginName = FName(*GetDesiredPluginName(EAtomPlugin::Mana));
	const FName& PluginName = (PlatformPluginName == NAME_None) ? GetDefaultManaRuntimePluginName() : PlatformPluginName;
	const FName& FeatureName = IManaRuntimeFactory::GetModularFeatureName();

	IModularFeatures::Get().LockModularFeatureList();
	TArray<IManaRuntimeFactory*> Factories = IModularFeatures::Get().GetModularFeatureImplementations<IManaRuntimeFactory>(FeatureName);
	IModularFeatures::Get().UnlockModularFeatureList();

	for (IManaRuntimeFactory* PluginFactory : Factories)
	{
		//if this plugin's name matches the name found in the platform settings, use it:
		if (PluginFactory->GetDisplayName() == PluginName)
		{
			return PluginFactory;
		}
	}

	return nullptr;
}

const FName& AtomPluginUtilities::GetDefaultManaRuntimePluginName()
{
	static FName DefaultManaRuntimePluginName(TEXT("DefaultManaRuntimePlugin"));

	//if (!DefaultModulationPluginCVar.IsEmpty())
	//{
	//	DefaultModulationPluginName = FName(*DefaultModulationPluginCVar);
	//}

	return DefaultManaRuntimePluginName;
}

IAtomRuntimePluginFactory* AtomPluginUtilities::GetDesiredRuntimePlugin(FName Name)
{
	FString DesiredRuntimePlugin = GetDesiredPluginName(EAtomPlugin::Runtime) + Name.ToString();

	IModularFeatures::Get().LockModularFeatureList();
	TArray<IAtomRuntimePluginFactory*> RuntimePluginFactories = IModularFeatures::Get().GetModularFeatureImplementations<IAtomRuntimePluginFactory>(IAtomRuntimePluginFactory::GetModularFeatureName());
	IModularFeatures::Get().UnlockModularFeatureList();

	//Iterate through all of the plugins we've discovered:
	for (IAtomRuntimePluginFactory* PluginFactory : RuntimePluginFactories)
	{
		//if this plugin's name matches the name found in the platform settings, use it:
		if (PluginFactory->GetDisplayName().Equals(DesiredRuntimePlugin))
		{
			return PluginFactory;
		}
	}

	return nullptr;
}

TArray<IAtomRuntimePluginFactory*> AtomPluginUtilities::GetAllDesiredAtomRuntimePlugins()
{
	FName PlatformPluginName = FName(*GetDesiredPluginName(EAtomPlugin::Runtime));
	const FName& PluginName = (PlatformPluginName == NAME_None) ? IAtomRuntimePluginFactory::GetDefaultDisplayName() : PlatformPluginName;
	const FName& FeatureName = IAtomRuntimePluginFactory::GetModularFeatureName();

	IModularFeatures::Get().LockModularFeatureList();
	TArray<IAtomRuntimePluginFactory*> RuntimePluginFactories = IModularFeatures::Get().GetModularFeatureImplementations<IAtomRuntimePluginFactory>(FeatureName);
	IModularFeatures::Get().UnlockModularFeatureList();

	TArray<IAtomRuntimePluginFactory*> DesiredPlugins;
	//Iterate through all of the plugins we've discovered:
	for (IAtomRuntimePluginFactory* PluginFactory : RuntimePluginFactories)
	{
		//if this plugin's name matches the name found in the platform settings, use it:
		//if (PluginFactory->GetDisplayName().Equals(PluginName.ToString()))
		//{
			DesiredPlugins.Add(PluginFactory);
		//}
	}

	return DesiredPlugins;
}

FString AtomPluginUtilities::GetDesiredPluginName(EAtomPlugin PluginType)
{
	// if the platform was passed in, use it, otherwise, get the runtime platform's name for looking up DDPI
	FString PlatformName = FString(FPlatformProperties::IniPlatformName());
	const FString ModuleName = TEXT("CriWare") + PlatformName + TEXT("Settings");
	const FString CategoryName = TEXT("/Script/") + ModuleName + TEXT(".") + ModuleName;

	FString PluginName;
	FString CriWareIni = GetDefault<UCriWareCoreSettings>()->GetDefaultConfigFilename();
	GConfig->GetString(*CategoryName, GetPluginConfigName(PluginType), PluginName, CriWareIni);

	return PluginName;
}
