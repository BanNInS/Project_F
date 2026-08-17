/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomThread.cpp
 *
 ****************************************************************************/

#include "Atom/AtomThread.h"

#include "Async/Async.h"
#include "Containers/SpscQueue.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Tasks/Pipe.h"

#include "CriWareLLM.h"
#include "CriWareCore.h"
#include "Atom/Atom.h"

//
// Globals
//

extern CRIWARECORE_API UE::Tasks::FPipe GAtomPipe;
extern CRIWARECORE_API std::atomic<bool> GIsAtomThreadRunning;
extern CRIWARECORE_API std::atomic<bool> GIsAtomThreadSuspended;

static int32 GCVarSuspendAtomThread = 0;
FAutoConsoleVariableRef CVarSuspendAtomThread(TEXT("AtomThread.SuspendAtomThread"), GCVarSuspendAtomThread, TEXT("0=Resume, 1=Suspend"), ECVF_Cheat);

static int32 GCVarAboveNormalAtomThreadPri = 0;
FAutoConsoleVariableRef CVarAboveNormalAtomThreadPri(TEXT("AtomThread.AboveNormalPriority"), GCVarAboveNormalAtomThreadPri, TEXT("0=Normal, 1=AboveNormal"), ECVF_Default);

static int32 GCVarEnableAtomCommandLogging = 0;
FAutoConsoleVariableRef CVarEnableAtomCommandLogging(TEXT("AtomThread.EnableAtomCommandLogging"), GCVarEnableAtomCommandLogging, TEXT("0=Disabled, 1=Enabled"), ECVF_Default);

