
#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareSoundscape, Log, All);

class CRIWARESOUNDSCAPE_API FCriWareSoundscapeModule 
	: public IModuleInterface
{
public:

	// IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
