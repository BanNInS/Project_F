/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCoreEditor
 * File     : AtomEditorStyle.cpp
 *
 ****************************************************************************/

#include "AtomEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Brushes/SlateBorderBrush.h"
#include "Brushes/SlateBoxBrush.h"
#include "Brushes/SlateImageBrush.h"
#include "Fonts/SlateFontInfo.h"
#include "Math/Vector2D.h"
#include "Misc/Paths.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"

///-------------------

// Preprocessor macro to make defining Atom icons simple...

// Icons from plugin

// CLASS_NAME - name of the class to make the icon for
// ICON_NAME - base-name of the icon to use. Not necessarily based off class name
#define SET_ATOM_ICON(CLASS_NAME, ICON_NAME) \
		Set( *FString::Printf(TEXT("ClassIcon.%s"), TEXT(#CLASS_NAME)), new FSlateImageBrush(RootToContentDir(FString::Printf(TEXT("Icons/AssetIcons/%s_16x.png"), TEXT(#ICON_NAME))), Icon16)); \
		Set( *FString::Printf(TEXT("ClassThumbnail.%s"), TEXT(#CLASS_NAME)), new FSlateImageBrush(RootToContentDir(FString::Printf(TEXT("Icons/AssetIcons/%s_64x.png"), TEXT(#ICON_NAME))), Icon64));

// Simpler version of SET_ATOM_ICON, assumes same name of icon png and class name
#define SET_ATOM_ICON_SIMPLE(CLASS_NAME) SET_ATOM_ICON(CLASS_NAME, CLASS_NAME)

#define SET_ATOM_ICON_SVG(CLASS_NAME, ICON_NAME) \
		Set( *FString::Printf(TEXT("ClassIcon.%s"), TEXT(#CLASS_NAME)), new FSlateVectorImageBrush(RootToContentDir(FString::Printf(TEXT("Starship/AssetIcons/%s_16.svg"), TEXT(#ICON_NAME))), Icon16)); \
		Set( *FString::Printf(TEXT("ClassThumbnail.%s"), TEXT(#CLASS_NAME)), new FSlateVectorImageBrush(RootToContentDir(FString::Printf(TEXT("Starship/AssetIcons/%s_64.svg"), TEXT(#ICON_NAME))), Icon64));

#define SET_ATOM_ICON_SVG_SIMPLE(CLASS_NAME) SET_ATOM_ICON_SVG(CLASS_NAME, CLASS_NAME)

// Icons from engine - temporary

// CLASS_NAME - name of the class to make the icon for
// ICON_NAME - base-name of the icon to use. Not necessarily based off class name
#define SET_ATOM_UE_ICON(CLASS_NAME, ICON_NAME) \
		Set( *FString::Printf(TEXT("ClassIcon.%s"), TEXT(#CLASS_NAME)), new FSlateImageBrush(FPaths::EngineContentDir() / FString::Printf(TEXT("Editor/Slate/Icons/AssetIcons/%s_16x.png"), TEXT(#ICON_NAME)), Icon16)); \
		Set( *FString::Printf(TEXT("ClassThumbnail.%s"), TEXT(#CLASS_NAME)), new FSlateImageBrush(FPaths::EngineContentDir() / FString::Printf(TEXT("Editor/Slate/Icons/AssetIcons/%s_64x.png"), TEXT(#ICON_NAME)), Icon64));

// Simpler version of SET_ATOM_ICON, assumes same name of icon png and class name
#define SET_ATOM_UE_ICON_SIMPLE(CLASS_NAME) SET_ATOM_UE_ICON(CLASS_NAME, CLASS_NAME)

#define SET_ATOM_UE_ICON_SVG(CLASS_NAME, ICON_NAME) \
		Set( *FString::Printf(TEXT("ClassIcon.%s"), TEXT(#CLASS_NAME)), new FSlateVectorImageBrush(FPaths::EngineContentDir() / FString::Printf(TEXT("Editor/Slate/Starship/AssetIcons/%s_16.svg"), TEXT(#ICON_NAME)), Icon16)); \
		Set( *FString::Printf(TEXT("ClassThumbnail.%s"), TEXT(#CLASS_NAME)), new FSlateVectorImageBrush(FPaths::EngineContentDir() / FString::Printf(TEXT("Editor/Slate/Starship/AssetIcons/%s_64.svg"), TEXT(#ICON_NAME)), Icon64));

#define SET_ATOM_UE_ICON_SVG_SIMPLE(CLASS_NAME) SET_ATOM_UE_ICON_SVG(CLASS_NAME, CLASS_NAME)

///-------------------

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define UE_IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(FPaths::EngineContentDir() / FString::Printf(TEXT("Editor/Slate/%s.png"), TEXT(RelativePath)), __VA_ARGS__)
#define IMAGE_BRUSH_SVG(RelativePath, ...) FSlateVectorImageBrush(RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)
#define BOX_BRUSH(RelativePath, ...) FSlateBoxBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BORDER_BRUSH(RelativePath, ...) FSlateBorderBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

FName FAtomEditorStyle::AtomStyleName = "AtomEditorStyle";

/* FAtomEditorStyle structors
 *****************************************************************************/

FAtomEditorStyle::FAtomEditorStyle()
	: FSlateStyleSet(AtomStyleName)
{
	const FVector2D Icon16(16.0f, 16.0f);
	const FVector2D Icon64(64.0f, 64.0f);
	const FVector2D Icon20(20.0f, 20.0f);
	const FVector2D Icon40(40.0f, 40.0f);

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CriWare"));
	if (Plugin.IsValid())
	{
		//SetContentRoot(FPaths::Combine(Plugin->GetContentDir(), TEXT("Icons")));
		SetContentRoot(Plugin->GetContentDir());
	}

	// buttons
	//Set("MediaPlayerEditor.SourceButton", new IMAGE_BRUSH("btn_source_12x", Icon12x12));
	//Set("MediaPlayerEditor.GoButton", new IMAGE_BRUSH("btn_go_12x", Icon12x12));
	//Set("MediaPlayerEditor.ReloadButton", new IMAGE_BRUSH("btn_reload_12x", Icon12x12));
	//Set("MediaPlayerEditor.SettingsButton", new IMAGE_BRUSH("btn_settings_16x", Icon12x12));
	
	// CriWare's Symbols
	Set("CriWare.ADXThumbnail", new IMAGE_BRUSH("Icons/ADX_symbol_64x", Icon64));
	Set("CriWare.ADXIcon", new IMAGE_BRUSH("Icons/ADX_symbol_16x", Icon16));

	// Overlay Icons
	Set("CriWare.LockIcon", new IMAGE_BRUSH_SVG("Starship/Icons/Lock", Icon16));
	Set("CriWare.LoopIcon", new IMAGE_BRUSH_SVG("Starship/Icons/Loop", Icon16));
	Set("CriWare.AisacIcon", new IMAGE_BRUSH_SVG("Starship/Icons/Aisac", Icon16));
	Set("CriWare.Sound3DIcon", new IMAGE_BRUSH_SVG("Starship/Icons/Sound3D", Icon16));
	Set("CriWare.PrivateIcon", new IMAGE_BRUSH_SVG("Starship/Icons/Private", Icon16));
	Set("CriWare.HeadphoneIcon", new IMAGE_BRUSH_SVG("Starship/Icons/Headphone", Icon16));

	// Atom Sound Class Editor Toolkit
	Set("AtomSoundClassEditor.Tabs.Properties", new UE_IMAGE_BRUSH("Icons/icon_tab_SelectionDetails_16x", Icon16));
	Set("AtomSoundClassEditor.ToggleSolo", new UE_IMAGE_BRUSH("Icons/icon_SCueEd_Solo_40x", Icon40));
	Set("AtomSoundClassEditor.ToggleSolo.Small", new UE_IMAGE_BRUSH("Icons/icon_SCueEd_Solo_40x", Icon20));
	Set("AtomSoundClassEditor.ToggleMute", new UE_IMAGE_BRUSH("Icons/icon_SCueEd_Mute_40x", Icon40));
	Set("AtomSoundClassEditor.ToggleMute.Small", new UE_IMAGE_BRUSH("Icons/icon_SCueEd_Mute_40x", Icon20));

	// Assets
	SET_ATOM_ICON_SVG_SIMPLE(AtomConfig);
	SET_ATOM_UE_ICON(AtomDspBusSetting, SoundMix);
	SET_ATOM_ICON_SVG_SIMPLE(AtomCueSheet);
	SET_ATOM_ICON_SVG_SIMPLE(AtomWaveBank);

	SET_ATOM_UE_ICON(AtomAttenuation, SoundAttenuation);
	SET_ATOM_UE_ICON(AtomConcurrency, SoundConcurrency);
	SET_ATOM_UE_ICON(AtomSoundClass, SoundClass);
	SET_ATOM_UE_ICON_SVG(AtomAmbientSound, AmbientSound);
	SET_ATOM_ICON_SVG_SIMPLE(AtomSoundCue);
	SET_ATOM_UE_ICON(AtomSoundWave, SoundGenericIcon_2);
	SET_ATOM_ICON_SVG_SIMPLE(AtomSoundSimple);
	SET_ATOM_ICON_SVG(AtomSoundProxy, AtomSoundCue);
	SET_ATOM_UE_ICON(AtomSoundSourceBus, SoundSourceBus);
	//SET_ATOM_UE_ICON_SIMPLE(SoundMix);
	//SET_ATOM_UE_ICON_SVG_SIMPLE(AudioVolume);
	SET_ATOM_UE_ICON(AtomBus, SoundSourceBus);
	SET_ATOM_ICON_SVG_SIMPLE(AtomRack);
	//SET_ATOM_UE_ICON_SIMPLE(ReverbEffect);

	SET_ATOM_ICON_SVG(AtomEndpointRack, AtomRack);
	SET_ATOM_ICON_SVG(AtomSoundfieldEndpointRack, AtomRack);
	SET_ATOM_ICON_SVG(AtomSoundfieldRack, AtomRack);
	SET_ATOM_ICON_SVG(AtomEndpointSettingsBase, AtomRackSettings);

	SET_ATOM_UE_ICON(AtomBusEffectPreset, SubmixEffectPreset);
	//SET_ATOM_UE_ICON(SoundEffectSourcePreset, SourceEffectPreset);
	//SET_ATOM_UE_ICON(SoundEffectSourcePresetChain, SourceEffectPresetChain_1);
	//SET_ATOM_UE_ICON(ModularSynthPresetBank, SoundGenericIcon_2);
	//SET_ATOM_UE_ICON(MonoWaveTableSynthPreset, SoundGenericIcon_2);
	//SET_ATOM_UE_ICON(TimeSynthClip, SoundGenericIcon_2);
	//SET_ATOM_UE_ICON(TimeSynthVolumeGroup, SoundGenericIcon_1);

	SET_ATOM_ICON(AtomAisacPatch, SoundModulationPatch);
	SET_ATOM_ICON(AtomModulationControlBus, SoundControlBus);
	SET_ATOM_ICON(AtomModulationGenerator, SoundModulationGenerator);
	SET_ATOM_ICON(AtomModulationControlBusMix, SoundControlBusMix);
	SET_ATOM_ICON(AtomModulationPatch, SoundModulationPatch);
	SET_ATOM_ICON(AtomModulationParameter, SoundModulationParameter);

	FSlateStyleRegistry::RegisterSlateStyle(*this);
}

FAtomEditorStyle::~FAtomEditorStyle()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*this);
}

const ISlateStyle& FAtomEditorStyle::Get()
{
	if (const ISlateStyle* AtomStyle = FSlateStyleRegistry::FindSlateStyle(AtomStyleName))
	{
		return *AtomStyle;
	}

	return FCoreStyle::GetCoreStyle();
}

#undef IMAGE_BRUSH
#undef IMAGE_BRUSH_SVG
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef DEFAULT_FONT