static int32 GCVarEnableAtomBatchProcessing = 1;
FAutoConsoleVariableRef CVarEnableAtomBatchProcessing(
	TEXT("AtomThread.EnableBatchProcessing"),
	GCVarEnableAtomBatchProcessing,
	TEXT("Enables batch processing atom thread commands.\n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 GBatchAtomAsyncBatchSize = 128;
static FAutoConsoleVariableRef CVarBatchAtomAsyncBatchSize(
	TEXT("AtomThread.BatchAsyncBatchSize"),
	GBatchAtomAsyncBatchSize,
	TEXT("When AtomThread.EnableBatchProcessing = 1, controls the number of atom commands grouped together for threading.")
);

static int32 GAtomCommandFenceWaitTimeMs = 35;
FAutoConsoleVariableRef  CVarAtomCommandFenceWaitTimeMs(
	TEXT("AtomCommand.FenceWaitTimeMs"),
	GAtomCommandFenceWaitTimeMs,
	TEXT("Sets number of ms for fence wait"),
	ECVF_Default);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
namespace AtomCommandPrivate
{
	bool bBatchGameThreadAtomCommands = true;

	/*
	 * An Atom command to be executed on the game thread.
	 */
	struct FAtomCommand
	{
		FAtomCommand() = default;

		FAtomCommand(TUniqueFunction<void()> InFunction, const TStatId InStatId)
			: Function(MoveTemp(InFunction))
			, StatId(InStatId)
		{
		}

		FAtomCommand(FAtomCommand&& Other) = default;
		FAtomCommand& operator=(FAtomCommand&& Other) = default;

		static void Execute(const TUniqueFunction<void()>& InFunction, TStatId InStatId)
		{
			check(IsInGameThread());

			CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Atom);
			QUICK_SCOPE_CYCLE_COUNTER(STAT_AtomCommandQueue_RunCommandOnGameThread);
			FScopeCycleCounter ScopeCycleCounter(InStatId);

			InFunction();
		}

		TUniqueFunction<void()> Function;
		TStatId StatId;
	};

	const FLazyName DiagnosticName("FGameThreadAtomCommandQueue");

	/*
	 * Used to run commands from the Atom thread, on the game thread.
	 */
	class FGameThreadAtomCommandQueue : public FTickFunction
	{
	public:
		FGameThreadAtomCommandQueue()
			: FTickFunction()
		{
			// Setup tick properties 
			bCanEverTick = true;
			bTickEvenWhenPaused = false;
			bStartWithTickEnabled = true;
			TickGroup = TG_PrePhysics;
			bAllowTickOnDedicatedServer = false;
			bRunOnAnyThread = false;
			bIsTicking = false;
		}

		virtual ~FGameThreadAtomCommandQueue() override = default;

		void StartTick()
		{
			if (!bIsTicking && bBatchGameThreadAtomCommands)
			{
				if (GEngine)
				{
					RegisterTickFunctionToWorld(GEngine->GetCurrentPlayWorld());
				}

				FWorldDelegates::OnWorldInitializedActors.AddRaw(this, &FGameThreadAtomCommandQueue::OnWorldActorsInitialized);
				bIsTicking = true;
			}
		}

		void EndTick()
		{
			if (bIsTicking)
			{
				FWorldDelegates::OnWorldInitializedActors.RemoveAll(this);
				UnRegisterTickFunction();
				bIsTicking = false;
			}
		}

		bool CanTick() const
		{
			return bBatchGameThreadAtomCommands && bIsTicking;
		}

		void RunCommandOnGameThread(TUniqueFunction<void()> InFunction, const TStatId InStatId)
		{
			if (IsAtomThreadRunning())
			{
				check(IsInAtomThread());
				Commands.Enqueue(FAtomCommand(MoveTemp(InFunction), InStatId));
			}
			else
			{
				FAtomCommand::Execute(InFunction, InStatId);
			}
		}

		void FlushCommands()
		{
			TOptional<FAtomCommand> AtomCommand = Commands.Dequeue();
			while (AtomCommand.IsSet())
			{
				if (IsInAtomThread())
				{
					ExecuteOnGameThread(
						TEXT("FAtomThread::RunCommandOnGameThread"),
						[Function = MoveTemp(AtomCommand.GetValue().Function), StatId = AtomCommand.GetValue().StatId]()
						{
							FAtomCommand::Execute(Function, StatId);
						});
				}
				else
				{
					FAtomCommand::Execute(AtomCommand.GetValue().Function, AtomCommand.GetValue().StatId);
				}

				AtomCommand = Commands.Dequeue();
			}
		}

	private:
		// Begin FTickFunction overrides
		virtual void ExecuteTick(float InDeltaTime, ELevelTick InTickType, ENamedThreads::Type InCurrentThread, const FGraphEventRef& InMyCompletionGraphEvent) override
		{
			TOptional<FAtomCommand> AtomCommand = Commands.Dequeue();
			while (AtomCommand.IsSet())
			{
				FAtomCommand::Execute(AtomCommand.GetValue().Function, AtomCommand.GetValue().StatId);
				AtomCommand = Commands.Dequeue();
			}
		}

		virtual FString DiagnosticMessage() override
		{
			return DiagnosticName.ToString();
		}

		virtual FName DiagnosticContext(bool bDetailed) override
		{
			return DiagnosticName;
		}
		// End FTickFunction overrides

		void OnWorldActorsInitialized(const UWorld::FActorsInitializedParams& InParams)
		{
			// When a new world is up and running unregister the tick function with the old world
			// and register it with the new world
			UnRegisterTickFunction();
			RegisterTickFunctionToWorld(InParams.World);
		}

		void RegisterTickFunctionToWorld(const UWorld* const InWorld)
		{
			if (InWorld)
			{
				RegisterTickFunction(InWorld->PersistentLevel);
			}
		}

		TSpscQueue<FAtomCommand> Commands;
		bool bIsTicking;
	};

	FGameThreadAtomCommandQueue CommandQueue;

	void BatchGameThreadAtomCommandsChangedCallback(IConsoleVariable* ConsoleVariable)
	{
		if (GIsAtomThreadRunning)
		{
			const bool bBatchCommands = ConsoleVariable->GetBool();
			if (bBatchCommands)
			{
				CommandQueue.StartTick();
			}
			else
			{
				CommandQueue.EndTick();
				CommandQueue.FlushCommands();
			}
		}
	}

	FAutoConsoleVariableRef CVarBatchGameThreadAtomCommands(
		TEXT("AtomThread.BatchCommands"),
		bBatchGameThreadAtomCommands,
		TEXT("Batch Atom commands that are created from the Atom thread and executed on the game thread so that they are executed in a single task."),
		FConsoleVariableDelegate::CreateStatic(&BatchGameThreadAtomCommandsChangedCallback),
		ECVF_Default
	);
}

static bool GAtomThreadUseSafeRunCommandOnGameThread = true;
FAutoConsoleVariableRef CVarAtomThreadUseSafeRunCommandOnGameThread(
	TEXT("AtomThread.UseSafeRunCommandOnGameThread"),
	GAtomThreadUseSafeRunCommandOnGameThread,
	TEXT("When active, limit commands sent to the game thread to run at a safe place inside a tick"),
	ECVF_Default);

#endif

struct FAtomThreadInteractor
{
	static void UseAtomThreadCVarSinkFunction()
	{
		static bool bLastSuspendAtomThread = false;
		const bool bSuspendAtomThread = GCVarSuspendAtomThread != 0;

		if (bLastSuspendAtomThread != bSuspendAtomThread)
		{
			bLastSuspendAtomThread = bSuspendAtomThread;
			if (bSuspendAtomThread && IsAtomThreadRunning())
			{
				FAtomThread::SuspendAtomThread();
			}
			else if (GIsAtomThreadSuspended)
			{
				FAtomThread::ResumeAtomThread();
			}
			else if (GIsEditor)
			{
				UE_LOG(LogCriWareAtom, Warning, TEXT("Atom threading is disabled in the editor."));
			}
			else if (!FAtomThread::IsUsingThreadedAtom())
			{
				UE_LOG(LogCriWareAtom, Warning, TEXT("Cannot manipulate atom thread when disabled by platform or ini."));
			}
		}
	}
};

UE::Tasks::ETaskPriority GAtomTaskPriority = UE::Tasks::ETaskPriority::Normal;

static void SetAtomTaskPriority(const TArray<FString>& Args)
{
	UE_LOG(LogConsoleResponse, Display, TEXT("AtomTaskPriority was %s."), LowLevelTasks::ToString(GAtomTaskPriority));

	if (Args.Num() > 1)
	{
		UE_LOG(LogConsoleResponse, Display, TEXT("WARNING: This command requires a single argument while %d were provided, all extra arguments will be ignored."), Args.Num());
	}
	else if (Args.IsEmpty())
	{
		UE_LOG(LogConsoleResponse, Display, TEXT("ERROR: Please provide a new priority value."));
		return;
	}

	if (!LowLevelTasks::ToTaskPriority(*Args[0], GAtomTaskPriority))
	{
		UE_LOG(LogConsoleResponse, Display, TEXT("ERROR: Invalid priority: %s."), *Args[0]);
	}

	UE_LOG(LogConsoleResponse, Display, TEXT("Atom Task Priority was set to %s."), LowLevelTasks::ToString(GAtomTaskPriority));
}

static FAutoConsoleCommand AtomThreadPriorityConsoleCommand(
	TEXT("AtomThread.TaskPriority"),
	TEXT("Takes a single parameter of value `High`, `Normal`, `BackgroundHigh`, `BackgroundNormal` or `BackgroundLow`."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&SetAtomTaskPriority)
);

static FAutoConsoleVariableSink CVarUseAtomThreadSink(FConsoleCommandDelegate::CreateStatic(&FAtomThreadInteractor::UseAtomThreadCVarSinkFunction));

bool FAtomThread::bUseThreadedAtom = false;

FCriticalSection FAtomThread::CurrentAtomThreadStatIdCS;
TStatId FAtomThread::CurrentAtomThreadStatId;
TStatId FAtomThread::LongestAtomThreadStatId;
double FAtomThread::LongestAtomThreadTimeMsec = 0.0;

TUniquePtr<UE::Tasks::FTaskEvent> FAtomThread::ResumeEvent;
int32 FAtomThread::SuspendCount{ 0 };

void FAtomThread::SuspendAtomThread()
{
	check(IsInGameThread()); // thread-safe version would be much more complicated

	if (!GIsAtomThreadRunning)
	{
		return; // nothing to suspend
	}

	if (++SuspendCount != 1)
	{
		return; // recursive scope
	}

	check(!GIsAtomThreadSuspended.load(std::memory_order_relaxed));

	FEventRef SuspendEvent;

	FAtomThread::RunCommandOnAtomThread(
		[&SuspendEvent]
		{
			GIsAtomThreadSuspended.store(true, std::memory_order_release);
			UE::Tasks::AddNested(*ResumeEvent);
			SuspendEvent->Trigger();
		}
	);

	// release batch processing so the task above will be executed
	FAtomThread::ProcessAllCommands();

	// wait for the command above to block atom processing
	SuspendEvent->Wait();
}

void FAtomThread::ResumeAtomThread()
{
	check(IsInGameThread());

	if (!GIsAtomThreadRunning)
	{
		return; // nothing to resume
	}

	if (--SuspendCount != 0)
	{
		return; // recursive scope
	}

	check(GIsAtomThreadSuspended.load(std::memory_order_relaxed));
	GIsAtomThreadSuspended.store(false, std::memory_order_release);

	check(!ResumeEvent->IsCompleted());
	ResumeEvent->Trigger();
	ResumeEvent = MakeUnique<UE::Tasks::FTaskEvent>(UE_SOURCE_LOCATION);
}

void FAtomThread::SetUseThreadedAtom(const bool bInUseThreadedAtom)
{
	if (IsAtomThreadRunning() && !bInUseThreadedAtom)
	{
		UE_LOG(LogCriWareAtom, Error, TEXT("You cannot disable using threaded Atom once the thread has already begun running."));
	}
	else
	{
		bUseThreadedAtom = bInUseThreadedAtom;
	}
}

bool FAtomThread::IsUsingThreadedAtom()
{
	return bUseThreadedAtom;
}

// Batching Atom commands allows to avoid the overhead of launching a task per command when resources are limited.
// We assume that resources are limited if the previous batch is not completed yet (potentially waiting for execution due to the CPU being busy
// with something else). Otherwise we don't wait until we collect a full batch.
struct FAtomAsyncBatcher
{
	using FWork = TUniqueFunction<void()>;
	TArray<FWork> WorkItems;

	UE::Tasks::FTask LastBatch;

	void Add(FWork&& Work)
	{
		check(IsInGameThread());

#if !WITH_EDITOR
		if (GCVarEnableAtomBatchProcessing)
		{
			if (WorkItems.Num() >= GBatchAtomAsyncBatchSize) // collected enough work
			{
				Flush();
			}
			WorkItems.Add(Forward<FWork>(Work));
		}
#else
		LastBatch = GAtomPipe.Launch(UE_SOURCE_LOCATION, MoveTemp(Work));
#endif
	}

	void Flush()
	{
		check(IsInGameThread());

		if (WorkItems.IsEmpty())
		{
			return;
		}

		LastBatch = GAtomPipe.Launch(TEXT("AtomBatch"),
			[WorkItems = MoveTemp(WorkItems)]() mutable
			{
				LLM_SCOPE_CRIWARE(ELLMTagCriWare::AtomMisc);

				for (FWork& Work : WorkItems)
				{
					Work();
				}
			},
			GAtomTaskPriority
		);
		WorkItems.Reset();
	}
};

static FAtomAsyncBatcher GAtomAsyncBatcher;

TUniqueFunction<void()> FAtomThread::GetCommandWrapper(TUniqueFunction<void()> InFunction, const TStatId InStatId)
{
	if (GCVarEnableAtomCommandLogging == 1)
	{
		return[Function = MoveTemp(InFunction), InStatId]()
		{
			CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Atom);

			FScopeCycleCounter ScopeCycleCounter(InStatId);
			FAtomThread::SetCurrentAtomThreadStatId(InStatId);

			// Time the execution of the function
			const double StartTime = FPlatformTime::Seconds();

			// Execute the function
			Function();

			// Track the longest one
			const double DeltaTime = (FPlatformTime::Seconds() - StartTime) * 1000.0f;
			if (DeltaTime > GetCurrentLongestTime())
			{
				SetLongestTimeAndId(InStatId, DeltaTime);
			}
		};
	}
	else
	{
		return[Function = MoveTemp(InFunction), InStatId]()
		{
			CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Atom);

			FScopeCycleCounter ScopeCycleCounter(InStatId);
			Function();
		};
	}
}

