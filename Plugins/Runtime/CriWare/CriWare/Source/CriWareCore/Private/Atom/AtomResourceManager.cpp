/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomResourceManager.cpp
 *
 ****************************************************************************/

#include "Atom/AtomResourceManager.h"

#include "Async/Async.h"
#include "Misc/ScopeLock.h"

#include "CriWareCorePrivate.h"
#include "CriWareCoreSettings.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomSoundBase.h"
#include "Atom/AtomSoundBank.h"
#include "Atom/AtomResource.h"

#if WITH_EDITOR
#include "CriWareCoreEditorModule.h"
#include "CriWareCoreEditorAtomSettings.h"
#endif

namespace AtomResourceManagerUtils
{
	FString PrintResourceInfo(const FAtomResourceId InResourceID, const EAtomResourceScope InScope, const bool bInIsStreamed, const int32* NumHandles = nullptr)
	{
		FString ScopeStr;
		switch (InScope)
		{
		case EAtomResourceScope::Shared:	ScopeStr = TEXT("Shared");	break;
		case EAtomResourceScope::Unique:	ScopeStr = TEXT("Unique");	break;
		default:							ScopeStr = TEXT("Default");	break;
		}

		FString ResourceInfo = FString::Printf(
			TEXT("                Id: %d, Scope: %s, Streamed: %s"),
			InResourceID,
			*ScopeStr,
			bInIsStreamed ? TEXT("False") : TEXT("True"));

		if (!NumHandles)
		{
			return ResourceInfo;
		}

		ResourceInfo += FString::Printf(TEXT(", Num Handles: %d"), *NumHandles);
		return ResourceInfo;
	}
}

FAtomResourceManager::FAtomResourceManager()
	: ResourceIDCounter(0)
{
}

FAtomResourceManager::~FAtomResourceManager()
{
	{
		FScopeLock ScopeLock(&ResourceMapCriticalSection);
		int NumRemaingResources = Resources.Num();
		if (NumRemaingResources > 0)
		{
			UE_LOG(LogCriWareAtom, Display, TEXT("Destroying %d Remaining Atom resource(s)..."), NumRemaingResources);
		}
	}

	// Call the destruction events.
	IterateOverAllResources([](FAtomResourceId ResourceID, FAtomResource* Resource)
	{
		// Notify anyone listening to the device manager that we are about to destroy the resource.
		if (IAtomSoundResource* Source = Resource->GetSoundResource())
		{
			Source->DestroyAtomResource();
		}
		FAtomResourceManagerDelegates::OnAtomResourceDestroyed.Broadcast(ResourceID);
	});

	// Destroy remainings.
	{
		FScopeLock ScopeLock(&ResourceMapCriticalSection);
		Resources.Reset();
	}
}

FAtomResourceHandle FAtomResourceManager::RequestSoundResource(const FAtomLoadParams& InParams)
{
	ResourceMapCriticalSection.Lock();

	// See if we already have a resource we can use.
	for (auto& Pair : Resources)
	{
		if (CanUseResource(InParams, Pair.Value))
		{
			FAtomResourceHandle Resource = BuildNewHandle(Pair.Value, Pair.Key, InParams);
			ResourceMapCriticalSection.Unlock();

			if (InParams.bAsyncLoad)
			{
				// Auto re-load the resource. If already loaded or loading, it simply calls the completion function if present.
				Resource->AsyncLoad(InParams.OnLoadCompleted, InParams.ThreadToCallOnLoadCompletedOn, InParams.Scope);
			}
			else
			{
				Resource->MakeSureIsLoaded(InParams.Scope);

				if (InParams.OnLoadCompleted)
				{
					ExecuteOnLoadCompleteCallback(EAtomResourceLoadResult::Completed, InParams.OnLoadCompleted, InParams.ThreadToCallOnLoadCompletedOn);
				}
			}

			return Resource;
		}
	}

	// If we did not find a suitable resource, build one.
	return CreateNewResource(InParams);
}

bool FAtomResourceManager::IsValidResource(FAtomResourceId ResourceID) const
{
	FScopeLock ScopeLock(&ResourceMapCriticalSection);

	return Resources.Contains(ResourceID);
}

