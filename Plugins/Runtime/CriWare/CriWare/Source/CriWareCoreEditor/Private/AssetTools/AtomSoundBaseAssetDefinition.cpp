/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundBaseDefinition.cpp
 *
 ****************************************************************************/

#include "AssetTools/AtomSoundBaseAssetDefinition.h"

#include "ToolMenus.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#include "CriWareEditor.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomDebug.h"
#include "Atom/AtomComponent.h"
#include "Atom/AtomSoundCue.h"
#include "Atom/AtomSoundWave.h"
#include "Atom/AtomSoundProxy.h"

#include "AtomDefaults.h"
#include "AtomEditorStyle.h"
#include "Factories/AtomSoundProxyFactory.h"

#define LOCTEXT_NAMESPACE "AtomSoundBaseDefintion"

namespace CriWare::AtomEditor
{
	void StopSound()
	{
		GCriWareEditor->ResetPreviewAtomComponent();
	}

	void PlaySound(UAtomSoundBase* Sound)
	{
		if (Sound)
		{
			GCriWareEditor->PlayPreviewSound(Sound);
		}
		else
		{
			StopSound();
		}
	}

	bool IsSoundPlaying(UAtomSoundBase* Sound)
	{
		UAtomComponent* PreviewComp = GCriWareEditor->GetPreviewAtomComponent();
		return PreviewComp && PreviewComp->Sound == Sound && PreviewComp->IsPlaying();
	}

	bool IsSoundPlaying(const FAssetData& AssetData)
	{
		const UAtomComponent* PreviewComp = GCriWareEditor->GetPreviewAtomComponent();
		if (PreviewComp && PreviewComp->Sound && PreviewComp->IsPlaying())
		{
			if (PreviewComp->Sound->GetFName() == AssetData.AssetName)
			{
				if (PreviewComp->Sound->GetOutermost()->GetFName() == AssetData.PackageName)
				{
					return true;
				}
			}
		}

		return false;
	}
} // namespace

namespace MenuExtension_AtomSoundBase
{
	static void GetAssetActions(FToolMenuSection& Section);
}

void UAtomSoundBaseAssetDefinition::GetAssetActions(FToolMenuSection& Section) const
{
	MenuExtension_AtomSoundBase::GetAssetActions(Section);
}

