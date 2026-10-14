
#include "AtomWaveBankAssetDefinition.h"

#include "ContentBrowserMenuContexts.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "EditorFramework/AssetImportData.h"
#include "AssetToolsModule.h"

#include "Atom/AtomSoundSimple.h"

#include "AtomEditorStyle.h"
#include "Toolkits/AtomSoundBankEditorToolkit.h"
#include "Factories/AtomSoundSimpleFactory.h"

#define LOCTEXT_NAMESPACE "AtomWaveBankActions"

void UAtomWaveBankAssetDefinition::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
	for (auto& Asset : TypeAssets)
	{
		const auto AtomWaveBank = CastChecked<UAtomWaveBank>(Asset);
		if (AtomWaveBank->AssetImportData)
		{
			AtomWaveBank->AssetImportData->ExtractFilenames(OutSourceFilePaths);
		}
	}
}

EAssetCommandResult UAtomWaveBankAssetDefinition::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UAtomSoundBank* SoundBank : OpenArgs.LoadObjects<UAtomSoundBank>())
	{
		TSharedRef<FAtomSoundBankEditorToolkit> EditorToolkit = MakeShareable(new FAtomSoundBankEditorToolkit());
		EditorToolkit->Initialize(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, SoundBank);
	}

	return EAssetCommandResult::Handled;
}

namespace MenuExtension_AtomWaveBank
{
	static void ExecuteCreateSimpleSound(const FToolMenuContext& InContext)
	{
		static const FString DefaultSuffix = TEXT("_AtomSimpleSound");

		if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
		{
			TArray<FAssetData> WaveBankAssets = CBContext->GetSelectedAssetsOfType(UAtomWaveBank::StaticClass());
			if (WaveBankAssets.Num() > 0)
			{
				// Determine an appropriate name
				FString Name;
				FString PackagePath;

				// Create the factory used to generate the asset
				UAtomSoundSimpleFactory* Factory = NewObject<UAtomSoundSimpleFactory>();
				Factory->SoundWaves.Reset();

				FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
				AssetToolsModule.Get().CreateUniqueAssetName(WaveBankAssets[0].GetPackage()->GetName(), DefaultSuffix, PackagePath, Name);

				for (auto& WaveBankAsset : WaveBankAssets)
				{
					if (UAtomWaveBank* WaveBank = Cast<UAtomWaveBank>(WaveBankAsset.GetAsset()))
					{
						for (int Index = 0; Index < WaveBank->GetNumWaves(); Index++)
						{
							if (UAtomSoundWave* Wave = WaveBank->GetSoundWave(Index))
							{
								Factory->SoundWaves.Add(Wave);
							}
						}
					}
				}

				FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UAtomSoundSimple::StaticClass(), Factory);
			}
		}
	}

	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, [] {
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
		{
			FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
			UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UAtomWaveBank::StaticClass());
			FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

			Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
			{
				const TAttribute<FText> Label = LOCTEXT("AtomWaveBank_CreateSimpleSound", "Create Atom Simple Sound");
				const TAttribute<FText> ToolTip = LOCTEXT("AtomWaveBank_CreateSimpleSoundTooltip", "Creates a simple sound assets using all sound waves of each selected wave bank.");
				const FSlateIcon Icon = FSlateIcon(FAtomEditorStyle::Get().GetStyleSetName(), "ClassIcon.AtomSoundSimple");
				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteCreateSimpleSound);

				InSection.AddMenuEntry("AtomWaveBank_CreateSimpleSound", Label, ToolTip, Icon, UIAction);
			}));
		}));
	});
} // namespace

#undef LOCTEXT_NAMESPACE