void FAtomThread::RunCommandOnAtomThread(TUniqueFunction<void()> InFunction, const TStatId InStatId)
{
	TUniqueFunction<void()> CommandWrapper{ GetCommandWrapper(MoveTemp(InFunction), InStatId) };
	if (IsInAtomThread())
	{
		// it's atom-thread-safe so execute the command in-place
		CommandWrapper();
	}
	else if (IsInGameThread())
	{
		// batch commands to minimise game thread overhead
		GAtomAsyncBatcher.Add(MoveTemp(CommandWrapper));
	}
	// we are on an unknown thread
	else if (IsUsingThreadedAtom())
	{
		GAtomPipe.Launch(TEXT("AtomCommand"), MoveTemp(CommandWrapper), GAtomTaskPriority);
	}
	else
	{
		// the command must be executed on the game thread
		AsyncTask(ENamedThreads::GameThread, MoveTemp(CommandWrapper));
	}
}

void FAtomThread::SetCurrentAtomThreadStatId(TStatId InStatId)
{
	FScopeLock Lock(&CurrentAtomThreadStatIdCS);
	CurrentAtomThreadStatId = InStatId;
}

FString FAtomThread::GetCurrentAtomThreadStatId()
{
	FScopeLock Lock(&CurrentAtomThreadStatIdCS);
#if STATS
	return FString(CurrentAtomThreadStatId.GetStatDescriptionANSI());
#else
	return FString(TEXT("NoStats"));
#endif
}