FAtomResourceHandle FAtomResourceManager::GetResource(FAtomResourceId InResourceID)
{
	FScopeLock ScopeLock(&ResourceMapCriticalSection);
	FResourceContainer* Container = Resources.Find(InResourceID);

	if (Container)
	{
		FAtomLoadParams Params = FAtomLoadParams();
		return BuildNewHandle(*Container, InResourceID, Params);
	}
	else
	{
		return FAtomResourceHandle();
	}
}

FAtomResource* FAtomResourceManager::GetResourceRaw(FAtomResourceId InResourceID)
{
	FScopeLock ScopeLock(&ResourceMapCriticalSection);
	if (!IsValidResource(InResourceID))
	{
		return nullptr;
	}

	FAtomResource* Resource = Resources[InResourceID].Resource.Get();
	check(Resource);

	return Resource;
}

void FAtomResourceManager::UpdateAtomResources(bool bGameTicking)
{
	// Update resources
	IterateOverAllResources([](FAtomResourceId ID, FAtomResource* Resource)
	{
		Resource->Update();
	});

	auto GetOldestUnloadableResource = [&]() -> FAtomResource*
	{
		FScopeLock ScopeLock(&ResourceMapCriticalSection);
		FAtomResourceId ResourceID = MAX_uint32;
		for (auto& Resource : Resources)
		{
			if (Resource.Value.bIsStreamed && Resource.Value.Source->CanUnloadAtomResource() && ResourceID > Resource.Key)
			{
				ResourceID = Resource.Key;
			}
		}
		return GetResourceRaw(ResourceID);
	};

	// todo check memory usage too instead of only just Atom binders limit
	auto AtomSettings = GetDefault<UCriWareCoreSettings>();
	check(AtomSettings);

	if (!AtomSettings->bUseAutomaticFileSystemManagement)
	{
		// Release a resouce by Fs Binders limitation

		// binders reserved to load some data before a resource release.
		static const int32 ReservedBinders = 4;

		CriSint32 CurNum = 0;
		CriSint32 MaxNum = 0;
		CriSint32 Limit = 0;
		FCriWareApi::criFs_GetNumUsedBinders(&CurNum, &MaxNum, &Limit);

		TArray<FAtomResource*> ResourcesToUnload;
		if (CurNum > Limit - ReservedBinders)
		{
			if (FAtomResource* Resource = GetOldestUnloadableResource())
			{
				UE_LOG(LogCriWareAtom, Display, TEXT("Auto-release Atom sound resource due to FileSystem Binders limitation."));
				if (auto SourceSoundBank = Cast<UAtomSoundBank>(Resource->GetSoundResource()))
				{
					SourceSoundBank->ReleaseResource();
				}
			}
		}
	}
}

void FAtomResourceManager::IterateOverAllResources(TUniqueFunction<void(FAtomResourceId, FAtomResource*)> ForEachResource)
{
	TArray<FAtomResourceId> ResourceIDs;
	{
		FScopeLock ScopeLock(&ResourceMapCriticalSection);
		Resources.GetKeys(ResourceIDs);
	}
	for (const FAtomResourceId ResourceID : ResourceIDs)
	{
		FAtomResourceHandle ResourceHandle = GetResource(ResourceID);
		if (ResourceHandle.IsValid())
		{
			ForEachResource(ResourceID, ResourceHandle.GetResource());
		}
	}
}

void FAtomResourceManager::IterateOverAllResources(FAtomRuntimeId InRuntimeID, TUniqueFunction<void(FAtomResourceId, FAtomResource*)> ForEachResource)
{
	TArray<FAtomResourceId> ResourceIDs;
	{
		FScopeLock ScopeLock(&ResourceMapCriticalSection);
		for (auto& Resource : Resources)
		{
			if (Resource.Value.Scope == EAtomResourceScope::Shared || Resource.Value.RuntimeID == InRuntimeID)
			{
				ResourceIDs.Add(Resource.Key);
			}
		}
	}
	for (const FAtomResourceId ResourceID : ResourceIDs)
	{
		FAtomResourceHandle ResourceHandle = GetResource(ResourceID);
		if (ResourceHandle.IsValid())
		{
			ForEachResource(ResourceID, ResourceHandle.GetResource());
		}
	}
}

