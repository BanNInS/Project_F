
#include "Atom/Analyzers/AtomAnalyzer.h"

#include "Async/Async.h"
#include "Engine/Engine.h"
#include "HAL/LowLevelMemStats.h"
#include "HAL/LowLevelMemTracker.h"
#include "Logging/LogMacros.h"

#include "Atom/Analyzers/AtomAnalyzerFacade.h"
#include "Atom/Analyzers/AtomAnalyzerSubsystem.h"

#include "Atom/AtomAudioBusSubsystem.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomAnalyzer)

DEFINE_LOG_CATEGORY(LogCriWareAtomAnalyzer);

FAtomAnalyzeTask::FAtomAnalyzeTask(TUniquePtr<Atom::FAnalyzerFacade>& InAnalyzerFacade, int32 InSampleRate, int32 InNumChannels)
	: AnalyzerFacade(InAnalyzerFacade.Release())
	, SampleRate(InSampleRate)
	, NumChannels(InNumChannels)
{
}

void FAtomAnalyzeTask::SetAudioBuffer(TArray<float>&& InAudioData)
{
	AudioData = MoveTemp(InAudioData);
}

void FAtomAnalyzeTask::SetAnalyzerControls(TSharedPtr<Atom::IAnalyzerControls> InControls)
{
	AnalyzerControls = InControls;
}

void FAtomAnalyzeTask::DoWork()
{
	check(AnalyzerFacade);
	Results = AnalyzerFacade->AnalyzeAudioBuffer(AudioData, NumChannels, SampleRate, AnalyzerControls);
}

void UAtomAnalyzer::StartAnalyzing(const FAtomRuntimeId InAtomRuntimeID, UAtomAudioBus* AudioBusToAnalyze)
{
	if (!AudioBusToAnalyze)
	{
		UE_LOG(LogCriWareAtomAnalyzer, Error, TEXT("Unable to analyze audio without an Atom audio bus to analyze."));
		return;
	}

	const FAtomRuntimeManager* AtomRuntimeManager = FAtomRuntimeManager::Get();
	if (!AtomRuntimeManager)
	{
		UE_LOG(LogCriWareAtomAnalyzer, Error, TEXT("Unable to analyze audio with a null Atom runtime manager."));
		return;
	}

	const FAtomRuntime* AtomRuntime = static_cast<const FAtomRuntime*>(AtomRuntimeManager->GetAtomRuntimeRaw(InAtomRuntimeID));
	if (!AtomRuntime)
	{
		UE_LOG(LogCriWareAtomAnalyzer, Error, TEXT("Unable to analyze audio with an invalid Atom runtime."));
		return;
	}

	// Retrieve the audio analyzer subsystem if it's not already retrieved
	if (!AtomAnalyzerSubsystem)
	{
		AtomAnalyzerSubsystem = UAtomAnalyzerSubsystem::Get();
	}

	// Store the audio bus we're analyzing
	AudioBus = AudioBusToAnalyze;

	NumBusChannels = AudioBus->GetNumChannels();
	check(NumBusChannels > 0);

	AtomRuntimeSampleRate = (int32)AtomRuntime->GetRuntimeSampleRate(); // master rack !

	// Get the analyzer factory to use for our analysis
	if (!AnalyzerFactory)
	{
		AnalyzerFactory = Atom::GetAnalyzerFactory(GetAnalyzerFactoryName());
	}
	checkf(AnalyzerFactory != nullptr, TEXT("Need to register the factory as a modular feature for the analyzer '%s'."), *GetAnalyzerFactoryName().ToString());

	// Start the audio bus. This won't do anythign if the bus is already started elsewhere.
	uint32 AudioBusID = AudioBus->GetUniqueID();
	int32 NumChannels = (int32)AudioBus->AudioBusChannels + 1;

	UAtomAudioBusSubsystem* AudioBusSubsystem = AtomRuntime->GetSubsystem<UAtomAudioBusSubsystem>();
	check(AudioBusSubsystem);
	Atom::FAudioBusKey AudioBusKey = Atom::FAudioBusKey(AudioBusID);

	const FString AudioAnalyzerBusName = FString::Format(TEXT("_Analyzer_{0}_AudioBusID_{1}"), { *GetName(), AudioBusID });
	AudioBusSubsystem->StartAudioBus(AudioBusKey, AudioAnalyzerBusName, NumChannels, false);

	// Get an output patch for the audio bus
	NumFramesPerBufferToAnalyze = AtomRuntime->GetRuntimeNumOutputFrames(); // 256
	PatchOutputStrongPtr = AudioBusSubsystem->AddPatchOutputForAudioBus(AudioBusKey, NumFramesPerBufferToAnalyze, NumChannels);

	// Register this audio analyzer with the audio analyzer subsystem
	// The subsystem will query this analyzer to see if it has enough audio to perform analysis.
	// If it does, it'll report the results (of any previous analysis) and ask us to start analyzing the audio.	
	if (AtomAnalyzerSubsystem)
	{
		AtomAnalyzerSubsystem->RegisterAtomAnalyzer(this);
	}

	// Setup the analyzer facade here once
	AnalyzerFacade = MakeUnique<Atom::FAnalyzerFacade>(GetSettings(AtomRuntimeSampleRate, NumBusChannels), AnalyzerFactory);
}


