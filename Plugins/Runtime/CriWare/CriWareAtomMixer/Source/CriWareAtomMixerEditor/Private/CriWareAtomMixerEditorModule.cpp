
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

#include "CriWareAtomMixerEditorPrivate.h"

DEFINE_LOG_CATEGORY(LogCriWareAtomMixerEditor);

class FCriWareAtomMixerEditorModule
	: public IModuleInterface
{
public:

	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

		// Custom Class Layouts
		auto RegisterCustomClass = [this, InPropertyModule = &PropertyModule](FName Name, FOnGetDetailCustomizationInstance InstanceGetter)
		{
			InPropertyModule->RegisterCustomClassLayout(Name, InstanceGetter);
			CustomClassLayoutNames.Add(Name);
		};

		//RegisterCustomClass("AcousticsAtomSourceDataOverrideSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FAcousticsAtomSourceDataOverrideSettingsDetails::MakeInstance));
	}

	virtual void ShutdownModule() override
	{
		// Unregister Custom Class Layouts
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

			for (FName ClassName : CustomClassLayoutNames)
			{
				PropertyModule.UnregisterCustomClassLayout(ClassName);
			}
			CustomClassLayoutNames.Reset();
		}
	}

private:

	/** The collection of registered customized class names. */
	TArray<FName> CustomClassLayoutNames;
};

IMPLEMENT_MODULE(FCriWareAtomMixerEditorModule, CriWareAtomMixerEditor)