void FAtomThread::ResetAtomThreadTimers()
{
	FScopeLock Lock(&CurrentAtomThreadStatIdCS);
	LongestAtomThreadStatId = TStatId();
	LongestAtomThreadTimeMsec = 0.0;
}

void FAtomThread::SetLongestTimeAndId(TStatId NewLongestId, double LongestTimeMsec)
{
	FScopeLock Lock(&CurrentAtomThreadStatIdCS);
	LongestAtomThreadTimeMsec = LongestTimeMsec;
	LongestAtomThreadStatId = NewLongestId;
}

void FAtomThread::GetLongestTaskInfo(FString& OutLongestTask, double& OutLongestTaskTimeMs)
{
	FScopeLock Lock(&CurrentAtomThreadStatIdCS);
#if STATS
	OutLongestTask = FString(LongestAtomThreadStatId.GetStatDescriptionANSI());
#else
	OutLongestTask = FString(TEXT("NoStats"));
#endif
	OutLongestTaskTimeMs = LongestAtomThreadTimeMsec;
}

void FAtomThread::ProcessAllCommands()
{
	if (IsAtomThreadRunning())
	{
		GAtomAsyncBatcher.Flush();
	}
	else
	{
		check(GAtomAsyncBatcher.WorkItems.IsEmpty());
	}
}