void UAtomAnalyzer::StartAnalyzing(const UObject* WorldContextObject, UAtomAudioBus* AudioBusToAnalyze)
{
	if (!AudioBusToAnalyze)
	{
		UE_LOG(LogCriWareAtomAnalyzer, Error, TEXT("Unable to analyze audio without an audio bus to analyze."));
		return;
	}

	// Retrieve the world and if it's not a valid world, don't do anything
	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!ThisWorld || !ThisWorld->bAllowAudioPlayback || ThisWorld->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const FAtomRuntimeHandle AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorld(ThisWorld);
	if (!AtomRuntime.IsValid())
	{
		return;
	}

	StartAnalyzing(AtomRuntime.GetRuntimeID(), AudioBusToAnalyze);
}

void UAtomAnalyzer::StopAnalyzing(const UObject* WorldContextObject)
{
	if (AtomAnalyzerSubsystem)
	{
		// Unregister the audio analyzer from the analyzer subsystem
		AtomAnalyzerSubsystem->UnregisterAtomAnalyzer(this);

		// Null out our patch output
		PatchOutputStrongPtr = nullptr;
	}
}

bool UAtomAnalyzer::IsReadyForAnalysis() const
{
	check(AudioBus != nullptr);

	// Only allow one worker task to happen at a time
	if (AnalysisTask.IsValid() && !AnalysisTask->IsWorkDone())
	{
		return false;
	}

	// Only ready if we've got a patch output and there's enough audio queued up
	if (PatchOutputStrongPtr.IsValid())
	{
		int32 NumSamplesAvailable = PatchOutputStrongPtr->GetNumSamplesAvailable();
		int32 NumAudioBusChannels = AudioBus->GetNumChannels();
		check(NumAudioBusChannels > 0);
		int32 NumFramesAvailable = NumSamplesAvailable / NumAudioBusChannels;
		return NumFramesAvailable >= NumFramesPerBufferToAnalyze;
	}
	return false;
}

bool UAtomAnalyzer::DoAnalysis()
{
	bool bHasResults = false;

	// Make sure the task is finished before we do any more analysis
	if (AnalysisTask.IsValid())
	{
		check(AnalysisTask->IsWorkDone());

		// Do final ensure completion before reusing the task
		AnalysisTask->EnsureCompletion();

		// Report results from previous analysis
		FAtomAnalyzeTask& Task = AnalysisTask->GetTask();
		ResultsInternal = Task.GetResults();

		// Retrieve the analysis buffer so we can reuse the memory when we copy to it from the patch output
		AnalysisBuffer = Task.GetAudioBuffer();

		bHasResults = true;
	}
	else
	{
		// Create a task if one doesn't exist
		AnalysisTask = TSharedPtr<FAsyncTask<FAtomAnalyzeTask>>(new FAsyncTask<FAtomAnalyzeTask>(AnalyzerFacade, AtomRuntimeSampleRate, NumBusChannels));
	}

	// Make sure our task is valid and done
	check(AnalysisTask.IsValid() && AnalysisTask->IsDone());

	// Copy the audio to be analyzed from the patch output
	int32 NumSamplesAvailable = PatchOutputStrongPtr->GetNumSamplesAvailable();

	AnalysisBuffer.Reset();
	AnalysisBuffer.AddUninitialized(NumSamplesAvailable);
	PatchOutputStrongPtr->PopAudio(AnalysisBuffer.GetData(), NumSamplesAvailable, false);

	check(AtomRuntimeSampleRate != 0);
	check(NumBusChannels != 0);

	FAtomAnalyzeTask& Task = AnalysisTask->GetTask();
	Task.SetAnalyzerControls(GetAnalyzerControls());
	Task.SetAudioBuffer(MoveTemp(AnalysisBuffer));

 	AnalysisTask->StartBackgroundTask();

	return bHasResults;
}

void UAtomAnalyzer::BeginDestroy()
{
	Super::BeginDestroy();

	if (AnalysisTask.IsValid())
	{
		AnalysisTask->EnsureCompletion();
	}

	// Unregister this audio analyzer from the subsystem since we won't be needing anymore ticks
	if (AtomAnalyzerSubsystem)
	{
		AtomAnalyzerSubsystem->UnregisterAtomAnalyzer(this);
	}
}

UAtomAnalyzerSettings* UAtomAnalyzer::GetSettingsFromProperty(FProperty* Property)
{
	if (nullptr == Property)
	{
		return nullptr;
	}

	if (Property->IsA(FObjectPropertyBase::StaticClass()))
	{
		FObjectPropertyBase* ObjectPropertyBase = CastFieldChecked<FObjectPropertyBase>(Property);

		if (nullptr == ObjectPropertyBase)
		{
			return nullptr;
		}

		if (ObjectPropertyBase->PropertyClass->IsChildOf(UAtomAnalyzerSettings::StaticClass()))
		{
			UObject* PropertyObject = ObjectPropertyBase->GetObjectPropertyValue_InContainer(this);
			return Cast<UAtomAnalyzerSettings>(PropertyObject);
		}
	}

	return nullptr;
}

TUniquePtr<Atom::IAnalyzerSettings> UAtomAnalyzer::GetSettings(const int32 InSampleRate, const int32 InNumChannels) const
{
	return MakeUnique<Atom::IAnalyzerSettings>();
}

TSharedPtr<Atom::IAnalyzerControls> UAtomAnalyzer::GetAnalyzerControls() const
{
	return AnalyzerControls;
}
