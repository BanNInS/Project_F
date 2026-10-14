

#include "Features/IModularFeatures.h"

#include "AdvancedWidgetsModule.h"

#include "Analyzers/AtomConstantQFactory.h"
#include "Analyzers/AtomLoudnessFactory.h"
#include "Analyzers/AtomMeterFactory.h"
#include "Analyzers/AtomSpectrumAnalysisFactory.h"
#include "Analyzers/AtomTruePeakFactory.h"
#include "AtomAudioAnalyzerRackUnitRegistry.h"
#include "AtomLevelMeter.h"
#include "AtomLoudnessMeter.h"
#include "AtomAudioOscilloscope.h"
#include "AtomAudioSpectrogram.h"
#include "AtomAudioSpectrumAnalyzer.h"
#include "AtomAudioVectorscope.h"

#define LOCTEXT_NAMESPACE "FCriWareAtomWidgetModule"

class FCriWareAtomWidgetsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		using namespace AtomWidgets;

		// Required to load so the AudioWidget plugin content can reference widgets
		// defined in AdvancedWidgets (ex. RadialSlider for UMG-defined knobs)
		//FModuleManager::Get().LoadModuleChecked<FAdvancedWidgetsModule>("AdvancedWidgets");
		
		// Initialize static style instance 
		FSlateStyle::Get();

		// Atom analyzers
		IModularFeatures::Get().RegisterModularFeature(Atom::FLoudnessFactory::GetModularFeatureName(), &LoudnessFactory);
		IModularFeatures::Get().RegisterModularFeature(Atom::FConstantQFactory::GetModularFeatureName(), &ConstantQFactory);
		IModularFeatures::Get().RegisterModularFeature(Atom::FMeterFactory::GetModularFeatureName(), &MeterFactory);
		IModularFeatures::Get().RegisterModularFeature(Atom::FSpectrumAnalysisFactory::GetModularFeatureName(), &SpectralAnalysisFactory);
		IModularFeatures::Get().RegisterModularFeature(Atom::FTruePeakFactory::GetModularFeatureName(), &TruePeakFactory);
	
		// Register standard analyzer rack units:
		RegisterAudioAnalyzerRackUnitType(&FAtomLevelMeter::RackUnitTypeInfo);
		RegisterAudioAnalyzerRackUnitType(&FAtomLoudnessMeter::RackUnitTypeInfo);
		RegisterAudioAnalyzerRackUnitType(&FAudioOscilloscope::RackUnitTypeInfo);
		RegisterAudioAnalyzerRackUnitType(&FAudioVectorscope::RackUnitTypeInfo);
		RegisterAudioAnalyzerRackUnitType(&FAudioSpectrogram::RackUnitTypeInfo);
		RegisterAudioAnalyzerRackUnitType(&FAudioSpectrumAnalyzer::RackUnitTypeInfo);
	}

	virtual void ShutdownModule() override
	{
		AtomWidgets::FAudioAnalyzerRackUnitRegistry::TearDown();

		// Atom analyzers
		IModularFeatures::Get().UnregisterModularFeature(Atom::FMeterFactory::GetModularFeatureName(), &MeterFactory);
	}

	// AtomWidgets
	Atom::FLoudnessFactory LoudnessFactory;
	Atom::FConstantQFactory ConstantQFactory;
	Atom::FMeterFactory MeterFactory;
	Atom::FSpectrumAnalysisFactory SpectralAnalysisFactory;
	Atom::FTruePeakFactory TruePeakFactory;

	// Register a custom audio analyzer rack unit type
	void RegisterAudioAnalyzerRackUnitType(const AtomWidgets::FAudioAnalyzerRackUnitTypeInfo* RackUnitTypeInfo)
	{
		AtomWidgets::FAudioAnalyzerRackUnitRegistry::Get().RegisterRackUnitType(RackUnitTypeInfo);
	}
};

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCriWareAtomWidgetsModule, CriWareAtomWidgets)