#if WITH_EDITOR 
void FAtomResourceManager::IterateOverAllResourcesForStatistics(TUniqueFunction<void(FAtomResourceId, FAtomResource*, int32 NumberOfHandles, EAtomResourceScope)> ForEachResource)
{
	struct FResourceInfo
	{
		int32 NumberOfHandles;
		EAtomResourceScope Scope;
	};

	TSortedMap<FAtomResourceId, FResourceInfo> TempResourceInfo;
	{
		FScopeLock ScopeLock(&ResourceMapCriticalSection);
		for (auto& Resource : Resources)
		{
			TempResourceInfo.Add(Resource.Key, { Resource.Value.NumberOfHandlesToThisResource, Resource.Value.Scope });
		}
	}
	for (auto& Pair : TempResourceInfo)
	{
		FAtomResourceHandle ResourceHandle = GetResource(Pair.Key);
		if (ResourceHandle.IsValid())
		{
			ForEachResource(Pair.Key, ResourceHandle.GetResource(), Pair.Value.NumberOfHandles, Pair.Value.Scope);
		}
	}
}
#endif

uint32 FAtomResourceManager::GetNumAtomResources() const
{
	FScopeLock ScopeLock(&ResourceMapCriticalSection);
	return Resources.Num();
}

TArray<FAtomResource*> FAtomResourceManager::GetAtomResources()
{
	TArray<FAtomResource*> ResourceList;
	FScopeLock ScopeLock(&ResourceMapCriticalSection);
	for (auto& Pair : Resources)
	{
		ResourceList.Add(Pair.Value.Resource.Get());
	}

	return ResourceList;
}

TArray<FAtomResource*> FAtomResourceManager::GetAtomResources(FAtomRuntimeId InRuntimeID)
{
	TArray<FAtomResource*> ResourceList;
	FScopeLock ScopeLock(&ResourceMapCriticalSection);
	for (auto& Pair : Resources)
	{
		if (Pair.Value.Scope == EAtomResourceScope::Shared || InRuntimeID == Pair.Value.RuntimeID)
		{
			ResourceList.Add(Pair.Value.Resource.Get());
		}
	}

	return ResourceList;
}

bool FAtomResourceManager::MoveResourceToRuntime(FAtomResourceId InResourceID, FAtomRuntimeId InRuntimeID)
{
	FScopeLock ScopeLock(&ResourceMapCriticalSection);
	if (Resources.Contains(InResourceID))
	{
		auto& Container = Resources[InResourceID];

		if (auto Resource = Container.Resource.Get())
		{
			Resource->Unload();
		}

		Container.RuntimeID = InRuntimeID;
		return true;
	}

	return false;
}

TArray<UObject*> FAtomResourceManager::GetObjectsUsingAtomResource(FAtomResourceId InResourceID)
{
	FScopeLock ScopeLock(&ResourceMapCriticalSection);
	if (Resources.Contains(InResourceID))
	{
		return Resources[InResourceID].ObjectsUsingThisResource;
	}
	else
	{
		return TArray<UObject*>();
	}
}

EAtomResourceScope FAtomResourceManager::GetScopeOfAtomResource(FAtomResourceId InResourceID)
{
	FScopeLock ScopeLock(&ResourceMapCriticalSection);
	if (Resources.Contains(InResourceID))
	{
		return Resources[InResourceID].Scope;
	}
	else
	{
		return EAtomResourceScope::Shared;
	}
}

