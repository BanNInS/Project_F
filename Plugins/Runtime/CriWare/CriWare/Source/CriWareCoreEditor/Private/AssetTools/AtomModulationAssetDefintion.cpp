
#include "AtomModulationAssetDefinition.h"

#include "Toolkits/AtomModulationPatchEditorToolkit.h"

EAssetCommandResult UAtomModulationPatchAssetDefinition::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UAtomModulationPatch* Patch : OpenArgs.LoadObjects<UAtomModulationPatch>())
	{
		TSharedRef<FAtomModulationPatchEditorToolkit> EditorToolkit = MakeShareable(new FAtomModulationPatchEditorToolkit());
		EditorToolkit->Init(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Patch);
	}

	return EAssetCommandResult::Handled;
}
