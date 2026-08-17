/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomSoundWaveAssetDefinitions.cpp
 *
 ****************************************************************************/

#include "AtomSoundWaveAssetDefinition.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "Misc/Attribute.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Images/SImage.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
#include "AssetDefinitionAssetInfo.h"
#include "IAssetStatusInfoProvider.h"
#endif

#include "Atom/AtomSoundSimple.h"

#include "AtomEditorStyle.h"
#include "Factories/AtomSoundSimpleFactory.h"

#define LOCTEXT_NAMESPACE "AtomSoundWaveDefinition"

/*
 * Atom Sound Wave Asset Definition
 *****************************************************************************/

namespace MenuExtension_AtomSoundWave
{
	static void GetAssetActions(FToolMenuSection& Section);
}

void UAtomSoundWaveAssetDefinition::GetAssetActions(FToolMenuSection& Section) const
{
	Super::GetAssetActions(Section);

	MenuExtension_AtomSoundWave::GetAssetActions(Section);
}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6

void UAtomSoundWaveAssetDefinition::GetAssetStatusInfo(const TSharedPtr<IAssetStatusInfoProvider>& InAssetStatusInfoProvider, TArray<FAssetDisplayInfo>& OutStatusInfo) const
{
	Super::GetAssetStatusInfo(InAssetStatusInfoProvider, OutStatusInfo);

	const ISlateStyle* AtomEditorStyle = FSlateStyleRegistry::FindSlateStyle(FAtomEditorStyle::Get().GetStyleSetName());
	if (ensure(AtomEditorStyle))
	{
		FAssetDisplayInfo LoopStatus;
		LoopStatus.StatusIcon = AtomEditorStyle->GetBrush("CriWare.LoopIcon");
		LoopStatus.Priority = FAssetStatusPriority(EStatusSeverity::Info, 1);
		LoopStatus.StatusDescription = LOCTEXT("UAtomSoundWaveAssetDefinition_LoopIcon_ToolTip", "This Wave is looping.");
		LoopStatus.IsVisible = MakeAttributeUObject(this, &UAtomSoundWaveAssetDefinition::GetThumbnailLoopStatusVisibility, InAssetStatusInfoProvider);
		OutStatusInfo.Add(LoopStatus);
	}
}

EVisibility UAtomSoundWaveAssetDefinition::GetThumbnailLoopStatusVisibility(const TSharedPtr<IAssetStatusInfoProvider> InAssetStatusInfoProvider) const
{
	const FAssetData AssetData = InAssetStatusInfoProvider->TryGetAssetData();
	if (AssetData.IsValid())
	{
		FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("IsOneShot"));
		if (AssetTagValue.IsSet())
		{
			return AssetTagValue.GetValue() == TEXT("false") ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}
	return EVisibility::Collapsed;
}

#else

TSharedPtr<SWidget> UAtomSoundWaveAssetDefinition::GetSoundWaveThumbnailOverlay(const FAssetData& AssetData)
{
	const ISlateStyle* AtomEditorStyle = FSlateStyleRegistry::FindSlateStyle(FAtomEditorStyle::Get().GetStyleSetName());
	if (ensure(AtomEditorStyle))
	{
		// From UE 5.3 we can use the right side before the place was used by Source Control's status overlay.
		const EHorizontalAlignment HAlign = (FEngineVersion::Current().GetMajor() == 5 && FEngineVersion::Current().GetMinor() >= 3) ? HAlign_Right : HAlign_Left;
		const FMargin Padding = HAlign == HAlign_Right ? FMargin(0.0f, 3.0f, 3.0f, 0.0f) : FMargin(3.0f, 3.0f, 0.0f, 0.0f);
		const FSlateBrush* LoopIcon = AtomEditorStyle->GetBrush("CriWare.LoopIcon");

		auto OnGetLoopOverlayVisibilityLambda = [AssetData]() -> EVisibility
		{
			FAssetTagValueRef AssetTagValue = AssetData.TagsAndValues.FindTag(TEXT("IsOneShot"));
			if (AssetTagValue.IsSet())
			{
				return AssetTagValue.GetValue() == TEXT("false") ? EVisibility::Visible : EVisibility::Collapsed;
			}

			return EVisibility::Hidden;
		};

		return SNew(SWrapBox)
			+ SWrapBox::Slot()
			.HAlign(HAlign)
			.VAlign(VAlign_Top)
			[
				// Looping
				SNew(SBorder)
				.BorderImage(FAppStyle::GetNoBrush())
				.Visibility_Lambda(OnGetLoopOverlayVisibilityLambda)
				.Padding(Padding)
				.HAlign(HAlign)
				.VAlign(VAlign_Top)
				[
					SNew(SImage)
					.ToolTipText(LOCTEXT("UAtomSoundWaveAssetDefinition_LoopIcon_ToolTip", "This Wave is looping."))
					.Image(LoopIcon)
				]
			];
	}

	return SNullWidget::NullWidget;
}