EAssetCommandResult UAtomSoundBaseAssetDefinition::ActivateSoundBase(const FAssetActivateArgs& ActivateArgs)
{
	if (ActivateArgs.ActivationMethod == EAssetActivationMethod::Previewed)
	{
		if (UAtomSoundBase* TargetSound = ActivateArgs.LoadFirstValid<UAtomSoundBase>())
		{
			UAtomComponent* PreviewComp = GCriWareEditor->GetPreviewAtomComponent();
			if (PreviewComp && PreviewComp->IsPlaying())
			{
				// Already previewing a sound, if it is the target cue then stop it, otherwise play the new one
				if (!TargetSound || PreviewComp->Sound == TargetSound)
				{
					CriWare::AtomEditor::StopSound();
				}
				else
				{
					CriWare::AtomEditor::PlaySound(TargetSound);
				}
			}
			else
			{
				// Not already playing, play the target sound cue if it exists
				CriWare::AtomEditor::PlaySound(TargetSound);
			}
			return EAssetCommandResult::Handled;
		}
	}
	return EAssetCommandResult::Unhandled;
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
bool UAtomSoundBaseAssetDefinition::GetThumbnailActionOverlay(const FAssetData& InAssetData, FAssetActionThumbnailOverlayInfo& OutActionOverlayInfo) const
{
	auto OnGetDisplayBrushLambda = [InAssetData]() -> const FSlateBrush*
		{
			if (CriWare::AtomEditor::IsSoundPlaying(InAssetData))
			{
				return FAppStyle::GetBrush("ContentBrowser.AssetAction.StopIcon");
			}

			return FAppStyle::GetBrush("ContentBrowser.AssetAction.PlayIcon");
		};

	OutActionOverlayInfo.ActionImageWidget = SNew(SImage).Image_Lambda(OnGetDisplayBrushLambda);

	auto OnToolTipTextLambda = [InAssetData]() -> FText
		{
			if (CriWare::AtomEditor::IsSoundPlaying(InAssetData))
			{
				return LOCTEXT("Thumbnail_StopSoundToolTip", "Stop selected sound");
			}

			return LOCTEXT("Thumbnail_PlaySoundToolTip", "Play selected sound");
		};

	auto OnClickedLambda = [InAssetData]() -> FReply
		{
			if (CriWare::AtomEditor::IsSoundPlaying(InAssetData))
			{
				CriWare::AtomEditor::StopSound();
			}
			else
			{
				// Load and play sound
				CriWare::AtomEditor::PlaySound(Cast<UAtomSoundBase>(InAssetData.GetAsset()));
			}
			return FReply::Handled();
		};

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	OutActionOverlayInfo.ActionButtonArgs = SButton::FArguments()
		.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
		.ContentPadding(0.0f)
		.ToolTipText_Lambda(OnToolTipTextLambda)
		.OnClicked_Lambda(OnClickedLambda)
		[
			SNew(SImage)
				.Image_Lambda(OnGetDisplayBrushLambda)
		];
#else
	OutActionOverlayInfo.ActionButtonWidget = SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
		.ContentPadding(0.0f)
		.ToolTipText_Lambda(OnToolTipTextLambda)
		.OnClicked_Lambda(OnClickedLambda)
		[
			SNew(SImage)
				.Image_Lambda(OnGetDisplayBrushLambda)
		];
#endif
	return true;
}
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
void UAtomSoundBaseAssetDefinition::GetSoundBaseAssetActionButtonExtensions(const FAssetData& InAssetData, TArray<FAssetButtonActionExtension>& OutExtensions)
{
	FAssetButtonActionExtension AssetButtonActionExtension
	{
		.PickTooltipAttribute = TAttribute<FText>::CreateLambda([InAssetData]() -> const FText
		{
			if (CriWare::AtomEditor::IsSoundPlaying(InAssetData))
			{
				return LOCTEXT("SoundAudition_PlaySoundToolTip", "Stop selected sound");
			}

			return LOCTEXT("SoundAudition_StopSoundToolTip", "Play selected sound");
		}),
		.PickBrushAttribute = TAttribute<const FSlateBrush*>::CreateLambda([InAssetData]() -> const FSlateBrush* {

			if (CriWare::AtomEditor::IsSoundPlaying(InAssetData))
			{
				return FAppStyle::GetBrush("MediaAsset.AssetActions.Stop.Small");
			}

			return FAppStyle::GetBrush("MediaAsset.AssetActions.Play.Small");
		}),
		.OnClicked = FOnClicked::CreateLambda([InAssetData]() -> FReply
			{
				if (CriWare::AtomEditor::IsSoundPlaying(InAssetData))
				{
					CriWare::AtomEditor::StopSound();
				}
				else
				{
					// Load and play sound
					CriWare::AtomEditor::PlaySound(Cast<UAtomSoundBase>(InAssetData.GetAsset()));
				}

			return FReply::Handled();
		})
	};

	OutExtensions.Add(AssetButtonActionExtension);
}
#endif

EAssetCommandResult UAtomSoundBaseAssetDefinition::ActivateAssets(const FAssetActivateArgs& ActivateArgs) const
{
	if (ActivateSoundBase(ActivateArgs) == EAssetCommandResult::Handled)
	{
		return EAssetCommandResult::Handled;
	}
	return Super::ActivateAssets(ActivateArgs);
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
void UAtomSoundBaseAssetDefinition::GetAssetActionButtonExtensions(const FAssetData& InAssetData, TArray<FAssetButtonActionExtension>& OutExtensions) const
{
	UAtomSoundBaseAssetDefinition::GetSoundBaseAssetActionButtonExtensions(InAssetData, OutExtensions);
}
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
TSharedPtr<SWidget> UAtomSoundBaseAssetDefinition::GetThumbnailOverlay(const FAssetData& InAssetData) const
{
	auto OnClickedLambda = [InAssetData]() -> FReply
		{
			if (CriWare::AtomEditor::IsSoundPlaying(InAssetData))
			{
				CriWare::AtomEditor::StopSound();
			}
			else
			{
				// Load and play sound
				CriWare::AtomEditor::PlaySound(Cast<UAtomSoundBase>(InAssetData.GetAsset()));
			}
			return FReply::Handled();
		};

	auto Box = GetSoundBaseThumbnailOverlay(InAssetData, MoveTemp(OnClickedLambda));

	// draw the base thumbnail overlay under this overlay if enabled 
	if (GetIconOverlayVisibility(InAssetData) == EVisibility::Visible)
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				Super::GetThumbnailOverlay(InAssetData).ToSharedRef()
			]
			+ SOverlay::Slot()
			[
				Box.ToSharedRef()
			];
	}

	return Box;
}

