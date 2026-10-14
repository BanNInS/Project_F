
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/EngineSubsystem.h"

#include "AtomAnalyzer.h"

#include "AtomAnalyzerSubsystem.generated.h"

class UWorld;

/** 
* Class manages querying analysis results from various audio analyzers.
*/
UCLASS(MinimalAPI)
class UAtomAnalyzerSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	CRIWARECORE_API UAtomAnalyzerSubsystem();
	CRIWARECORE_API ~UAtomAnalyzerSubsystem();

	CRIWARECORE_API bool Tick(float DeltaTime);

	//~ Begin USubsystem
	CRIWARECORE_API virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	CRIWARECORE_API virtual void Deinitialize() override;
	//~ End USubsystem

	CRIWARECORE_API void RegisterAtomAnalyzer(UAtomAnalyzer* InAnalyzer);
	CRIWARECORE_API void UnregisterAtomAnalyzer(UAtomAnalyzer* InAnalyzer);

	static CRIWARECORE_API UAtomAnalyzerSubsystem* Get();

private:

	UPROPERTY(Transient);
	TArray<TObjectPtr<UAtomAnalyzer>> AtomAnalyzers;

	FTSTicker::FDelegateHandle TickerHandle;
};
