
#include "ICriWareAtomInsightsModule.h"

#include "CriWareAtomInsightsModule.h"

IAtomInsightsTraceModule& ICriWareAtomInsightsModule::GetTraceModule()
{
	ICriWareAtomInsightsModule& AtomInsightsModule = static_cast<ICriWareAtomInsightsModule&>(FCriWareAtomInsightsModule::GetChecked());
	return AtomInsightsModule.GetTraceModule();
}

ICriWareAtomInsightsModule& ICriWareAtomInsightsModule::GetChecked()
{
	return static_cast<ICriWareAtomInsightsModule&>(FCriWareAtomInsightsModule::GetChecked());
}

#ifdef WITH_EDITOR
ICriWareAtomInsightsModule& ICriWareAtomInsightsModule::GetEditorChecked()
{
	return static_cast<ICriWareAtomInsightsModule&>(FModuleManager::GetModuleChecked<ICriWareAtomInsightsModule>("CriWareAtomInsightsEditor"));
}
#endif