TSharedPtr<SWidget> UAtomSoundBaseAssetDefinition::GetSoundBaseThumbnailOverlay(const FAssetData& AssetData, TFunction<FReply()>&& OnClicked)
{
	auto OnGetDisplayBrushLambda = [AssetData]() -> const FSlateBrush*
		{
			if (CriWare::AtomEditor::IsSoundPlaying(AssetData))
			{
				return FAppStyle::GetBrush("MediaAsset.AssetActions.Stop.Large");
			}

			return FAppStyle::GetBrush("MediaAsset.AssetActions.Play.Large");
		};

	auto OnClickedLambda = MoveTemp(OnClicked);
	if (!OnClickedLambda)
	{
		OnClickedLambda = [AssetData]() -> FReply
		{
			if (CriWare::AtomEditor::IsSoundPlaying(AssetData))
			{
				CriWare::AtomEditor::StopSound();
			}
			else
			{
				// Load and play sound
				CriWare::AtomEditor::PlaySound(Cast<UAtomSoundBase>(AssetData.GetAsset()));
			}
			return FReply::Handled();
		};
	}

	auto OnToolTipTextLambda = [AssetData]() -> FText
	{
		if (CriWare::AtomEditor::IsSoundPlaying(AssetData))
		{
			return LOCTEXT("AtomThumbnail_StopSoundToolTip", "Stop selected sound");
		}

		return LOCTEXT("AtomThumbnail_PlaySoundToolTip", "Play selected sound");
	};

	TSharedPtr<SBox> Box;
	SAssignNew(Box, SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(FMargin(2));

	auto OnGetVisibilityLambda = [Box, AssetData]() -> EVisibility
		{
			if (Box.IsValid() && (Box->IsHovered() || CriWare::AtomEditor::IsSoundPlaying(AssetData)))
			{
				return EVisibility::Visible;
			}

			return EVisibility::Hidden;
		};

	TSharedPtr<SButton> Widget;
	SAssignNew(Widget, SButton)
		.ButtonStyle(FAppStyle::Get(), "HoverHintOnly")
		.ToolTipText_Lambda(OnToolTipTextLambda)
		.Cursor(EMouseCursor::Default) // The outer widget can specify a DragHand cursor, so we need to override that here
		.ForegroundColor(FSlateColor::UseForeground())
		.IsFocusable(false)
		.OnClicked_Lambda(OnClickedLambda)
		.Visibility_Lambda(OnGetVisibilityLambda)
		[
			SNew(SImage)
			.Image_Lambda(OnGetDisplayBrushLambda)
		];

	Box->SetContent(Widget.ToSharedRef());
	Box->SetVisibility(EVisibility::Visible);

	return Box;
}
#endif

void UAtomSoundBaseAssetDefinition::ExecutePlaySound(const FToolMenuContext& InContext)
{
	if (UAtomSoundBase* Sound = UContentBrowserAssetContextMenuContext::LoadSingleSelectedAsset<UAtomSoundBase>(InContext))
	{
		// Only play the first valid sound
		CriWare::AtomEditor::PlaySound(Sound);
	}
}

void UAtomSoundBaseAssetDefinition::ExecuteStopSound(const FToolMenuContext& InContext)
{
	CriWare::AtomEditor::StopSound();
}

bool UAtomSoundBaseAssetDefinition::CanExecutePlayCommand(const FToolMenuContext& InContext)
{
	if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
	{
		return CBContext->SelectedAssets.Num() == 1;
	}

	return false;
}

void UAtomSoundBaseAssetDefinition::ExecuteMuteSound(const FToolMenuContext& InContext)
{
#if ENABLE_ATOM_DEBUG
	if (FAtomRuntimeManager* RuntimeManager = GCriWareEditor->GetAtomRuntimeManager())
	{
		if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
		{
		Atom::FAtomDebugger& Debugger = RuntimeManager->GetDebugger();

		// In a selection that consists of some already muted, toggle everything in the same direction,
		// to avoid AB problem.
		const bool bAnyMuted = IsActionCheckedMute(InContext) == ECheckBoxState::Checked;

		for (const FAssetData& SoundCueAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundCue::StaticClass()))
		{
			Debugger.SetMuteSoundCue(SoundCueAsset.AssetName, !bAnyMuted);
		}

		for (const FAssetData& SoundWaveAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundWave::StaticClass()))
		{
			Debugger.SetMuteSoundWave(SoundWaveAsset.AssetName, !bAnyMuted);
		}
		}
	}