FAtomResourceHandle FAtomResourceManager::CreateNewResource(const FAtomLoadParams& InParams)
{
	const FAtomResourceId ResourceID = GetNewResourceID();

	FString ResourceInfo = AtomResourceManagerUtils::PrintResourceInfo(ResourceID, InParams.Scope, InParams.bIsStreamed);
	UE_LOG(LogCriWareAtom, Display, TEXT("Creating Atom Resource: %s"), *ResourceInfo);

	Resources.Emplace(ResourceID, FResourceContainer(InParams, ResourceID, this));
	FResourceContainer* ContainerPtr = Resources.Find(ResourceID);
	check(ContainerPtr);
	if (!ContainerPtr->Resource)
	{
		UE_LOG(LogCriWareAtom, Display, TEXT("Destroying Atom Resource %d: could not be initialized."), ResourceID);

		// Initializing the Atom resource failed. Remove the resource container and return an empty handle.
		Resources.Remove(ResourceID);
		return FAtomResourceHandle();
	}
	else
	{
		FAtomResourceHandle Handle = BuildNewHandle(*ContainerPtr, ResourceID, InParams);
		FAtomResourceManagerDelegates::OnAtomResourceCreated.Broadcast(ResourceID);

		// try load resource to runtime after referenced it in resource list (can async or sync)
		ResourceMapCriticalSection.Unlock();
		ContainerPtr->LoadResource(InParams, ResourceID, this);

		return Handle;
	}
}

void FAtomResourceManager::ExecuteOnLoadCompleteCallback(EAtomResourceLoadResult Result, const TFunction<void(EAtomResourceLoadResult)>& OnLoadCompleted, const ENamedThreads::Type& CallbackThread)
{
	if (CallbackThread == ENamedThreads::AnyThread)
	{
		OnLoadCompleted(Result);
	}
	else
	{
		// Dispatch an async notify.
		AsyncTask(CallbackThread, [OnLoadCompleted, Result]()
		{
			OnLoadCompleted(Result);
		});
	}
}

uint32 FAtomResourceManager::GetNewResourceID()
{
	return ++ResourceIDCounter;
}

void FAtomResourceManager::RegisterObject(UObject* InObject, FResourceContainer& InContainer, FAtomResourceId InResourceID)
{
	if (InObject)
	{
		if (!InContainer.ObjectsUsingThisResource.Contains(InObject))
		{
			UE_LOG(LogCriWareAtom, Display, TEXT("Atom resource (ID: %d) registered with object '%s'."), InResourceID, *InObject->GetName());
			InContainer.ObjectsUsingThisResource.AddUnique(InObject);
			//FAtomWorldDelegates::OnWorldRegisteredToAtom.Broadcast(InWorld, AtomID);
		}
	}
}

void FAtomResourceManager::UnregisterObject(UObject* InObject, FResourceContainer& InContainer, FAtomResourceId InResourceID)
{
	if (InObject)
	{
		if (InContainer.ObjectsUsingThisResource.Contains(InObject))
		{
			UE_LOG(LogCriWareAtom, Display, TEXT("Atom resource unregistered from object '%s'."), *InObject->GetName());
			InContainer.ObjectsUsingThisResource.Remove(InObject);
			//FAtomWorldDelegates::OnWorldUnregisteredWithAtom.Broadcast(InWorld, AtomID);
		}
	}
}

void FAtomResourceManager::IncrementResource(FAtomResourceId ResourceID)
{
	FScopeLock ScopeLock(&ResourceMapCriticalSection);

	// If there is an FAtomResourceHandle out in the world
	check(Resources.Contains(ResourceID));

	FResourceContainer& Container = Resources[ResourceID];
	Container.NumberOfHandlesToThisResource++;
}

void FAtomResourceManager::DecrementResource(FAtomResourceId ResourceID)
{
	TUniquePtr<FAtomResource> ResourceToTearDown = nullptr;

	{
		FScopeLock ScopeLock(&ResourceMapCriticalSection);

		// If there is an FAtomResourceHandle out in the world
		check(Resources.Contains(ResourceID));

		FResourceContainer& Container = Resources[ResourceID];
		check(Container.NumberOfHandlesToThisResource > 0);

		// Report Atom resource being destroyed before actual destruction
		// to allow listeners to access and respond where applicable.
		bool bDestroyingResource = false;
		if (Container.NumberOfHandlesToThisResource == 1)
		{
			bDestroyingResource = true;
			FAtomResourceManagerDelegates::OnAtomResourceDestroyed.Broadcast(ResourceID);

			auto ObjectsUsingThisResource = Container.ObjectsUsingThisResource;
			for (auto& Object : ObjectsUsingThisResource)
			{
				//Object->WillDestroy(ResourceID);
				UnregisterObject(Object, Container, ResourceID);
			}
		}

		Container.NumberOfHandlesToThisResource--;

		// If there is no longer any users of this resource, destroy it.
		if (Container.NumberOfHandlesToThisResource)
		{
			ensureMsgf(!bDestroyingResource, TEXT("AtomResource Destruction Failure: 'OnAtomResourceDestroyed' listener generated new persistent handle(s) to AtomResource."));
		}
		else
		{
			ResourceToTearDown = MoveTemp(Container.Resource);
			Resources.Remove(ResourceID);
		}
	}

	if (ResourceToTearDown && ResourceToTearDown.IsValid())
	{
		ResourceToTearDown->Teardown();
		ResourceToTearDown.Reset();
	}
}

