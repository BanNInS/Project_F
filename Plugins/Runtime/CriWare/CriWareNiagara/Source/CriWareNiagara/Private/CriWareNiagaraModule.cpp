/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine 4
 * Module   : CriWareNiagara
 * File     : CriWareNiagaraModule.cpp
 *
 ****************************************************************************/

#include "CriWareNiagaraModule.h"

//#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "CriWareNiagaraModule"

class FCriWareNiagaraModule : public ICriWareNiagaraModule
{
	void StartupModule()
	{
		// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

		//FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ChaosNiagara"))->GetBaseDir(), TEXT("Shaders"));
		//AddShaderSourceDirectoryMapping(TEXT("/Plugin/Experimental/ChaosNiagara"), PluginShaderDir);
	}

	void ShutdownModule()
	{
		// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
		// we call this function before unloading the module.
	}
};

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCriWareNiagaraModule, CriWareNiagara)
