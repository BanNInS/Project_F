
#include "AtomSoundClassAssetDefinition.h"

#include "ContentBrowserMenuContexts.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/PackageName.h"
#include "ToolMenus.h"
#include "ToolMenuSection.h"

#include "CriWare.h"
#include "CriWareDefines.h"
#include "Atom/AtomDebug.h"

#include "Toolkits/AtomSoundClassEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AtomSoundClassActions"

EAssetCommandResult UAtomSoundClassAssetDefinition::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	for (UAtomSoundClass* SoundClass : OpenArgs.LoadObjects<UAtomSoundClass>())
	{
			TSharedRef<FAtomSoundClassEditorToolkit> EditorToolkit = MakeShareable(new FAtomSoundClassEditorToolkit());
			EditorToolkit->Initialize(SoundClass, OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost);
	}

	return EAssetCommandResult::Handled;
}

namespace MenuExtension_AtomSoundClass
{
	static void ExecuteMute(const FToolMenuContext& InContext)
	{
#if ENABLE_ATOM_DEBUG
		if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
		{
			if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
			{
				for (const FAssetData& SoundClassAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundClass::StaticClass()))
				{
					RuntimeManager->GetDebugger().ToggleMuteSoundClass(SoundClassAsset.AssetName);
				}
			}
		}
#endif
	}

	static void ExecuteSolo(const FToolMenuContext& InContext)
	{
#if ENABLE_ATOM_DEBUG
		if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
		{
			if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
			{
				for (const FAssetData& SoundClassAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundClass::StaticClass()))
				{
					RuntimeManager->GetDebugger().ToggleSoloSoundClass(SoundClassAsset.AssetName);
				}
			}
		}
#endif
	}

	static ECheckBoxState IsActionCheckedMute(const FToolMenuContext& InContext)
	{
#if ENABLE_ATOM_DEBUG
		if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
		{
			if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
			{
				for (const FAssetData& SoundClassAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundClass::StaticClass()))
				{
					if (RuntimeManager->GetDebugger().IsMuteSoundClass(SoundClassAsset.AssetName))
					{
						return ECheckBoxState::Checked;
					}
				}
			}
		}
#endif
		return ECheckBoxState::Unchecked;
	}

	static ECheckBoxState IsActionCheckedSolo(const FToolMenuContext& InContext)
	{
#if ENABLE_ATOM_DEBUG
		if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
		{
			if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
			{
				for (const FAssetData& SoundClassAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundClass::StaticClass()))
				{
					if (RuntimeManager->GetDebugger().IsSoloSoundClass(SoundClassAsset.AssetName))
					{
						return ECheckBoxState::Checked;
					}
				}
			}
		}
#endif
		return ECheckBoxState::Unchecked;
	}

	static bool CanExecuteMuteCommand(const FToolMenuContext& InContext)
	{
#if ENABLE_ATOM_DEBUG
		if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
		{
			if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
			{
				// Allow muting if we're not Soloing.
				for (const FAssetData& SoundClassAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundClass::StaticClass()))
				{
					if (RuntimeManager->GetDebugger().IsSoloSoundClass(SoundClassAsset.AssetName))
					{
						return false;
					}
				}
				// Ok.
				return true;
			}
		}
#endif
		return false;
	}

	static bool CanExecuteSoloCommand(const FToolMenuContext& InContext)
	{
#if ENABLE_ATOM_DEBUG
		if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
		{
			if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
			{
				// Allow Soloing if we're not Muting.
				for (const FAssetData& SoundClassAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundClass::StaticClass()))
				{
					if (RuntimeManager->GetDebugger().IsMuteSoundClass(SoundClassAsset.AssetName))
					{
						return false;
					}
				}
			
				// Ok.
				return true;
			}
		}
#endif
		return false;
	}

	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, [] {
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
		{
			FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
			UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UAtomSoundClass::StaticClass());
			FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

			Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
			{
				{
					const TAttribute<FText> Label = LOCTEXT("AtomSound_MuteSoundClass", "Mute");
					const TAttribute<FText> ToolTip = LOCTEXT("AtomSound_MuteSoundClassSoundTooltip", "Mutes anything using this AtomClass.");
					const FSlateIcon Icon = FSlateIcon(FAppStyle::Get().GetStyleSetName(), "MediaAsset.AssetActions.Mute.Small");
					FToolUIAction UIAction;
					UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteMute);
					UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateStatic(&CanExecuteMuteCommand);
					UIAction.GetActionCheckState = FToolMenuGetActionCheckState::CreateStatic(&IsActionCheckedMute);

					
				}
				{
					const TAttribute<FText> Label = LOCTEXT("AtomSound_SoloSoundClass", "Solo");
					const TAttribute<FText> ToolTip = LOCTEXT("AtomSound_SoloSoundClassSoundTooltip", "Mutes anything except sounds using this AtomClass.");
					const FSlateIcon Icon = FSlateIcon(FAppStyle::Get().GetStyleSetName(), "MediaAsset.AssetActions.Solo.Small");
					FToolUIAction UIAction;
					UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteSolo);
					UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateStatic(&CanExecuteSoloCommand);
					UIAction.GetActionCheckState = FToolMenuGetActionCheckState::CreateStatic(&IsActionCheckedSolo);

					InSection.AddMenuEntry("AtomSound_SoloSoundClass", Label, ToolTip, Icon, UIAction, EUserInterfaceActionType::ToggleButton);
				}
			}));
		}));
	});
} // namespace

#undef LOCTEXT_NAMESPACE