FAtomResourceHandle FAtomResourceManager::BuildNewHandle(FResourceContainer& InContainer, FAtomResourceId InResourceID, const FAtomLoadParams& InParams)
{
	auto& Manager = FAtomRuntimeManager::Get()->GetAtomResourceManager();
	Manager.IncrementResource(InResourceID);
	Manager.RegisterObject(InParams.AssociatedObject, InContainer, InResourceID);
	return FAtomResourceHandle(InContainer.Resource.Get(), InResourceID, InParams.AssociatedObject);
}

bool FAtomResourceManager::CanUseResource(const FAtomLoadParams& InParams, const FResourceContainer& InContainer)
{
	auto Resource = InContainer.Resource.Get();
	// Shared : can be resused/reloaded by any runtime
	// Unique : usage is restricted to one runtime. Other runtimes need to create a new resource entry.
	return (InContainer.Scope == EAtomResourceScope::Shared || InParams.RuntimeID == InContainer.RuntimeID)
		&& InParams.Source == InContainer.Source;
	// debug only - check inlined or not if alredy loaded.
	//  && InParams.bIsStreamed == InContainer.bIsStreamed;
}

/*
 * FAtomRuntimeHandle
 ********************************************************/

FAtomResourceHandle::FAtomResourceHandle()
	: Object(nullptr)
	, Resource(nullptr)
	, ResourceID(INDEX_NONE)
{
}

FAtomResourceHandle::FAtomResourceHandle(FAtomResource* InResource, FAtomResourceId InID, UObject* InObject)
	: Object(nullptr)
	, Resource(InResource)
	, ResourceID(InID)
{
}

FAtomResourceHandle::FAtomResourceHandle(const FAtomResourceHandle& Other)
	: FAtomResourceHandle()
{
	*this = Other;
}

FAtomResourceHandle::FAtomResourceHandle(FAtomResourceHandle&& Other)
	: FAtomResourceHandle()
{
	*this = MoveTemp(Other);
}

FAtomResourceHandle::~FAtomResourceHandle()
{
	if (IsValid())
	{
		FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get();
		// If you are breaking here, it means that this resource or a player using it is on error or in an invalid status.
		// It also happens when an object is retaining it after runtime was teardown from engine.
		if (ensure(RuntimeManager))
		{
			RuntimeManager->GetAtomResourceManager().DecrementResource(ResourceID);
		}
	}
}

FAtomResource* FAtomResourceHandle::GetResource() const
{
	return Resource;
}

FAtomResourceId FAtomResourceHandle::GetResourceID() const
{
	return ResourceID;
}

TWeakObjectPtr<UObject> FAtomResourceHandle::GetObject() const
{
	return Object;
}

bool FAtomResourceHandle::IsValid() const
{
	return Resource != nullptr;
}

void FAtomResourceHandle::Reset()
{
	*this = FAtomResourceHandle();
}

FAtomResourceHandle& FAtomResourceHandle::operator=(const FAtomResourceHandle& Other)
{
	const bool bWasValid = IsValid();
	const FAtomResourceId OldID = ResourceID;
	UObject* OldObject = Object.Get();

	Resource = Other.Resource;
	ResourceID = Other.ResourceID;
	Object = Other.Object;

	if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
	{
		if (IsValid())
		{
			RuntimeManager->GetAtomResourceManager().IncrementResource(ResourceID);
		}

		if (bWasValid)
		{
			RuntimeManager->GetAtomResourceManager().DecrementResource(OldID);
		}
	}

	return *this;
}

