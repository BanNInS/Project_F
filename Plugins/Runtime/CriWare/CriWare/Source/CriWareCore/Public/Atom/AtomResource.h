/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomResource.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"

#include "CriWareCore.h"
#include "CriWareUtils.h"
#include "CriWareMemory.h"
#include "Atom.h"
#include "AtomResourceManager.h"

// Forward Declarations.
class FAtomCueSheetResource;
class FAtomWaveBankResource;
class IAtomSoundResource;

/*
 * Interface for asset using an Atom native resource.
 */
struct IAtomResource
{
	virtual void OnAtomResourceLoadCompleted() {}
};

/*
 * Class to define a resource in Atom runtime.
 */
class CRIWARECORE_API FAtomResource
{
protected:

	FAtomResourceId ResourceID;

	typedef TUniqueFunction<void(IAtomSoundResource* Dependency)> FIterateDependenciesCallback;

public:

	FAtomResource() : ResourceID(INDEX_NONE) {}
	virtual ~FAtomResource() {}

	virtual bool Init(FAtomResourceId InResourceID)
	{ 
		ResourceID = InResourceID;
		
		return true;
	}
	
	// Remove data resource from source asset. Works only if not shared with an other resource.
	virtual void Teardown()  
	{
		// inform all consumers to stop accessing this resouce because it will be removed.
	}

	// on atom tick
	virtual void Update() {}

	// load resource to Atom library, load data in source asset if needed.
	virtual bool AsyncLoad(TFunction<void(EAtomResourceLoadResult)> OnLoadCompleted, ENamedThreads::Type CallbackThread, EAtomResourceScope Scope) { return false; }
	
	virtual bool IsLoaded() const { return false; }
	virtual bool IsAsyncLoading() const { return false; }
	virtual void MakeSureIsLoaded(EAtomResourceScope Scope) {}

	// Unload the resource from Atom library.
	virtual void Unload() {}

	virtual IAtomSoundResource* GetSoundResource() const { return nullptr; }
	virtual void IterateOverSoundResourceDependencies(const FIterateDependenciesCallback& Callback) const {}

	// Dynamic cast methods.
	virtual FAtomCueSheetResource* GetCueSheetResource() { return nullptr; }
	virtual FAtomWaveBankResource* GetWaveBankResource() { return nullptr; }
};

/*
 * Template for Atom resource that uses a cri data stream handler.
 */