void FAtomThread::RunCommandOnGameThread(TUniqueFunction<void()> InFunction, const TStatId InStatId)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	if (AtomCommandPrivate::CommandQueue.CanTick())
	{
		AtomCommandPrivate::CommandQueue.RunCommandOnGameThread(MoveTemp(InFunction), InStatId);
	}
	else
	{
		if (IsAtomThreadRunning())
		{
			check(IsInAtomThread());
			if (GAtomThreadUseSafeRunCommandOnGameThread)
			{
				ExecuteOnGameThread(
					TEXT("FAtomThread::RunCommandOnGameThread"),
					[Function = MoveTemp(InFunction), InStatId]()
					{
						CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Atom);
						QUICK_SCOPE_CYCLE_COUNTER(STAT_AtomThread_RunCommandOnGameThread);
						FScopeCycleCounter ScopeCycleCounter(InStatId);
						Function();
					}
				);
			}
			else
			{
				// This is the legacy behavior that will run game thread tasks anywhere on the game thread.
				// This could be inside the GC, postload, etc... which might lead to problematic behavior.
				FFunctionGraphTask::CreateAndDispatchWhenReady(
					[Function = MoveTemp(InFunction), InStatId]()
					{
						CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Atom);
						QUICK_SCOPE_CYCLE_COUNTER(STAT_AtomThread_RunCommandOnGameThread);
						FScopeCycleCounter ScopeCycleCounter(InStatId);
						Function();
					},
					TStatId(),
					nullptr,
					ENamedThreads::GameThread);
			}
		}
		else
		{
			check(IsInGameThread());
			CSV_SCOPED_TIMING_STAT_EXCLUSIVE(Atom);
			QUICK_SCOPE_CYCLE_COUNTER(STAT_AtomThread_RunCommandOnGameThread);
			FScopeCycleCounter ScopeCycleCounter(InStatId);
			InFunction();
		}
	}