FAtomResourceHandle& FAtomResourceHandle::operator=(FAtomResourceHandle&& Other)
{
	const bool bWasValid = IsValid();
	const FAtomResourceId OldID = ResourceID;
	UObject* OldObject = Object.Get();

	Resource = Other.Resource;
	ResourceID = Other.ResourceID;
	Object = Other.Object;

	if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
	{
		if (bWasValid)
		{
			RuntimeManager->GetAtomResourceManager().DecrementResource(OldID);
		}
	}

	Other.Resource = nullptr;
	Other.ResourceID = INDEX_NONE;
	Other.Object.Reset();

	return *this;
}

/*
 * FResourceContainer
 ********************************************************/

FAtomResourceManager::FResourceContainer::FResourceContainer(const FAtomLoadParams& InParams, FAtomResourceId InResourceID, FAtomResourceManager* Manager)
	: Resource(nullptr)
	, NumberOfHandlesToThisResource(0)
	, Scope(InParams.Scope)
	, Source(InParams.Source)
	, RuntimeID(InParams.RuntimeID)
	, bIsStreamed(InParams.bIsStreamed)
	, bIsLoadAsync(InParams.bAsyncLoad)
{
	// CreateResource
	if (Source)
	{
		Resource = TUniquePtr<FAtomResource>(Source->CreateAtomResource());
	}
	else
	{
		Resource = MakeUnique<FAtomResource>();
	}

	check(Resource.Get());

	if (!Resource->Init(InResourceID))
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("FAtomResource::Init Failed!"));
		Resource->Teardown();
		Resource.Reset();
	}
}

void FAtomResourceManager::FResourceContainer::LoadResource(const FAtomLoadParams& InParams, FAtomResourceId InResourceID, FAtomResourceManager* Manager)
{	
	// start load
	if (bIsLoadAsync)
	{
		Resource->AsyncLoad(InParams.OnLoadCompleted, InParams.ThreadToCallOnLoadCompletedOn, InParams.Scope);
	}
	else
	{
		Resource->MakeSureIsLoaded(InParams.Scope);

		if (InParams.OnLoadCompleted)
		{
			Manager->ExecuteOnLoadCompleteCallback(EAtomResourceLoadResult::Completed, InParams.OnLoadCompleted, InParams.ThreadToCallOnLoadCompletedOn);
		}
	}	
}

FAtomResourceManager::FResourceContainer::FResourceContainer(FResourceContainer&& Other)
{
	Resource = MoveTemp(Other.Resource);

	NumberOfHandlesToThisResource = Other.NumberOfHandlesToThisResource;
	Other.NumberOfHandlesToThisResource = 0;

	Scope = Other.Scope;
	Other.Scope = EAtomResourceScope::Unique;

	bIsStreamed = Other.bIsStreamed;
	Other.bIsStreamed = false;

	bIsLoadAsync = Other.bIsLoadAsync;
	Other.bIsLoadAsync = false;

	Source = Other.Source;
	Other.Source = nullptr;

	RuntimeID = Other.RuntimeID;
	Other.RuntimeID = INDEX_NONE;
}

FAtomResourceManager::FResourceContainer::~FResourceContainer()
{
	if (NumberOfHandlesToThisResource != 0)
	{
		UE_LOG(LogCriWareAtom, Display, TEXT("Shutting down Atom resource while %d references to it are still alive. For more information, compile with INSTRUMENT_AUDIODEVICE_HANDLES."), NumberOfHandlesToThisResource);
	}

	if (Resource)
	{
		Resource->Teardown();
		Resource.Reset();
	}
}

FAtomResourceManagerDelegates::FOnAtomResourceCreated FAtomResourceManagerDelegates::OnAtomResourceCreated;
FAtomResourceManagerDelegates::FOnAtomResourceDestroyed FAtomResourceManagerDelegates::OnAtomResourceDestroyed;
