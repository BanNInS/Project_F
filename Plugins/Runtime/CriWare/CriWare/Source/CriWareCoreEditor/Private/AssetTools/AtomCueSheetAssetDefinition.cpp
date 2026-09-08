
#include "AtomCueSheetAssetDefinition.h"

#include "EditorFramework/AssetImportData.h"

#include "AtomEditorStyle.h"
#include "Toolkits/AtomSoundBankEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AtomCueSheetActions"

void UAtomCueSheetAssetDefinition::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
	for (auto& Asset : TypeAssets)
	{
		const auto AtomCueSheet = CastChecked<UAtomCueSheet>(Asset);
		if (AtomCueSheet->AssetImportData)
		{
			AtomCueSheet->AssetImportData->ExtractFilenames(OutSourceFilePaths);
		}
	}
}

EAssetCommandResult UAtomCueSheetAssetDefinition::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UAtomSoundBank* SoundBank : OpenArgs.LoadObjects<UAtomSoundBank>())
	{
		TSharedRef<FAtomSoundBankEditorToolkit> EditorToolkit = MakeShareable(new FAtomSoundBankEditorToolkit());
		EditorToolkit->Initialize(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, SoundBank);
	}

	return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE
