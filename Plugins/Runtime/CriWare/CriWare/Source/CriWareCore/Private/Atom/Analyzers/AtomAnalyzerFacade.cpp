
#include "Atom/Analyzers/AtomAnalyzerFacade.h"

#include "Features/IModularFeatures.h"

#include "Atom/Analyzers/AtomAnalyzer.h"
#include "Extensions/IAtomAnalyzerInterface.h"

namespace Atom
{
	IAnalyzerFactory* GetAnalyzerFactory(FName InFactoryName)
	{
		ATOM_ANALYSIS_LLM_SCOPE

		// Get all analyzer nrt factories implementations.
		IModularFeatures::Get().LockModularFeatureList();
		TArray<IAnalyzerFactory*> RegisteredFactories = IModularFeatures::Get().GetModularFeatureImplementations<IAnalyzerFactory>(IAnalyzerFactory::GetModularFeatureName());
		IModularFeatures::Get().UnlockModularFeatureList();

		// Get the factory of interest by matching the name.
		TArray<IAnalyzerFactory*> MatchingFactories = RegisteredFactories.FilterByPredicate([InFactoryName](IAnalyzerFactory* Factory) { check(nullptr != Factory); return Factory->GetName() == InFactoryName; });

		if (0 == MatchingFactories.Num())
		{
			// There is a likely programming error if the factory is not found. 
			UE_LOG(LogCriWareAtomAnalyzer, Error, TEXT("Failed to find factory of type '%s' with name '%s'"), *IAnalyzerFactory::GetModularFeatureName().ToString(), *InFactoryName.ToString());

			return nullptr;
		}

		if (MatchingFactories.Num() > 1)
		{
			// If multiple factories with the same name exist, the first one in the array will be used. 
			UE_LOG(LogCriWareAtomAnalyzer, Warning, TEXT("Found multiple factories of type '%s' with name '%s'. Factory names should be unique."), *IAnalyzerFactory::GetModularFeatureName().ToString(), *InFactoryName.ToString());
		}

		return MatchingFactories[0];
	}

	FAnalyzerFacade::FAnalyzerFacade(TUniquePtr<IAnalyzerSettings> InSettings, IAnalyzerFactory* InFactory)
		: Settings(MoveTemp(InSettings))
		, Factory(InFactory)
	{
	}

	TUniquePtr<IAnalyzerResult> FAnalyzerFacade::AnalyzeAudioBuffer(const TArray<float>& InAudioBuffer, int32 InNumChannels, float InSampleRate, TSharedPtr<IAnalyzerControls> InControls)
	{
		ATOM_ANALYSIS_LLM_SCOPE

		if (nullptr == Factory)
		{
			UE_LOG(LogCriWareAtomAnalyzer, Error, TEXT("Cannot analyze audio due to null factory"));

			return TUniquePtr<IAnalyzerResult>();
		}

		// Create result and worker from factory
		if (!Worker.IsValid())
		{
			FAnalyzerParameters AnalyzerParameters(InSampleRate, InNumChannels);
			Worker = Factory->NewWorker(AnalyzerParameters, Settings.Get());
		}

		// Check that worker created successfully
		if (!Worker.IsValid())
		{
			UE_LOG(LogCriWareAtomAnalyzer, Error, TEXT("Failed to create IAnalyzerWorker with factory of type '%s' with name '%s'"), *IAnalyzerFactory::GetModularFeatureName().ToString(), *Factory->GetName().ToString());

			return TUniquePtr<IAnalyzerResult>();
		}

		TUniquePtr<IAnalyzerResult> Result = Factory->NewResult();

		// Check that result created successfully
		if (!Result.IsValid())
		{
			UE_LOG(LogCriWareAtomAnalyzer, Error, TEXT("Failed to create IAnalyzerResult with factory of type '%s' with name '%s'"), *Atom::IAnalyzerFactory::GetModularFeatureName().ToString(), *Factory->GetName().ToString());

			return TUniquePtr<IAnalyzerResult>();
		}

		// Perform and finalize audio analysis.
		Worker->SetControls(InControls);
		Worker->Analyze(MakeArrayView(InAudioBuffer), Result.Get());

		return Result;
	}
}

