
#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "ContentBrowserMenuContexts.h"

#include "Atom/AtomSoundBase.h"

#include "AtomDefaults.h"
#include "AssetTools/AtomAssetDefinition_Base.h"

#include "AtomSoundBaseAssetDefinition.generated.h"

namespace CriWare::AtomEditor
{
	CRIWARECOREEDITOR_API void StopSound();
	
	CRIWARECOREEDITOR_API void PlaySound(UAtomSoundBase* Sound);
	
	CRIWARECOREEDITOR_API bool IsSoundPlaying(UAtomSoundBase* Sound);
	CRIWARECOREEDITOR_API bool IsSoundPlaying(const FAssetData& AssetData);
}

UCLASS()
class CRIWARECOREEDITOR_API UAtomSoundBaseAssetDefinition : public UAtomAssetDefinition_Base
{
	GENERATED_BODY()

public:
	// UAssetDefinition Begin
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AtomSoundBase", "Atom Sound Base"); }
	virtual FLinearColor GetAssetColor() const override { return Atom::DefaultSoundBaseColor; }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UAtomSoundBase::StaticClass(); }
	virtual void GetAssetActions(FToolMenuSection& Section) const override;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
	virtual TSharedPtr<SWidget> GetThumbnailOverlay(const FAssetData& InAssetData) const override;
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	virtual bool GetThumbnailActionOverlay(const FAssetData& InAssetData, FAssetActionThumbnailOverlayInfo& OutActionOverlayInfo) const override;
#endif
	virtual EAssetCommandResult ActivateAssets(const FAssetActivateArgs& ActivateArgs) const override;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	virtual void GetAssetActionButtonExtensions(const FAssetData& InAssetData, TArray<FAssetButtonActionExtension>& OutExtensions) const override;
#endif
	// UAssetDefinition End

	// Menu Extension statics
	static void ExecutePlaySound(const FToolMenuContext& InContext);
	static void ExecuteStopSound(const FToolMenuContext& InContext);
	static bool CanExecutePlayCommand(const FToolMenuContext& InContext);
	static ECheckBoxState IsActionCheckedMute(const FToolMenuContext& InContext);
	static ECheckBoxState IsActionCheckedSolo(const FToolMenuContext& InContext);
	static void ExecuteMuteSound(const FToolMenuContext& InContext);
	static void ExecuteSoloSound(const FToolMenuContext& InContext);
	static bool CanExecuteMuteCommand(const FToolMenuContext& InContext);
	static bool CanExecuteSoloCommand(const FToolMenuContext& InContext);

	// Asset definition static utilities
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 6
	static TSharedPtr<SWidget> GetSoundBaseThumbnailOverlay(const FAssetData& InAssetData, TFunction<FReply()>&& OnClicked);
#endif
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	static void GetSoundBaseAssetActionButtonExtensions(const FAssetData& InAssetData, TArray<FAssetButtonActionExtension>& OutExtensions);
#endif
	static EAssetCommandResult ActivateSoundBase(const FAssetActivateArgs& ActivateArgs);
};
