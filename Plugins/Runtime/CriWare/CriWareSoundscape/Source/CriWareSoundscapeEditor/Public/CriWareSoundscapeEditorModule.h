
#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "IAssetTypeActions.h"

class FCriWareSoundscapeEditorModule
	: public IModuleInterface
{
public:

	// IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	/** The collection of registered asset type actions. */
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;
};