#endif
}

void UAtomSoundBaseAssetDefinition::ExecuteSoloSound(const FToolMenuContext& InContext)
{
#if ENABLE_ATOM_DEBUG
	if (FAtomRuntimeManager* RuntimeManager = GCriWareEditor->GetAtomRuntimeManager())
	{
		if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
		{
			Atom::FAtomDebugger& Debugger = RuntimeManager->GetDebugger();

			// In a selection that consists of some already soloed, toggle everything in the same direction,
			// to avoid AB problem.
			const bool bAnySoloed = IsActionCheckedSolo(InContext) == ECheckBoxState::Checked;

			for (const FAssetData& SoundCueAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundCue::StaticClass()))
			{
				Debugger.SetSoloSoundCue(SoundCueAsset.AssetName, !bAnySoloed);
			}

			for (const FAssetData& SoundWaveAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundWave::StaticClass()))
			{
				Debugger.SetSoloSoundWave(SoundWaveAsset.AssetName, !bAnySoloed);
			}
		}
	}
#endif
}

ECheckBoxState UAtomSoundBaseAssetDefinition::IsActionCheckedMute(const FToolMenuContext& InContext)
{
#if ENABLE_ATOM_DEBUG
	if (FAtomRuntimeManager* RuntimeManager = GCriWareEditor->GetAtomRuntimeManager())
	{
		if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
		{
			// If *any* of the selection are muted, show the tick box as ticked.
			Atom::FAtomDebugger& Debugger = RuntimeManager->GetDebugger();
			for (const FAssetData& SoundCueAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundCue::StaticClass()))
			{
				if (Debugger.IsMuteSoundCue(SoundCueAsset.AssetName))
				{
					return ECheckBoxState::Checked;
				}
			}

			for (const FAssetData& SoundWaveAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundWave::StaticClass()))
			{
				if (Debugger.IsMuteSoundWave(SoundWaveAsset.AssetName))
				{
					return ECheckBoxState::Checked;
				}
			}
		}
	}
#endif
	return ECheckBoxState::Unchecked;
}

ECheckBoxState UAtomSoundBaseAssetDefinition::IsActionCheckedSolo(const FToolMenuContext& InContext)
{
#if ENABLE_ATOM_DEBUG
	if (FAtomRuntimeManager* RuntimeManager = GCriWareEditor->GetAtomRuntimeManager())
	{
		if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
		{
			// If *any* of the selection are solod, show the tick box as ticked.
			Atom::FAtomDebugger& Debugger = RuntimeManager->GetDebugger();
			for (const FAssetData& SoundCueAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundCue::StaticClass()))
			{
				if (Debugger.IsSoloSoundCue(SoundCueAsset.AssetName))
				{
					return ECheckBoxState::Checked;
				}
			}

			for (const FAssetData& SoundWaveAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundWave::StaticClass()))
			{
				if (Debugger.IsSoloSoundWave(SoundWaveAsset.AssetName))
				{
					return ECheckBoxState::Checked;
				}
			}
		}
	}
#endif
	return ECheckBoxState::Unchecked;
}