#else
	if (IsAtomThreadRunning())
	{
		check(IsInAtomThread());
		FFunctionGraphTask::CreateAndDispatchWhenReady(MoveTemp(InFunction), InStatId, nullptr, ENamedThreads::GameThread);
	}
	else
	{
		check(IsInGameThread());
		FScopeCycleCounter ScopeCycleCounter(InStatId);
		InFunction();
	}
#endif
}

FDelegateHandle FAtomThread::PreGC;
FDelegateHandle FAtomThread::PostGC;
FDelegateHandle FAtomThread::PreGCDestroy;
FDelegateHandle FAtomThread::PostGCDestroy;

void FAtomThread::StartAtomThread()
{
	check(IsInGameThread());

	check(!GIsAtomThreadRunning.load(std::memory_order_relaxed));

	if (!bUseThreadedAtom)
	{
		return;
	}

	PreGC = FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddStatic(&FAtomThread::SuspendAtomThread);
	PostGC = FCoreUObjectDelegates::GetPostGarbageCollect().AddStatic(&FAtomThread::ResumeAtomThread);

	PreGCDestroy = FCoreUObjectDelegates::PreGarbageCollectConditionalBeginDestroy.AddStatic(&FAtomThread::SuspendAtomThread);
	PostGCDestroy = FCoreUObjectDelegates::PostGarbageCollectConditionalBeginDestroy.AddStatic(&FAtomThread::ResumeAtomThread);

	check(!ResumeEvent.IsValid());
	ResumeEvent = MakeUnique<UE::Tasks::FTaskEvent>(UE_SOURCE_LOCATION);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	AtomCommandPrivate::CommandQueue.StartTick();
#endif

	GIsAtomThreadRunning.store(true, std::memory_order_release);
}

void FAtomThread::StopAtomThread()
{
	if (!IsAtomThreadRunning())
	{
		return;
	}

	GAtomAsyncBatcher.Flush();
	GAtomAsyncBatcher.LastBatch.Wait();
	GAtomAsyncBatcher.LastBatch = UE::Tasks::FTask{}; // release the task as it can hold some references

	FCoreUObjectDelegates::GetPreGarbageCollectDelegate().Remove(PreGC);
	FCoreUObjectDelegates::GetPostGarbageCollect().Remove(PostGC);
	FCoreUObjectDelegates::PreGarbageCollectConditionalBeginDestroy.Remove(PreGCDestroy);
	FCoreUObjectDelegates::PostGarbageCollectConditionalBeginDestroy.Remove(PostGCDestroy);

	GIsAtomThreadRunning.store(false, std::memory_order_release);

	check(ResumeEvent.IsValid());
	ResumeEvent->Trigger(); // every FTaskEvent must be triggered before destruction to pass the check for completion
	ResumeEvent.Reset();

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	AtomCommandPrivate::CommandQueue.EndTick();
	AtomCommandPrivate::CommandQueue.FlushCommands();
#endif
}

FAtomCommandFence::~FAtomCommandFence()
{
	check(IsInGameThread());
	CSV_SCOPED_SET_WAIT_STAT(Atom);
	Fence.Wait();
}

void FAtomCommandFence::BeginFence()
{
	check(IsInGameThread());

	if (!IsAtomThreadRunning())
	{
		return;
	}

	{
		CSV_SCOPED_SET_WAIT_STAT(Atom);
		Fence.Wait();
	}

	FAtomThread::ProcessAllCommands();
	Fence = GAtomAsyncBatcher.LastBatch;
}

bool FAtomCommandFence::IsFenceComplete() const
{
	check(IsInGameThread());
	return Fence.IsCompleted();
}

/**
 * Waits for pending fence commands to retire.
 */
void FAtomCommandFence::Wait(bool bProcessGameThreadTasks) const
{
	check(IsInGameThread());

	{
		CSV_SCOPED_SET_WAIT_STAT(Atom);
		Fence.Wait();
	}

	Fence = UE::Tasks::FTask{}; // release the task as it can hold some references
}
