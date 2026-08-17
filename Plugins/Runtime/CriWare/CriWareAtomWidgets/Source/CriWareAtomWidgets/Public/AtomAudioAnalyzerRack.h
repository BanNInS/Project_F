
#pragma once

#include "Framework/Docking/TabManager.h"

#include "CriWareDefines.h"
#include "Atom/AtomAudioBus.h"
#include "Atom/AtomRack.h"

#define CRI_API CRIWAREATOMWIDGETS_API

class FUICommandList;

namespace AtomWidgets
{
	/**
	 * FAudioBusInfo encapsulates the required info that describes the audio bus that is to be analyzed.
	 */
	struct FAudioBusInfo
	{
		static constexpr FAtomRuntimeId InvalidAtomRuntimeID = static_cast<FAtomRuntimeId>(INDEX_NONE);

		FAtomRuntimeId AtomRuntimeID = InvalidAtomRuntimeID;
		TObjectPtr<UAtomAudioBus> AudioBus = nullptr;

		int32 GetNumChannels() const { return (AudioBus) ? AudioBus->GetNumChannels() : 0; }
	};


	/**
	 * FAtomRackInfo encapsulates the required info that describes the Atom rack that is to be analyzed.
	 */
	struct FAtomRackInfo
	{
		static constexpr FAtomRuntimeId InvalidAtomRuntimeID = static_cast<FAtomRuntimeId>(INDEX_NONE);

		FAtomRuntimeId AtomRuntimeID = InvalidAtomRuntimeID;
		TObjectPtr<UAtomRackBase> AtomRack = nullptr;

		int32 GetNumChannels() const;
	};

	/**
	 * Interface for something that can be used in an analyzer rack.
	 */
	class IAudioAnalyzerRackUnit : public TSharedFromThis<IAudioAnalyzerRackUnit>
	{
	public:
		virtual ~IAudioAnalyzerRackUnit() {};

		/** If the Audio Bus to analyze changes (due to channel count change), handle this here. */
		virtual void SetAudioBusInfo(const FAudioBusInfo& AudioBusInfo) = 0;

		/** Spawn the actual analyzer Widget in a DockTab. */
		virtual TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args) const = 0;

		/** Be notified of request to start processing. */
		virtual void StartProcessing() {};

		/** Be notified of request to stop processing. */
		virtual void StopProcessing() {};
	};

	/**
	 * Parameters for the FOnMakeAudioAnalyzerRackUnit delegate.
	 */
	struct FAudioAnalyzerRackUnitConstructParams
	{
		FAtomRackInfo AtomRackInfo;
		FAudioBusInfo AudioBusInfo;
		const ISlateStyle* StyleSet = nullptr;
		const UClass* EditorSettingsClass = nullptr;
	};

	DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<IAudioAnalyzerRackUnit>, FOnMakeAudioAnalyzerRackUnit, const FAudioAnalyzerRackUnitConstructParams&);

	/**
	 * Static type descriptor data for a rack unit type.
	 */
	struct FAudioAnalyzerRackUnitTypeInfo
	{
		FName TypeName;
		FText DisplayName;
		FSlateIcon Icon;
		FOnMakeAudioAnalyzerRackUnit OnMakeAudioAnalyzerRackUnit;
		float VerticalSizeCoefficient;
	};

	/**
	 * Manages display of audio analyzer rack units. Rack units can be shown, hidden, and reordered by the user.
	 */
	class FAudioAnalyzerRack : public TSharedFromThis<FAudioAnalyzerRack>
	{
	public:
		struct FRackConstructParams
		{
			/** The rack layout can be saved using the given name. */
			FName TabManagerLayoutName;

			/** An ISlateStyle can be provided to override AudioWidgetsStyle. */
			const ISlateStyle* StyleSet = nullptr;

			/** An Editor Settings class can be provided for rack units that require one for saving settings. */
			const UClass* EditorSettingsClass = nullptr;
		};

		CRI_API FAudioAnalyzerRack(const FRackConstructParams& Params);
		CRI_API virtual ~FAudioAnalyzerRack();

		CRI_API void Init(UAtomRackBase* InAtomRack, FAtomRuntimeId InAtomRuntimeId);
		CRI_API void Init(int32 InNumChannels, FAtomRuntimeId InAtomRuntimeId);
		CRI_API void DestroyAnalyzers();

		CRI_API TSharedRef<SWidget> CreateWidget(TSharedRef<SDockTab> DockTab, const FSpawnTabArgs& SpawnTabArgs);

		CRI_API UAtomAudioBus* GetAudioBus() const;

		CRI_API void StartProcessing();
		CRI_API void StopProcessing();

		void SetTitleText(const FText& NewTitle, const FText& NewTooltip)
		{
			TitleText = NewTitle;
			TitleTooltipText = NewTooltip;
		}

	protected:
		CRI_API virtual TSharedRef<FTabManager::FArea> CreatePrimaryArea(const TArray<const FAudioAnalyzerRackUnitTypeInfo*>& RackUnitTypes);

	private:
		static TSharedRef<SWidget> MakeVisibleAnalyzersMenu(TSharedRef<FUICommandList> InCommandList, TWeakPtr<FWorkspaceItem> InWorkspaceGroup, TWeakPtr<FTabManager> InTabManager);

		static void SaveTabLayout(const TSharedRef<FTabManager::FLayout>& InLayout);
		TSharedRef<FTabManager::FLayout> LoadTabLayout();
		TSharedRef<FTabManager::FLayout> GetDefaultTabLayout();

		TSharedRef<IAudioAnalyzerRackUnit> MakeRackUnit(FName RackUnitTypeName);

		FName TabManagerLayoutName;

		TStrongObjectPtr<UAtomRackBase> AtomRack;
		TStrongObjectPtr<UAtomAudioBus> AudioBus;
		FAudioAnalyzerRackUnitConstructParams RackUnitConstructParams;

		TMap<FName, TSharedRef<IAudioAnalyzerRackUnit>> RackUnits;
		TSharedPtr<FTabManager> TabManager;
		
		bool bIsProcessingStarted = false;

		FText TitleText;
		FText TitleTooltipText;
	};
}

#undef CRI_API