bool UAtomSoundBaseAssetDefinition::CanExecuteMuteCommand(const FToolMenuContext& InContext)
{
#if ENABLE_ATOM_DEBUG
	if (FAtomRuntimeManager* RuntimeManager = GCriWareEditor->GetAtomRuntimeManager())
	{
		if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
		{
			// Allow muting if we're not Soloing.
			Atom::FAtomDebugger& Debugger = RuntimeManager->GetDebugger();
			
			for (const FAssetData& SoundCueAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundCue::StaticClass()))
			{
				if (Debugger.IsSoloSoundCue(SoundCueAsset.AssetName))
				{
					return false;
				}
			}

			for (const FAssetData& SoundWaveAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundWave::StaticClass()))
			{
				if (Debugger.IsSoloSoundWave(SoundWaveAsset.AssetName))
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

bool UAtomSoundBaseAssetDefinition::CanExecuteSoloCommand(const FToolMenuContext& InContext)
{
#if ENABLE_ATOM_DEBUG
	if (FAtomRuntimeManager* RuntimeManager = GCriWareEditor->GetAtomRuntimeManager())
	{
		if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
		{
			// Allow Soloing if we're not Muting.
			Atom::FAtomDebugger& Debugger = RuntimeManager->GetDebugger();

			for (const FAssetData& SoundCueAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundCue::StaticClass()))
			{
				if (Debugger.IsMuteSoundCue(SoundCueAsset.AssetName))
				{
					return false;
				}
			}

			for (const FAssetData& SoundWaveAsset : CBContext->GetSelectedAssetsOfType(UAtomSoundWave::StaticClass()))
			{
				if (Debugger.IsMuteSoundWave(SoundWaveAsset.AssetName))
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

namespace MenuExtension_AtomSoundBase
{
	/** Creates a unique package and asset name taking the form InBasePackageName+InSuffix */
	static void CreateUniqueAssetName(const FString& InBasePackageName, const FString& InSuffix, FString& OutPackageName, FString& OutAssetName)
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(InBasePackageName, InSuffix, OutPackageName, OutAssetName);
	}

	static bool CanExecuteCreateSoundProxy(const FToolMenuContext& MenuContext)
	{
		if (UContentBrowserAssetContextMenuContext::GetNumAssetsSelected(MenuContext) == 1)
		{
			return true;
		}

		return false;
	}

	static void ExecuteCreateSoundProxy(const FToolMenuContext& MenuContext)
	{
		const FString DefaultSuffix = TEXT("_AtomSoundProxy");

		TArray<UAtomSoundBase*> Sounds;

		if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext))
		{
			Sounds = Context->LoadSelectedObjects<UAtomSoundBase>();
		}

		for (auto Sound : Sounds)
		{
			if (Sound)
			{
				FString Suffix = Sound->GetName() + DefaultSuffix;
				// Determine an appropriate name
				FString Name;
				FString PackagePath;
				CreateUniqueAssetName(Sound->GetOutermost()->GetName(), Suffix, PackagePath, Name);

				// Create the factory used to generate the asset
				UAtomSoundProxyFactory* Factory = NewObject<UAtomSoundProxyFactory>();
				Factory->Sound = Sound;

				FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UAtomSoundProxy::StaticClass(), Factory);
			}
		}
	}

	static void GetAssetActions(FToolMenuSection& InSection)
	{
		if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InSection))
		{
			if (Context->SelectedAssets.Num() > 0)
			{
				//const FAssetData& AssetData = Context->SelectedAssets[0];
				//if (AssetData.AssetClassPath == UAtomSoundWave::StaticClass()->GetClassPathName() || AssetData.AssetClassPath == UAtomSoundCue::StaticClass()->GetClassPathName())
				//{
				{
					const TAttribute<FText> Label = LOCTEXT("AtomSound_PlaySound", "Play");
					const TAttribute<FText> ToolTip = LOCTEXT("AtomSound_PlaySoundTooltip", "Plays the selected sound.");
					const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "MediaAsset.AssetActions.Play.Small");

					FToolUIAction UIAction;
					UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&UAtomSoundBaseAssetDefinition::ExecutePlaySound);
					UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateStatic(&UAtomSoundBaseAssetDefinition::CanExecutePlayCommand);
					InSection.AddMenuEntry("AtomSound_PlaySound", Label, ToolTip, Icon, UIAction);
				}
				{
					const TAttribute<FText> Label = LOCTEXT("AtomSound_StopSound", "Stop");
					const TAttribute<FText> ToolTip = LOCTEXT("AtomSound_StopSoundTooltip", "Stops the selected sounds.");
					const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "MediaAsset.AssetActions.Stop.Small");

					FToolUIAction UIAction;
					UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&UAtomSoundBaseAssetDefinition::ExecuteStopSound);
					InSection.AddMenuEntry("AtomSound_StopSound", Label, ToolTip, Icon, UIAction);
				}
				{
					const TAttribute<FText> Label = LOCTEXT("AtomSound_MuteSound", "Mute");
					const TAttribute<FText> ToolTip = LOCTEXT("AtomSound_MuteSoundTooltip", "Mutes the selected sounds.");
					const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "MediaAsset.AssetActions.Mute.Small");

					FToolUIAction UIAction;
					UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&UAtomSoundBaseAssetDefinition::ExecuteMuteSound);
					UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateStatic(&UAtomSoundBaseAssetDefinition::CanExecuteMuteCommand);
					UIAction.GetActionCheckState = FToolMenuGetActionCheckState::CreateStatic(&UAtomSoundBaseAssetDefinition::IsActionCheckedMute);
					InSection.AddMenuEntry("AtomSound_SoundMute", Label, ToolTip, Icon, UIAction, EUserInterfaceActionType::ToggleButton);
				}
				{
					const TAttribute<FText> Label = LOCTEXT("AtomSound_SoloSound", "Solo");
					const TAttribute<FText> ToolTip = LOCTEXT("AtomSound_SoloSoundTooltip", "Solos the selected sounds.");
					const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "MediaAsset.AssetActions.Solo.Small");

					FToolUIAction UIAction;
					UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&UAtomSoundBaseAssetDefinition::ExecuteSoloSound);
					UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateStatic(&UAtomSoundBaseAssetDefinition::CanExecuteSoloCommand);
					UIAction.GetActionCheckState = FToolMenuGetActionCheckState::CreateStatic(&UAtomSoundBaseAssetDefinition::IsActionCheckedSolo);
					InSection.AddMenuEntry("AtomSound_StopSolo", Label, ToolTip, Icon, UIAction, EUserInterfaceActionType::ToggleButton);
				}
				{
					const TAttribute<FText> Label = LOCTEXT("GenerateThumbnail", "Generate Thumbnail");
					const TAttribute<FText> ToolTip = LOCTEXT("GenerateThumbnail_Tooltip", "Generate a thumbnail for this asset.");
					const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "AssetEditor.SaveThumbnail");

					FToolUIAction UIAction;
					UIAction.ExecuteAction = FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& InContext)
						{
							if (const UContentBrowserAssetContextMenuContext* CBContext = UContentBrowserAssetContextMenuContext::FindContextWithAssets(InContext))
							{
								for (auto Obj : CBContext->LoadSelectedObjectsIfNeeded())
								{
									if (auto Sound = Cast<UAtomSoundBase>(Obj))
									{
										Sound->GenerateWaveSnapshot();
										Sound->MarkPackageDirty();
									}
								}
							}
						});
					InSection.AddMenuEntry("Atom_GenerateThumbnail", Label, ToolTip, Icon, UIAction);
				}
				{
					const TAttribute<FText> Label = LOCTEXT("AtomSound_CreateSoundProxy", "Create Atom Sound Proxy");
					const TAttribute<FText> ToolTip = Context->SelectedAssets.Num() == 1
						? LOCTEXT("AtomSound_CreateSoundProxySingleWaveTooltip", "Creates a sound proxy referencing this sound.")
						: LOCTEXT("AtomSound_CreateSoundProxyMultiWavesTooltip", "Creates sound proxies referencing these sounds.");
					const FSlateIcon Icon = FSlateIcon(FAtomEditorStyle::Get().GetStyleSetName(), "ClassIcon.AtomSoundCue");

					FToolUIAction UIAction;
					UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteCreateSoundProxy);
					UIAction.CanExecuteAction = FToolMenuCanExecuteAction::CreateStatic(&CanExecuteCreateSoundProxy);
					InSection.AddMenuEntry("AtomSound_CreateSoundProxy", Label, ToolTip, Icon, UIAction);
				}
				//}
			}
		}
	}

	static void RegisterAssetActions(UClass* InClass)
	{
		UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(InClass);
		FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

		Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
		{	
			GetAssetActions(InSection);
		}));
	}

	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, [] {
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
			{
				FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);

				// Note: we can't do this with UAtomSoundBase because USoundSource and others may need to do special actions on these actions.
				// Since these actions are registered via delayed static callbacks, there's not a clean why to do this with inheritance.
				//RegisterAssetActions(UAtomSoundCue::StaticClass());
				//RegisterAssetActions(UAtomSoundWave::StaticClass());
				//RegisterAssetActions(UMySound::StaticClass());
				
				RegisterAssetActions(UAtomSoundBase::StaticClass());
			}));
		});
} // namespace

#undef LOCTEXT_NAMESPACE
