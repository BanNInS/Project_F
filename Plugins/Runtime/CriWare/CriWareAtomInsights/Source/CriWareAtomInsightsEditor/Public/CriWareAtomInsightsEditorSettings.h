
#pragma once

#include "Engine/DeveloperSettings.h"

#include "AtomAudioSpectrogram.h"
#include "AtomAudioSpectrumAnalyzer.h"

#include "CriWareAtomInsightsEditorSettings.generated.h"

UCLASS(config = EditorPerProjectUserSettings)
class CRIWAREATOMINSIGHTSEDITOR_API UCriWareAtomInsightsEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;

	/** Whether to automatically set the first PIE client in Atom Insights World Filter. */
	UPROPERTY(Config, EditAnywhere, Category="World Filter")
	bool bWorldFilterDefaultsToFirstClient = false;

	/** Settings for analyzer rack spectrogram widget */
	UPROPERTY(EditAnywhere, config, Category = Spectrogram, meta = (ShowOnlyInnerProperties))
	FAtomSpectrogramRackUnitSettings SpectrogramSettings;

	/** Settings for analyzer rack spectrum analyzer widget */
	UPROPERTY(EditAnywhere, config, Category = SpectrumAnalyzer, meta = (ShowOnlyInnerProperties))
	FAtomSpectrumAnalyzerRackUnitSettings SpectrumAnalyzerSettings;
};
