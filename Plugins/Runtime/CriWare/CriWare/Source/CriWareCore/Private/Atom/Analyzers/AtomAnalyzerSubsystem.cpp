
#include "Atom/Analyzers/AtomAnalyzerSubsystem.h"

#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomAnalyzerSubsystem)

UAtomAnalyzerSubsystem::UAtomAnalyzerSubsystem()
{
}

UAtomAnalyzerSubsystem::~UAtomAnalyzerSubsystem()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	AtomAnalyzers.Reset();
}

UAtomAnalyzerSubsystem* UAtomAnalyzerSubsystem::Get()
{
	if (GEngine)
	{
		return GEngine->GetEngineSubsystem<UAtomAnalyzerSubsystem>();
	}
	return nullptr;
}

bool UAtomAnalyzerSubsystem::Tick(float DeltaTime)
{
	ATOM_ANALYSIS_LLM_SCOPE

	// Loop through all analyzers and if they're ready to analyze, do it
	for (UAtomAnalyzer* Analyzer : AtomAnalyzers)
	{
		if (Analyzer->IsReadyForAnalysis())
		{
			if (Analyzer->DoAnalysis())
			{
				Analyzer->BroadcastResults();
			}
		}
	}
	return true;
}

void UAtomAnalyzerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAtomAnalyzerSubsystem::Deinitialize()
{
	ATOM_ANALYSIS_LLM_SCOPE

	// Release our Atom analyzers
	AtomAnalyzers.Reset();
}

void UAtomAnalyzerSubsystem::RegisterAtomAnalyzer(UAtomAnalyzer* InAnalyzer)
{
	ATOM_ANALYSIS_LLM_SCOPE

	if (!TickerHandle.IsValid())
	{
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UAtomAnalyzerSubsystem::Tick), 0.0f);
	}
	AtomAnalyzers.AddUnique(InAnalyzer);
}

void UAtomAnalyzerSubsystem::UnregisterAtomAnalyzer(UAtomAnalyzer* InAnalyzer)
{
	ATOM_ANALYSIS_LLM_SCOPE

	AtomAnalyzers.Remove(InAnalyzer);
	if (AtomAnalyzers.IsEmpty() && TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
}