template<class TCriHandleType>
class FAtomStreamableResource
	: public FAtomResource
{
	// Ensure that this uses a subclass of FCriHandle.
	static_assert(std::is_base_of_v<FCriHandle, TCriHandleType>, "TCriHandleType must be derived from FCriHandle");

public:

	FAtomStreamableResource(IAtomSoundResource* InSource)
		: Source(InSource)
	{}

	inline bool IsUsingFileSystemBinder() const
	{
		return FsBinder.IsValid();
	}

	//~ Begin FAtomResource
	virtual void Update() override
	{
		FScopeLock Lock(&LoadTaskLock);
		if (LoadTask && LoadTask->IsComplete())
		{
			// cleaup worker task
			delete LoadTask;
			LoadTask = nullptr;
		}
	};

	virtual bool IsLoaded() const override
	{
		return !IsAsyncLoading() && CriResource.IsValid();
	}

	virtual bool IsAsyncLoading() const override;
	virtual void MakeSureIsLoaded(EAtomResourceScope Scope) override;

	virtual IAtomSoundResource* GetSoundResource() const override
	{ 
		return Source;
	}
	//~ End FAtomResource

protected:

	void SetNativeHandle(TCriHandleType& InCriHandle)
	{
		CriResource = MoveTemp(InCriHandle);
	}

	void SetFsHandle(FCriFsBinderPtr& InFsBinder)
	{
		bIsStreamed = InFsBinder.IsValid();
		if (bIsStreamed)
		{
			FsBinder = MoveTemp(InFsBinder);
		}
	}

	void SetFsBinderWork(int32 WorkSize)
	{
		// manually call the FS allocator...
		FCriWareAllocator& Allocator = FCriWareCore::GetPlatform().Atom()->GetMemoryAllocator(ECriWareMemoryType::FileSystemMemory);
		FsBinderWork = FCriWareAllocator_Native::AllocateMemory((void*)&Allocator, (CriUint32)WorkSize);
	}

	void ReleaseFsBinderWork()
	{
		if (FsBinderWork)
		{
			// manually call the FS allocator...
			FCriWareAllocator& Allocator = FCriWareCore::GetPlatform().Atom()->GetMemoryAllocator(ECriWareMemoryType::FileSystemMemory);
			FCriWareAllocator_Native::FreeMemory((void*)&Allocator, FsBinderWork);
			FsBinderWork = nullptr;
		}
	}

	// todo: a data source proxy/ptr 
	IAtomSoundResource* Source;

	class FLoadTask : public FRunnable {
	public:

		FLoadTask(TFunction<void(bool& bShutdown)> InTaskFunc)
			: TaskFunc(InTaskFunc)
		{
			Thread = FRunnableThread::Create(this, TEXT("AtomLoadTask"), 0, TPri_Highest);
		};

		~FLoadTask()
		{
			bIsExit = true;
			Thread->WaitForCompletion();
			delete Thread;
			Thread = nullptr;
		}

		bool IsComplete() { return bIsExit; }
		void WaitUntilTaskCompletes() { if (Thread) { Thread->WaitForCompletion(); } }

		virtual bool Init() override { return true; }
		virtual uint32 Run() override { if (TaskFunc) { TaskFunc(bShutdown); } return 0; }
		virtual void Exit() override { bIsExit = true; }
		virtual void Stop() override { bShutdown = true; }

	private:
		FRunnableThread* Thread;
		bool bShutdown = false;
		bool bIsExit = false;
		TFunction<void(bool& bShutdown)> TaskFunc;
	};

	/** Task used to load data. */
	mutable FCriticalSection LoadTaskLock;
	FLoadTask* LoadTask = nullptr;

	/** Multicast delegate trigged on loading completion. */
	struct FOnLoadCompletedDelegate
	{
		TFunction<void(EAtomResourceLoadResult)> Callback;
		ENamedThreads::Type CallThread;
	};

	/** Multicast event on loading completion. */
	struct FOnLoadCompletedEvent
	{
	private:

		mutable FCriticalSection EventLock;
		TArray<FOnLoadCompletedDelegate> Delegates;
		bool bWasBroadcasted = false;

	public:

		void Add(TFunction<void(EAtomResourceLoadResult)> Delegate, ENamedThreads::Type CallThread)
		{
			FScopeLock Lock(&EventLock);
			Delegates.Add({ Delegate, CallThread });
		}

		void Broadcast(EAtomResourceLoadResult Result)
		{
			FScopeLock Lock(&EventLock);
			for (auto Delegate : Delegates)
			{
				FAtomResourceManager::ExecuteOnLoadCompleteCallback(Result, Delegate.Callback, Delegate.CallThread);
			}
			Delegates.Empty();
			bWasBroadcasted = true;
		}

		void Reset()
		{
			FScopeLock Lock(&EventLock);
			Delegates.Empty();
			bWasBroadcasted = false;
		}

		bool WasBroadcasted() const
		{
			FScopeLock Lock(&EventLock);
			return bWasBroadcasted;
		}
	};

	FOnLoadCompletedEvent OnLoadCompletedEvent;

	TCriHandleType CriResource;
	FCriFsBinderPtr FsBinder;
	void* FsBinderWork = nullptr;

	/** For inlined resource, When scope is Unique, keep data localy for exclusive access. */
	uint8* LocalData = nullptr;

	bool bIsStreamed = false;
};

template <typename T>
bool FAtomStreamableResource<T>::IsAsyncLoading() const
{
	FScopeLock Lock(&LoadTaskLock);
	return LoadTask && !LoadTask->IsComplete();
}

template <typename T>
void FAtomStreamableResource<T>::MakeSureIsLoaded(EAtomResourceScope Scope)
{
	AsyncLoad(nullptr, ENamedThreads::AnyThread, Scope);
	if (IsAsyncLoading())
	{
		FScopeLock Lock(&LoadTaskLock);
		LoadTask->WaitUntilTaskCompletes();
	}
}
