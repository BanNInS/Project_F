/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomThread.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "Tasks/Pipe.h"

#include "CriWareDefines.h"

////////////////////////////////////
// Atom thread API
// (the naming is outdated as the thread was replaced by piped tasks)
////////////////////////////////////

DECLARE_STATS_GROUP(TEXT("Atom Thread Commands"), STATGROUP_AtomThreadCommands, STATCAT_Advanced);

class FAtomThread
{
private:
	/** Whether to run with an atom thread */
	static bool bUseThreadedAtom;

	static TUniquePtr<UE::Tasks::FTaskEvent> ResumeEvent;
	static int32 SuspendCount;  // accessed only from GT

	// GC callback handles
	static FDelegateHandle PreGC;
	static FDelegateHandle PostGC;
	static FDelegateHandle PreGCDestroy;
	static FDelegateHandle PostGCDestroy;

	/** Stat id of the currently executing atom thread command. */
	static TStatId CurrentAtomThreadStatId;
	static TStatId LongestAtomThreadStatId;
	static double LongestAtomThreadTimeMsec;
	static FCriticalSection CurrentAtomThreadStatIdCS;

	/** Sets the current atom thread stat id. */
	static void SetCurrentAtomThreadStatId(TStatId InStatId);
	static void SetLongestTimeAndId(TStatId NewLongestId, double LongestTimeMsec);
	static double GetCurrentLongestTime() { return LongestAtomThreadTimeMsec; }

	// a helper to apply stats and optional additional logging
	static TUniqueFunction<void()> GetCommandWrapper(TUniqueFunction<void()> InFunction, const TStatId InStatId);

public:

	/** Starts the atom thread. */
	static CRIWARECORE_API void StartAtomThread();

	/** Stops the atom thread. */
	static CRIWARECORE_API void StopAtomThread();

	/** Execute a command on the atom thread. If it's safe the command will execute immediately. */
	static CRIWARECORE_API void RunCommandOnAtomThread(TUniqueFunction<void()> InFunction, const TStatId InStatId = TStatId());

	/** Processes all enqueued atom thread commands. */
	static CRIWARECORE_API void ProcessAllCommands();

	/** Execute a (presumably atom) command on the game thread. If GIsAtomThreadRunning is false the command will execute immediately */
	static CRIWARECORE_API void RunCommandOnGameThread(TUniqueFunction<void()> InFunction, const TStatId InStatId = TStatId());

	static CRIWARECORE_API void SetUseThreadedAtom(bool bInUseThreadedAtom);
	static CRIWARECORE_API bool IsUsingThreadedAtom();

	static CRIWARECORE_API void SuspendAtomThread();
	static CRIWARECORE_API void ResumeAtomThread();

	/** Retrieves the current atom thread stat id. Useful for reporting when an atom thread command stalls or deadlocks. */
	static CRIWARECORE_API FString GetCurrentAtomThreadStatId();

	static CRIWARECORE_API void ResetAtomThreadTimers();

	static CRIWARECORE_API void GetLongestTaskInfo(FString& OutLongestTask, double& OutLongestTaskTimeMs);
};


/** Suspends the atom thread for the duration of the lifetime of the object */
struct FAtomThreadSuspendContext
{
	FAtomThreadSuspendContext()
	{
		FAtomThread::SuspendAtomThread();
	}

	~FAtomThreadSuspendContext()
	{
		FAtomThread::ResumeAtomThread();
	}
};

////////////////////////////////////
// Atom fences
////////////////////////////////////

/**
* Used to track pending atom commands from the game thread.
*/
class CRIWARECORE_API FAtomCommandFence
{
public:

	~FAtomCommandFence();

	/**
	* Adds a fence command to the atom command queue.
	* Conceptually, the pending fence count is incremented to reflect the pending fence command.
	* Once the rendering thread has executed the fence command, it decrements the pending fence count.
	*/
	void BeginFence();

	/**
	* Waits for pending fence commands to retire.
	* @param bProcessGameThreadTasks, if true we are on a short callstack where it is safe to process arbitrary game thread tasks while we wait
	*/
	void Wait(bool bProcessGameThreadTasks = false) const;

	// return true if the fence is complete
	bool IsFenceComplete() const;

private:

	/** The last atom batch task **/
	mutable UE::Tasks::FTask Fence;
};
