
#include "AtomRackAssetDefinition.h"

#include "EditorFramework/AssetImportData.h"

#include "Toolkits/AtomRackEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AtomRackActions"

EAssetCommandResult UAtomRackAssetDefinition::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UAtomRackBase* AtomRack : OpenArgs.LoadObjects<UAtomRackBase>())
	{
		TSharedRef<FAtomRackEditorToolkit> EditorToolkit = MakeShareable(new FAtomRackEditorToolkit());
		EditorToolkit->Initialize(AtomRack, OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost);
	}

	return EAssetCommandResult::Handled;
}

EAssetCommandResult UAtomRackAssetDefinition::ActivateAssets(const FAssetActivateArgs& ActivateArgs) const
{
	TSet<UAtomRackBase*> RacksToSelect;
	IAssetEditorInstance* Editor = nullptr;
	for (UAtomRackBase* RackToSelect : ActivateArgs.LoadObjects<UAtomRackBase>())
	{
		if (!Editor)
		{
			Editor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(RackToSelect, true);
		}
		RacksToSelect.Add(RackToSelect);
	}

	if (Editor)
	{
		static_cast<FAtomRackEditorToolkit*>(Editor)->SelectRacks(RacksToSelect);
		return EAssetCommandResult::Handled;
	}

	return EAssetCommandResult::Unhandled;
}

#undef LOCTEXT_NAMESPACE
