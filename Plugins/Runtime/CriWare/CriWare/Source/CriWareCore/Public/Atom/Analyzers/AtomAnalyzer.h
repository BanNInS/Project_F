
#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "DSP/MultithreadedPatching.h"

#include "CriWareDefines.h"
#include "CriWareLLM.h"
#include "Atom/AtomAudioBus.h"
#include "Extensions/IAtomAnalyzerInterface.h"
#include "AtomAnalyzerFacade.h"

#include "AtomAnalyzer.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogCriWareAtomAnalyzer, Log, All);

// Convenience macro for Atom_Analysis LLM scope to avoid misspells.
#define ATOM_ANALYSIS_LLM_SCOPE LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomAudioAnalysis);

class UAtomAnalyzerSubsystem;

/** UAtomAnalyzerSettings
 *
 * UAtomAnalyzerSettings provides a way to store and reuse existing analyzer settings
 * across multiple analyzers. 
 *
 */
UCLASS(Abstract, EditInlineNew, BlueprintType, MinimalAPI)
class UAtomAnalyzerSettings : public UObject/* : public UAtomAnalyzerAssetBase */
{
	GENERATED_BODY()
};

typedef TSharedPtr<Atom::IAnalyzerResult, ESPMode::ThreadSafe> FAtomAnalyzerResultSharedPtr;

class FAtomAnalyzeTask : public FNonAbandonableTask
{
public:
	FAtomAnalyzeTask(TUniquePtr<Atom::FAnalyzerFacade>& InAnalyzerFacade, int32 InSampleRate, int32 InNumChannels);
	
	// Set in the task the current state of the analyzer controls
	void SetAnalyzerControls(TSharedPtr<Atom::IAnalyzerControls> InControls);

	// Give the task the audio data to analyze
	void SetAudioBuffer(TArray<float>&& InAudioData);

	// Move the audio buffer back out
	TArray<float>&& GetAudioBuffer() { return MoveTemp(AudioData); }

	// Does the task work
	void DoWork();

	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(AudioAnalyzeTask, STATGROUP_ThreadPoolAsyncTasks); }

	// Get the results from the task
	TUniquePtr<Atom::IAnalyzerResult> GetResults() { return MoveTemp(Results); }

private:
	TUniquePtr<Atom::FAnalyzerFacade> AnalyzerFacade;
	int32 SampleRate = 0;
	int32 NumChannels = 0;
	TArray<float> AudioData;
	TUniquePtr<Atom::IAnalyzerResult> Results;
	TSharedPtr<Atom::IAnalyzerControls> AnalyzerControls;
};

/** UAtomAnalyzer
 *
 * UAtomAnalyzer performs analysis on an audio bus using specific settings and exposes the results via blueprints.
 *
 * Subclasses of UAtomAnalyzer must implement GetAnalyzerFactoryName() to associate
 * the UAtomAnalyzer asset with an IAudioAnalyzerFactory implementation.
 *
 * To support blueprint access, subclasses can implement UFUNCTIONs to expose the data
 * returned by GetResult().
 */
UCLASS(Abstract, EditInlineNew, BlueprintType, MinimalAPI)
class UAtomAnalyzer : public UObject
{
	GENERATED_BODY()
	
	friend class UAtomAnalyzerSubsystem;

public:
	/**
	 * ID to keep track of results. Useful for tracking most recent result when performing
	 * asynchronous processing.
	 */
	typedef int32 FResultId;

	/** Thread safe shared point to result object. */
	typedef TSharedPtr<Atom::IAnalyzerResult, ESPMode::ThreadSafe> FResultSharedPtr;

	/** The UAtomAudioBus which is analyzed in real-time. */
	UPROPERTY(Transient)
	TObjectPtr<UAtomAudioBus> AudioBus;

	/** Starts analyzing audio from the given audio bus. Optionally override the Atom audio bus desired to analyze. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AudioAnalyzer, meta = (WorldContext = "WorldContextObject"))
	CRIWARECORE_API void StartAnalyzing(const UObject* WorldContextObject, UAtomAudioBus* AudioBusToAnalyze);

	/** Starts analyzing using the given world.*/
	CRIWARECORE_API void StartAnalyzing(const FAtomRuntimeId InAtomRuntimeID, UAtomAudioBus* AudioBusToAnalyze);

	/** Stops analyzing audio. */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = AudioAnalyzer, meta = (WorldContext = "WorldContextObject"))
	CRIWARECORE_API void StopAnalyzing(const UObject* WorldContextObject = nullptr);

	/**
	 * Implementations can override this method to create settings objects
	 * specific for their analyzer.
	 */
	CRIWARECORE_API virtual TUniquePtr<Atom::IAnalyzerSettings> GetSettings(const int32 InSampleRate, const int32 InNumChannels) const;

	/**
	 * Implementations can override this method to create controls objects
	 * specific for their analyzer.
	 */
	CRIWARECORE_API virtual TSharedPtr<Atom::IAnalyzerControls> GetAnalyzerControls() const;

	/** Function to broadcast results. */
	virtual void BroadcastResults() {}

	//~ Begin UObject Interface.
	CRIWARECORE_API virtual void BeginDestroy() override;
	//~ End UObject interface

private: 
	/** Returns if this audio analyzer has enough audio queued up and is ready for analysis. */
	CRIWARECORE_API bool IsReadyForAnalysis() const;

	/** Does the actual analysis and casts the results to the derived classes type. Returns true if there results to broadcast. */
	CRIWARECORE_API bool DoAnalysis();

protected:

	template<class ResultType>
	TUniquePtr<ResultType> GetResults()
	{
		return TUniquePtr<ResultType>(static_cast<ResultType*>(ResultsInternal.Release()));
	}

	/* Subclasses must override this method in order to inform this object which AnalyzerFactory to use for analysis */
	CRIWARECORE_API virtual FName GetAnalyzerFactoryName() const PURE_VIRTUAL(UAtomAnalyzer::GetAnalyzerFactoryName, return FName(););

	/** How many frames of audio to wait before analyzing the audio. */
	int32 NumFramesPerBufferToAnalyze = 1024;

private:

	// Returns UAtomAnalyzerSettings* if property points to a valid UAtomAnalyzerSettings, otherwise returns nullptr.
	CRIWARECORE_API UAtomAnalyzerSettings* GetSettingsFromProperty(FProperty* Property);

	// Atom analysis subsystem used with this Atom analyzer
	UPROPERTY(Transient)
	TObjectPtr<UAtomAnalyzerSubsystem> AtomAnalyzerSubsystem;

	// Output patch for retrieving audio from Atom audio bus for analysis
	Audio::FPatchOutputStrongPtr PatchOutputStrongPtr;

	// Analysis task
	TSharedPtr<FAsyncTask<FAtomAnalyzeTask>> AnalysisTask;

	// The analyzer facade used for async tasks
	TUniquePtr<Atom::FAnalyzerFacade> AnalyzerFacade;

	// Cached results of previous analysis task. Can be invalid.
	TUniquePtr<Atom::IAnalyzerResult> ResultsInternal;

	// The analyzer controls that can be modified real-time
	TSharedPtr<Atom::IAnalyzerControls> AnalyzerControls;

	// Scratch buffer used for copying data from patch output and fed to analysis task
	TArray<float> AnalysisBuffer;

	// The sample of the Atom renderer
	int32 AtomRuntimeSampleRate = 0;

	// The number of channels for the Atom audio bus
	int32 NumBusChannels = 0;

	// The analyzer factory to use
	Atom::IAnalyzerFactory* AnalyzerFactory = nullptr;
};

