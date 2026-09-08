
#include "ICriWareAtomInsightsEditorModule.h"

#include "CriWareAtomInsightsEditorModule.h"
#include "Modules/ModuleManager.h"

IAtomInsightsTraceModule& ICriWareAtomInsightsEditorModule::GetTraceModule()
{
	ICriWareAtomInsightsEditorModule& CriWareAtomInsightsModule = static_cast<ICriWareAtomInsightsEditorModule&>(FCriWareAtomInsightsEditorModule::GetChecked());
	return CriWareAtomInsightsModule.GetTraceModule();
}

bool ICriWareAtomInsightsEditorModule::IsModuleLoaded()
{
	return FCriWareAtomInsightsEditorModule::IsModuleLoaded();
}

ICriWareAtomInsightsEditorModule& ICriWareAtomInsightsEditorModule::GetChecked()
{
	return static_cast<ICriWareAtomInsightsEditorModule&>(FCriWareAtomInsightsEditorModule::GetChecked());
}