TSharedPtr<SWidget> UAtomSoundWaveAssetDefinition::GetThumbnailOverlay(const FAssetData& AssetData) const
{
	TSharedPtr<SWidget> PreviewOverlay = UAtomSoundBaseAssetDefinition::GetThumbnailOverlay(AssetData);

	const ISlateStyle* AtomEditorStyle = FSlateStyleRegistry::FindSlateStyle("AtomEditorStyle");
	if (ensure(AtomEditorStyle))
	{
		// From UE 5.3 we can use the right side before the place was used by Source Control's status overlay.
		const EHorizontalAlignment HAlign = (FEngineVersion::Current().GetMajor() == 5 && FEngineVersion::Current().GetMinor() >= 3) ? HAlign_Right : HAlign_Left;

		return SNew(SOverlay)
			+SOverlay::Slot()
			.HAlign(HAlign)
			.VAlign(VAlign_Top)
			[
				GetSoundWaveThumbnailOverlay(AssetData).ToSharedRef()
			]
			+SOverlay::Slot()
			[
				PreviewOverlay.ToSharedRef()
			];
	}

	return PreviewOverlay;
}

#endif

// Menu Extensions
//--------------------------------------------------------------------
namespace MenuExtension_AtomSoundWave
{
	/** Creates a unique package and asset name taking the form InBasePackageName+InSuffix */
	static void CreateUniqueAssetName(const FString& InBasePackageName, const FString& InSuffix, FString& OutPackageName, FString& OutAssetName)
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(InBasePackageName, InSuffix, OutPackageName, OutAssetName);
	}

	static void ExecuteCreateSimpleSound(const FToolMenuContext& MenuContext)
	{
		const FString DefaultSuffix = TEXT("_AtomSimpleSound");

		TArray<UAtomSoundWave*> SoundWaves;

		if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext))
		{
			SoundWaves = Context->LoadSelectedObjects<UAtomSoundWave>();
		}


		if (SoundWaves.Num() > 0)
		{
			// Determine an appropriate name
			FString Name;
			FString PackagePath;
			CreateUniqueAssetName(SoundWaves[0]->GetOutermost()->GetName(), DefaultSuffix, PackagePath, Name);

			// Create the factory used to generate the asset
			UAtomSoundSimpleFactory* Factory = NewObject<UAtomSoundSimpleFactory>();
			Factory->SoundWaves.Reset();
			for (auto WaveIt = SoundWaves.CreateIterator(); WaveIt; ++WaveIt)
			{
				if (UAtomSoundWave* Wave = *WaveIt)
				{
					Factory->SoundWaves.Add(Wave);
				}
			}

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UAtomSoundSimple::StaticClass(), Factory);
		}
	}

	static void GetAssetActions(FToolMenuSection& Section)
	{
		if (const UContentBrowserAssetContextMenuContext* Context = UContentBrowserAssetContextMenuContext::FindContextWithAssets(Section))
		{
			if (Context->SelectedAssets.Num() > 0)
			{
				const TAttribute<FText> Label = LOCTEXT("AtomSoundWave_CreateSimpleSound", "Create Atom Simple Sound");
				const TAttribute<FText> ToolTip = Context->SelectedAssets.Num() == 1
					? LOCTEXT("AtomSoundWave_CreateSimpleSoundSingleWaveTooltip", "Creates a simple sound using this sound wave.")
					: LOCTEXT("AtomSoundWave_CreateSimpleSoundMultiWavesTooltip", "Creates a simple sound using these sound waves.");
				const FSlateIcon Icon = FSlateIcon(FAtomEditorStyle::Get().GetStyleSetName(), "ClassIcon.AtomSoundSimple");

				FToolUIAction UIAction;
				UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&ExecuteCreateSimpleSound);
				Section.AddMenuEntry("AtomSoundWave_CreateSimpleSound", Label, ToolTip, Icon, UIAction);
			}
		}
	}

	static FDelayedAutoRegisterHelper DelayedAutoRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{ 
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
		{
			FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
			{
				UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UAtomSoundWave::StaticClass());		
				FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");
				Section.AddDynamicEntry(NAME_None, FNewToolMenuSectionDelegate::CreateLambda([](FToolMenuSection& InSection)
				{
					GetAssetActions(InSection);
				}));
			}
		}));
	});
}

#undef LOCTEXT_NAMESPACE
