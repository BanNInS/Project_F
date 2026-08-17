
#include "CriWareAtomMixerModule.h"
#include "CriWareAtomMixerPrivate.h"

#include "Extensions/IAtomRuntimePlugin.h"

#include "AtomMixerAtomRuntimePlugin.h"

 // Log Categories
DEFINE_LOG_CATEGORY(LogCriWareAtomMixer2);
DEFINE_LOG_CATEGORY(LogCriWareAtomMixer2Debug);

#define LOCTEXT_NAMESPACE "CriWareAtomMixerModule"

// Forward Declarations
class FAtomRuntime;

// For creating our custom source data override plugin
class FAtomMixerAtomRuntimePluginFactory
	: public IAtomRuntimePluginFactory
{
public:

	virtual FString GetDisplayName() override
	{
		static FString DisplayName = FString(TEXT("CriWare Atom Mixer"));
		return DisplayName;
	}

	virtual bool SupportsPlatform(const FString& PlatformName) override
	{
		return true;
	}

	virtual UClass* GetCustomRuntimeSettingsClass() const override { return nullptr; }// UAtomMixerAtomRuntimeSourceSettings::StaticClass(); }

	virtual TAtomRuntimePluginPtr CreateNewRuntimePlugin(FAtomRuntime* OwningRuntime) override
	{
		return TAtomRuntimePluginPtr(new FAtomMixerAtomRuntimePlugin(OwningRuntime));
	}
};

class FCriWareAtomMixerModule
	: public ICriWareAtomMixerModule
{
	virtual void StartupModule() override
	{
		IModularFeatures::Get().RegisterModularFeature(FAtomMixerAtomRuntimePluginFactory::GetModularFeatureName(), &AtomMixerAtomRuntimePluginFactory);
	}

	virtual void ShutdownModule() override
	{
	}

public:

	// Plugins
	IAtomPluginFactory* GetPluginFactory(EAtomPlugin PluginType)
	{
		switch (PluginType)
		{
		case EAtomPlugin::Runtime:
			return &AtomMixerAtomRuntimePluginFactory;
			break;
		default:
			return nullptr;
		}
	}

private:

	// Factories
	FAtomMixerAtomRuntimePluginFactory AtomMixerAtomRuntimePluginFactory;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCriWareAtomMixerModule, CriWareAtomMixer)
