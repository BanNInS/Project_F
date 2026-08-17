
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FCriWareSoundXREditorModule
	: public IModuleInterface
{
public:

	virtual void StartupModule() override
	{

	}

	virtual void ShutdownModule() override
	{

	}
};

IMPLEMENT_MODULE(FCriWareSoundXREditorModule, CriWareSoundXREditor)
