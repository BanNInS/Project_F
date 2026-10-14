/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomRuntimeManager.cpp
 *
 ****************************************************************************/

 #include "Atom/AtomRuntimeManager.h"

#include "CriWare.h"
#include "CriWareCoreSettings.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomResourceManager.h"
#include "Atom/AtomSoundBank.h"
#include "Atom/AtomRuntimeUnrealAudio.h"

#if ENABLE_ATOM_MONITOR
#include "Atom/Profiling/AtomMonitor.h"
#endif

#if WITH_EDITOR
#include "Editor.h"
#include "CriWareCoreEditorModule.h"
#include "CriWareCoreEditorAtomSettings.h"
#endif

static int32 GCVarEnableAtomThreadWait = 1;
TAutoConsoleVariable<int32> CVarEnableAtomThreadWait(
	TEXT("AtomThread.EnableAtomThreadWait"),
	GCVarEnableAtomThreadWait,
	TEXT("Enables waiting on the Atom thread to finish its commands.\n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static int32 CVarIsAtomVisualizeEnabled = 0;
FAutoConsoleVariableRef CVarAtomVisualizeEnabled(
	TEXT("atom.3dVisualize.Enabled"),
	CVarIsAtomVisualizeEnabled,
	TEXT("Whether or not Atom visualization is enabled. \n")
	TEXT("0: Not Enabled, 1: Enabled"),
	ECVF_Default);

static FAutoConsoleCommand GReportAtomRuntimesCommand(
	TEXT("atom.ReportAtomRuntimes"),
	TEXT("This will log any active Atom runtimes alive right now."),
	FConsoleCommandDelegate::CreateStatic([]()
		{
			FAtomRuntimeManager::Get()->LogListOfAtomRuntimes();
		})
	);

namespace AtomRuntimeManagerUtils
{
	FString PrintRuntimeInfo(const FAtomRuntimeId InRuntimeID, const EAtomRuntimeScope InScope, const bool bInIsNonRealtime, const int32* NumHandles = nullptr/*, const TMap<uint32, FString>* InStackWalk = nullptr*/)
	{
		FString ScopeStr;
		switch (InScope)
		{
		case EAtomRuntimeScope::Shared:	ScopeStr = TEXT("Shared");	break;
		case EAtomRuntimeScope::Unique:	ScopeStr = TEXT("Unique");	break;
		case EAtomRuntimeScope::Default:
		default:						ScopeStr = TEXT("Default");	break;
		}

		FString RuntimeInfo = FString::Printf(
			TEXT("                Id: %d, Scope: %s, Realtime: %s"),
			InRuntimeID,
			*ScopeStr,
			bInIsNonRealtime ? TEXT("False") : TEXT("True"));

		if (!NumHandles)
		{
			return RuntimeInfo;
		}

		RuntimeInfo += FString::Printf(TEXT(", Num Handles: %d"), *NumHandles);

/*#if INSTRUMENT_AUDIODEVICE_HANDLES
		if (InStackWalk)
		{
			DeviceInfo += TEXT("\n            Active Handles:\n\n");
			for (const TPair<uint32, FString>& StackWalkPairs : *InStackWalk)
			{
				DeviceInfo += StackWalkPairs.Value;
				DeviceInfo += TEXT("\n\n");
			}
		}
#endif*/

		return RuntimeInfo;
	}
}


FAtomRuntimeManager* FAtomRuntimeManager::Singleton = nullptr;

FAtomRuntimeManager::FAtomRuntimeManager()
	: AtomRuntimeModule(nullptr)
	, RuntimeIDCounter(0)
	, ActiveAtomRuntimeID(INDEX_NONE)
{
	ResourceManager = new FAtomResourceManager();

#if ENABLE_ATOM_DEBUG
	AtomDebugger = MakeUnique<Atom::FAtomDebugger>();

	// Check for a command line debug sound argument.
	FString DebugSound;
	if (FParse::Value(FCommandLine::Get(), TEXT("AtomDebugSound="), DebugSound))
	{
		GetDebugger().SetAtomDebugSound(*DebugSound);
	}

#endif // ENABLE_ATOM_DEBUG

#if ENABLE_ATOM_MONITOR
	AtomMonitor = MakeShared<FAtomMonitor>();
#endif  // ENABLE_ATOM_MONITOR
}

FAtomRuntimeManager::~FAtomRuntimeManager()
{
	UE_LOG(LogCriWareAtom, Display, TEXT("Beginning Atom Manager Shutdown (Module: %s)..."), *AtomMixerModuleName);

	TArray<FAtomRuntimeId> RuntimeIDs;
	{
		FScopeLock ScopeLock(&RuntimeMapCriticalSection);
		Runtimes.GetKeys(RuntimeIDs);
	}

	if (RuntimeIDs.Num() > 0)
	{
		UE_LOG(LogCriWareAtom, Display, TEXT("Destroying %d Remaining Atom runtime(s)..."), RuntimeIDs.Num());

		// Notify anyone listening to the device manager that we are about to destroy the runtime.
		for (FAtomRuntimeId RuntimeID : RuntimeIDs)
		{
			FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.Broadcast(RuntimeID);
		}
	}

	FAtomThread::StopAtomThread();

	TMap<FAtomRuntimeId, FAtomRuntimeContainer> RuntimesToShutdown;
	{
		FScopeLock ScopeLock(&RuntimeMapCriticalSection);
		RuntimesToShutdown = MoveTemp(Runtimes);
	}

	// Can only be destroyed outside of critical section to avoid a deadlock,
	// but need to remove the runtime from the manager's list in case of calls
	// being executed from individual runtime render thread commands attempting
	// to access their given runtime. This is a means to communicate to pending
	// commands the runtime is no longer available without destroying it mid-flight.
	RuntimesToShutdown.Reset();
	MainAtomRuntimeHandle.Reset();

	if (ResourceManager)
	{
		delete ResourceManager;
		ResourceManager = nullptr;
	}

#if WITH_ENGINE
	FWorldDelegates::OnPreWorldInitialization.Remove(OnPreWorldInitializationHandle);
	FWorldDelegates::OnPreWorldFinishDestroy.Remove(OnPreWorldFinishDestroyHandle);
	FWorldDelegates::OnWorldBeginTearDown.Remove(OnWorldBeginTearDownHandle);
	FWorldDelegates::OnPostWorldCleanup.Remove(OnPostWorldCleanupHandle);
#endif
#if WITH_EDITOR
	FEditorDelegates::MapChange.Remove(OnEditorMapChangeHandle);
#endif

	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
}

FAtomRuntimeHandle FAtomRuntimeManager::GetAtomRuntimeFromWorld(const UWorld* InWorld)
{
	// todo: instead of create new handles: an array of handles with registered world that release when worlds are destroyed

	if (!InWorld || !InWorld->bAllowAudioPlayback || InWorld->GetNetMode() == NM_DedicatedServer)
	{
		return FAtomRuntimeHandle();
	}

	if (!GCriWare || !GCriWare->GetCriWareCore() || !GCriWare->GetCriWareCore()->GetAtomRuntimeManager())
	{
		return FAtomRuntimeHandle();
	}

	FAtomRuntimeManager* AtomRuntimeManager = GCriWare->GetCriWareCore()->GetAtomRuntimeManager();

	for (auto& AtomRuntime : GCriWare->GetCriWareCore()->GetAtomRuntimeManager()->Runtimes)
	{
		auto& Container = AtomRuntime.Value;

		if (Container.WorldsUsingThisRuntime.ContainsByPredicate([InWorld](const UWorld* World) { return World && InWorld->GetFName() == World->GetFName(); }))
		{
			FAtomRuntimeParams Params = FAtomRuntimeParams();
			Params.AssociatedWorld = const_cast<UWorld*>(InWorld); // atom never modifies a world
			return AtomRuntimeManager->BuildNewHandle(Container, Container.Runtime->GetAtomRuntimeID(), Params);
		}
	}

	return FAtomRuntimeHandle();
}

FAtomRuntime* FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(const UWorld* InWorld)
{
	// todo: instead of create new handles: an array of handles with registered world that release when worlds are destroyed

	FAtomRuntimeHandle AtomRuntineHandle = GetAtomRuntimeFromWorld(InWorld);

	if (AtomRuntineHandle)
	{
		return AtomRuntineHandle.GetAtomRuntime();
	}
	else if (GCriWare)
	{
		return GCriWare->GetCriWareCore()->GetMainAtomRuntimeRaw();
	}
	else
	{
		return nullptr;
	}
}

FAtomRuntime* FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(const UObject* WorldContextObject)
{
	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!ThisWorld || !ThisWorld->bAllowAudioPlayback || ThisWorld->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	if (!GCriWare || !GCriWare->GetCriWareCore() || !GCriWare->GetCriWareCore()->GetAtomRuntimeManager())
	{
		return nullptr;
	}

	for (auto& AtomRuntime : GCriWare->GetCriWareCore()->GetAtomRuntimeManager()->Runtimes)
	{
		auto& Container = AtomRuntime.Value;

		if (Container.WorldsUsingThisRuntime.ContainsByPredicate([ThisWorld](const UWorld* World) { return World && ThisWorld->GetFName() == World->GetFName(); }))
		{
			return Container.Runtime;
		}
	}

	return nullptr;
}

FAtomRuntimeParams FAtomRuntimeManager::GetDefaultParamsForNewWorld()
{
	bool bCreateNewRuntimeForPlayInEditor = false;

#if WITH_EDITOR
	// GIsEditor is necessary here to ignore this setting for -game situations.
	if (GIsEditor)
	{
		bCreateNewRuntimeForPlayInEditor = GetDefault<UCriWareCoreEditorAtomSettings>()->bCreateNewAtomRuntimeForPlayInEditor;
	}
#endif

	FAtomRuntimeParams Params;
	Params.Scope = bCreateNewRuntimeForPlayInEditor ? EAtomRuntimeScope::Unique : EAtomRuntimeScope::Shared;

	return Params;
}

FAtomRuntimeHandle FAtomRuntimeManager::RequestAtomRuntime(const FAtomRuntimeParams& InParams)
{
	FScopeLock ScopeLock(&RuntimeMapCriticalSection);
	if (InParams.Scope == EAtomRuntimeScope::Unique)
	{
		return CreateNewRuntime(InParams);
	}
	else
	{
		// See if we already have a runtime we can use.
		for (auto& Runtime : Runtimes)
		{
			if (CanUseAtomRuntime(InParams, Runtime.Value))
			{
				RegisterWorld(InParams.AssociatedWorld, Runtime.Key);
				return BuildNewHandle(Runtime.Value, Runtime.Key, InParams);
			}
		}

		// If we did not find a suitable runtime, build one.
		return CreateNewRuntime(InParams);
	}
}

void FAtomRuntimeManager::RegisterWorld(UWorld* InWorld, FAtomRuntimeId RuntimeID)
{
	if (!InWorld)
	{
		return;
	}

	if (FAtomRuntimeContainer* RuntimeContainer = Runtimes.Find(RuntimeID))
	{
		if (!RuntimeContainer->WorldsUsingThisRuntime.Contains(InWorld))
		{
			UE_LOG(LogCriWareAtom, Display, TEXT("Atom runtime (ID: %d) registered with world '%s'."), RuntimeID, *InWorld->GetName());
			RuntimeContainer->WorldsUsingThisRuntime.AddUnique(InWorld);
			FAtomRuntimeWorldDelegates::OnWorldRegisteredToAtomRuntime.Broadcast(InWorld, RuntimeID);
		}
	}
}

void FAtomRuntimeManager::UnregisterWorld(UWorld* InWorld, FAtomRuntimeId RuntimeID)
{
	if (!InWorld)
	{
		return;
	}

	if (FAtomRuntimeContainer* AtomContainer = Runtimes.Find(RuntimeID))
	{
		if (AtomContainer->WorldsUsingThisRuntime.Contains(InWorld))
		{
			UE_LOG(LogCriWareAtom, Display, TEXT("Atom runtime unregistered from world '%s'."), *InWorld->GetName());
			AtomContainer->WorldsUsingThisRuntime.Remove(InWorld);
			FAtomRuntimeWorldDelegates::OnWorldUnregisteredWithAtomRuntime.Broadcast(InWorld, RuntimeID);
		}

		if (MainAtomRuntimeHandle.World.Get() == InWorld)
		{
			MainAtomRuntimeHandle.World.Reset();
		}
	}
}

bool FAtomRuntimeManager::IsVisualizeDebug3dEnabled() const
{
#if ENABLE_ATOM_DEBUG
	return GetDebugger().IsVisualizeDebug3dEnabled() || CVarIsAtomVisualizeEnabled;
#else // ENABLE_ATOM_DEBUG
	return false;
#endif // !ENABLE_ATOM_DEBUG
}

void FAtomRuntimeManager::ToggleVisualize3dDebug()
{
#if ENABLE_ATOM_DEBUG
	if (!IsInAtomThread())
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.ToggleVisualize3dDebug"), STAT_AtomToggleVisualize3dDebug, STATGROUP_AtomThreadCommands);

		FAtomRuntimeManager* AtomRuntimeManager = this;
		FAtomThread::RunCommandOnAtomThread([AtomRuntimeManager]()
			{
				AtomRuntimeManager->ToggleVisualize3dDebug();

			}, GET_STATID(STAT_AtomToggleVisualize3dDebug));

		return;
	}

	GetDebugger().ToggleVisualizeDebug3dEnabled();
#endif // ENABLE_AUDIO_DEBUG
}

float FAtomRuntimeManager::GetDynamicSoundVolume(EAtomSoundType SoundType, const FName& SoundName) const
{
	check(IsInAtomThread());

	TTuple<EAtomSoundType, FName> SoundKey(SoundType, SoundName);
	if (const float* Volume = DynamicSoundVolumes.Find(SoundKey))
	{
		return FMath::Max(0.0f, *Volume);
	}

	return 1.0f;
}

void FAtomRuntimeManager::ResetAllDynamicSoundVolumes()
{
	if (!IsInAtomThread())
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.ResetAllDynamicSoundVolumes"), STAT_AtomResetAllDynamicSoundVolumes, STATGROUP_AtomThreadCommands);

		FAtomRuntimeManager* AtomRuntimeManager = this;
		FAtomThread::RunCommandOnAtomThread([AtomRuntimeManager]()
		{
			AtomRuntimeManager->ResetAllDynamicSoundVolumes();

		}, GET_STATID(STAT_AtomResetAllDynamicSoundVolumes));
		return;
	}

	DynamicSoundVolumes.Reset();
	DynamicSoundVolumes.Shrink();
}

void FAtomRuntimeManager::ResetDynamicSoundVolume(EAtomSoundType SoundType, const FName& SoundName)
{
	if (!IsInAtomThread())
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.ResetSoundCueTrimVolume"), STAT_AtomResetSoundCueTrimVolume, STATGROUP_AtomThreadCommands);

		FAtomRuntimeManager* AtomRuntimeManager = this;
		FAtomThread::RunCommandOnAtomThread([AtomRuntimeManager, SoundType, SoundName]()
		{
			AtomRuntimeManager->ResetDynamicSoundVolume(SoundType, SoundName);

		}, GET_STATID(STAT_AtomResetSoundCueTrimVolume));
		return;
	}

	TTuple<EAtomSoundType, FName> Key(SoundType, SoundName);
	DynamicSoundVolumes.Remove(Key);
}

void FAtomRuntimeManager::SetDynamicSoundVolume(EAtomSoundType SoundType, const FName& SoundName, float Volume)
{
	if (!IsInAtomThread())
	{
		DECLARE_CYCLE_STAT(TEXT("FAtomThreadTask.SetDynamicSoundVolume"), STAT_AtomSetDynamicSoundVolume, STATGROUP_AtomThreadCommands);

		FAtomRuntimeManager* AtomRuntimeManager = this;
		FAtomThread::RunCommandOnAtomThread([AtomRuntimeManager, SoundType, SoundName, Volume]()
		{
			AtomRuntimeManager->SetDynamicSoundVolume(SoundType, SoundName, Volume);

		}, GET_STATID(STAT_AtomSetDynamicSoundVolume));
		return;
	}

	TTuple<EAtomSoundType, FName> Key(SoundType, SoundName);
	DynamicSoundVolumes.FindOrAdd(Key) = FMath::Clamp(Volume, 0.0f, ATOM_MAX_VOLUME);;
}

void FAtomRuntimeManager::LogListOfAtomRuntimes()
{
	FString ListOfRuntimes;

	for (const TPair<FAtomRuntimeId, FAtomRuntimeContainer>& RuntimePair : Runtimes)
	{
		ListOfRuntimes += AtomRuntimeManagerUtils::PrintRuntimeInfo(
			RuntimePair.Key,
			RuntimePair.Value.Scope,
			RuntimePair.Value.bIsNonRealtime,
			&RuntimePair.Value.NumberOfHandlesToThisRuntime
/*#if INSTRUMENT_AUDIODEVICE_HANDLES
			, &DevicePair.Value.HandleCreationStackWalks
#endif*/
		);
	}

	UE_LOG(LogAudio, Display, TEXT("Active Atom Runtimes:\n%s"), *ListOfRuntimes);
}

FAtomRuntimeHandle FAtomRuntimeManager::CreateNewRuntime(const FAtomRuntimeParams& InParams)
{
	const FAtomRuntimeId RuntimeID = GetNewRuntimeID();

	FString RuntimeInfo = AtomRuntimeManagerUtils::PrintRuntimeInfo(RuntimeID, InParams.Scope, InParams.bIsNonRealtime);
	UE_LOG(LogCriWareAtom, Display, TEXT("Creating Atom Runtime: %s"), *RuntimeInfo);
	Runtimes.Emplace(RuntimeID, FAtomRuntimeContainer(InParams, RuntimeID, this));

	FAtomRuntimeContainer* ContainerPtr = Runtimes.Find(RuntimeID);
	check(ContainerPtr);
	if (!ContainerPtr->Runtime)
	{
		UE_LOG(LogCriWareAtom, Display, TEXT("Destroying Atom Runtime %d: could not be initialized. Check AtomModuleName and AtomMixerModuleName in [Platform]Engine.ini."), RuntimeID);

		// Initializing the Atom runtime failed. Remove the runtime container and return an empty handle.
		Runtimes.Remove(RuntimeID);
		return FAtomRuntimeHandle();
	}
	else
	{
		RegisterWorld(InParams.AssociatedWorld, RuntimeID);

		FAtomRuntimeHandle Handle = BuildNewHandle(*ContainerPtr, RuntimeID, InParams);
		FAtomRuntimeManagerDelegates::OnAtomRuntimeCreated.Broadcast(RuntimeID);
		return Handle;
	}
}

uint32 FAtomRuntimeManager::GetNewRuntimeID()
{
	return ++RuntimeIDCounter;
}

void FAtomRuntimeManager::IncrementRuntime(FAtomRuntimeId RuntimeID)
{
	FScopeLock ScopeLock(&RuntimeMapCriticalSection);

	// If there is an FAtomRuntimeHandle out in the world
	check(Runtimes.Contains(RuntimeID));

	FAtomRuntimeContainer& Container = Runtimes[RuntimeID];
	Container.NumberOfHandlesToThisRuntime++;
}

void FAtomRuntimeManager::DecrementRuntime(FAtomRuntimeId RuntimeID, UWorld* InWorld)
{
	FAtomRuntime* AtomToTearDown = nullptr;

	{
		FScopeLock ScopeLock(&RuntimeMapCriticalSection);

		// If there is an FAtomRuntimeHandle out in the world
		if (Runtimes.Contains(RuntimeID))
		{
			FAtomRuntimeContainer& Container = Runtimes[RuntimeID];
			check(Container.NumberOfHandlesToThisRuntime > 0);

			// Report Atom runtime being destroyed before actual destruction
			// to allow listeners to access and respond where applicable.
			bool bDestroyingAtom = false;
			if (Container.NumberOfHandlesToThisRuntime == 1)
			{
				bDestroyingAtom = true;
				FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.Broadcast(RuntimeID);

				// If this is the active Atom runtime and being destroyed, set the main runtime as the active runtime.
				if (RuntimeID == ActiveAtomRuntimeID)
				{
					SetActiveAtomRuntime(MainAtomRuntimeHandle.GetRuntimeID());
				}

				UnregisterWorld(InWorld, RuntimeID);
			}

			Container.NumberOfHandlesToThisRuntime--;

			// If there is no longer any users of this runtime, destroy it.
			if (Container.NumberOfHandlesToThisRuntime)
			{
				ensureMsgf(!bDestroyingAtom, TEXT("AtomRuntime Destruction Failure: 'OnAtomDestroyed' listener generated new persistent handle(s) to AtomRuntime."));
			}
			else
			{
				Swap(AtomToTearDown, Container.Runtime);
				Runtimes.Remove(RuntimeID);
			}
		}
	}

	if (AtomToTearDown)
	{
		//AtomToTearDown->FadeOut();
		AtomToTearDown->Teardown();
		delete AtomToTearDown;
	}
}

FAtomRuntimeHandle FAtomRuntimeManager::BuildNewHandle(FAtomRuntimeContainer& Container, FAtomRuntimeId RuntimeID, const FAtomRuntimeParams& InParams)
{
	FAtomRuntimeManager::Get()->IncrementRuntime(RuntimeID);
	return FAtomRuntimeHandle(Container.Runtime, RuntimeID, InParams.AssociatedWorld);
}

bool FAtomRuntimeManager::CanUseAtomRuntime(const FAtomRuntimeParams& InParams, const FAtomRuntimeContainer& InContainer)
{
	return InContainer.Scope == EAtomRuntimeScope::Shared
		&& InParams.AtomModule == InContainer.SpecifiedModule
		&& InParams.bIsNonRealtime == InContainer.bIsNonRealtime;
}

bool FAtomRuntimeManager::IsValidAtomRuntime(FAtomRuntimeId RuntimeID) const
{
	FScopeLock ScopeLock(&RuntimeMapCriticalSection);

	return Runtimes.Contains(RuntimeID);
}

FAtomRuntimeHandle FAtomRuntimeManager::GetAtomRuntime(FAtomRuntimeId InRuntimeID)
{
	FScopeLock ScopeLock(&RuntimeMapCriticalSection);
	FAtomRuntimeContainer* Container = Runtimes.Find(InRuntimeID);

	if (Container)
	{
		FAtomRuntimeParams Params = FAtomRuntimeParams();
		return BuildNewHandle(*Container, InRuntimeID, Params);
	}
	else
	{
		return FAtomRuntimeHandle();
	}
}

FAtomRuntime* FAtomRuntimeManager::GetAtomRuntimeRaw(FAtomRuntimeId InRuntimeID)
{
	FScopeLock ScopeLock(&RuntimeMapCriticalSection);
	if (!IsValidAtomRuntime(InRuntimeID))
	{
		return nullptr;
	}

	FAtomRuntime* AtomRuntime = Runtimes[InRuntimeID].Runtime;
	check(AtomRuntime);

	return AtomRuntime;
}

const FAtomRuntime* FAtomRuntimeManager::GetAtomRuntimeRaw(FAtomRuntimeId InRuntimeID) const
{
	FScopeLock ScopeLock(&RuntimeMapCriticalSection);
	if (!IsValidAtomRuntime(InRuntimeID))
	{
		return nullptr;
	}

	const FAtomRuntime* AtomRuntime = Runtimes[InRuntimeID].Runtime;
	check(AtomRuntime);

	return AtomRuntime;
}

void FAtomRuntimeManager::SetAtomRuntimeWorld(UWorld& InWorld, FAtomRuntimeId InRuntimeID)
{
	FScopeLock ScopeLock(&RuntimeMapCriticalSection);

	// get atom from world
	FAtomRuntimeId OldRuntimeID = INDEX_NONE;
	for (auto& Runtime : Runtimes)
	{
		auto& Container = Runtime.Value;
		if (Container.WorldsUsingThisRuntime.Contains(&InWorld))
		{
			OldRuntimeID = Container.Runtime->GetAtomRuntimeID();
		}
	}

	if (OldRuntimeID == InRuntimeID)
	{
		return;
	}

	if (FAtomRuntimeContainer* Container = Runtimes.Find(InRuntimeID))
	{
		FAtomRuntimeParams Params = FAtomRuntimeParams();
		Params.AssociatedWorld = &InWorld;
		FAtomRuntimeHandle Handle = BuildNewHandle(*Container, InRuntimeID, Params);
		//InWorld.SetAudioDevice(Handle);
		// ->
		RegisterWorld(&InWorld, InRuntimeID);
	}
	else
	{
		//static const FAudioDeviceHandle InvalidHandle;
		//InWorld.SetAudioDevice(InvalidHandle);
	}

	// unregister previous 
	if (OldRuntimeID != INDEX_NONE)
	{
		UnregisterWorld(&InWorld, OldRuntimeID);
	}
}

void FAtomRuntimeManager::SetActiveAtomRuntime(FAtomRuntimeId InRuntimeID)
{
	// Only change the active device if there are no solo'd audio devices
	//if (SoloDeviceHandle == INDEX_NONE)
	//{
		FAtomRuntime* RuntimeToActivate = nullptr;
		TArray<FAtomRuntime*> RuntimesToDeactivate;

		{
			FScopeLock ScopeLock(&RuntimeMapCriticalSection);

			// Iterate over all of our runtimes and disable (finalize) every Atom except for InRuntimeID:
			for (auto& RuntimeContainer : Runtimes)
			{
				check(RuntimeContainer.Value.Runtime);
				FAtomRuntime* Runtime = RuntimeContainer.Value.Runtime;

				if (RuntimeContainer.Key == InRuntimeID)
				{
					ActiveAtomRuntimeID = InRuntimeID;
					RuntimeToActivate = Runtime;
				}
				else
				{
					RuntimesToDeactivate.Add(Runtime);
				}
			}
		}

		if (RuntimeToActivate)
		{
			for (auto Runtime : RuntimesToDeactivate)
			{
				Runtime->Deactivate();
				//Runtime->SetDeviceMuted(true);
			}

			RuntimeToActivate->Activate();
			//Runtime->SetDeviceMuted(false);
		}

	//}
}

bool FAtomRuntimeManager::InitializeManager()
{

	// Platform specific module - Atom interface
	auto PlatformAtom = FCriWareCore::GetPlatform().Atom();

	check(PlatformAtom);

	if (PlatformAtom)
	// load and check if atom sub module is present
	//if (LoadDefaultAtomModule())
	{
		AtomRuntimeModule = PlatformAtom;

		//check(AtomModule);

		//const bool bIsAudioMixerEnabled = AudioDeviceModule->IsAudioMixerModule();
		//GetMutableDefault<UAudioSettings>()->SetAudioMixerEnabled(bIsAudioMixerEnabled);

#if WITH_EDITOR
		/*if (bIsAudioMixerEnabled)
		{
			IAudioEditorModule* AudioEditorModule = &FModuleManager::LoadModuleChecked<IAudioEditorModule>("AudioEditor");
			AudioEditorModule->RegisterAudioMixerAssetActions();
			AudioEditorModule->RegisterEffectPresetAssetActions();
		}*/
#endif

		FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddRaw(this, &FAtomRuntimeManager::AppWillEnterBackground);

#if WITH_ENGINE
		// register world delegate
		OnPreWorldInitializationHandle = FWorldDelegates::OnPreWorldInitialization.AddRaw(this, &FAtomRuntimeManager::OnWorldInitialization);
		OnPostWorldCleanupHandle = FWorldDelegates::OnPostWorldCleanup.AddRaw(this, &FAtomRuntimeManager::OnPostWorldCleanup);
		OnWorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddRaw(this, &FAtomRuntimeManager::OnWorldBeginTearDown);
		OnPreWorldFinishDestroyHandle = FWorldDelegates::OnPreWorldFinishDestroy.AddRaw(this, &FAtomRuntimeManager::OnPreWorldFinishDestroy);
#endif
#if WITH_EDITOR
		OnEditorMapChangeHandle = FEditorDelegates::MapChange.AddRaw(this, &FAtomRuntimeManager::OnEditorMapChange);
#endif

		// register default modulation parameters set in config
		if (UCriWareCoreSettings* CriWareSettings = GetMutableDefault<UCriWareCoreSettings>())
		{
			CriWareSettings->LoadDefaultObjects();
			CriWareSettings->RegisterModulationParameters();
		}

		return true;
	}

	// Failed to initialize
	return false;
}

bool FAtomRuntimeManager::Initialize()
{
	if (!Singleton)
	{
		bool bUseSound = FApp::CanEverRenderAudio();

		if (bUseSound)
		{
			UE_LOG(LogCriWareAtom, Display, TEXT("Initializing Atom Manager..."));
			Singleton = new FAtomRuntimeManager();

			if (Singleton->InitializeManager())
			{
				UE_LOG(LogCriWareAtom, Display, TEXT("Atom Manager Initialized"));
			}
			else
			{
				UE_LOG(LogCriWareAtom, Warning, TEXT("Atom Manager Initialization Failed!"));
				delete Singleton;
				Singleton = nullptr;
			}
		}
	}

	return Singleton != nullptr;
}

FAtomRuntimeManager* FAtomRuntimeManager::Get()
{
	return Singleton;
}

void FAtomRuntimeManager::Shutdown()
{
	if (Singleton)
	{
		delete Singleton;
		Singleton = nullptr;

		UE_LOG(LogCriWareAtom, Display, TEXT("Atom Manager Shutdown"));
	}
}

FAtomRuntimeHandle FAtomRuntimeManager::GetActiveAtomRuntime()
{
	if (ActiveAtomRuntimeID != INDEX_NONE)
	{
		return GetAtomRuntime(ActiveAtomRuntimeID);
	}

	return MainAtomRuntimeHandle;
}

bool FAtomRuntimeManager::CreateMainAtomRuntime()
{
	if (!MainAtomRuntimeHandle)
	{
		UWorld* World = GWorld;
#if WITH_EDITOR
		if (GEditor)
		{
			World = GEditor->GetEditorWorldContext().World();
		}
#endif

		// Initialize the main audio device.
		FAtomRuntimeParams MainAtomParams;
		MainAtomParams.Scope = EAtomRuntimeScope::Shared;
		MainAtomParams.bIsNonRealtime = false;
		MainAtomParams.AssociatedWorld = World;

		MainAtomRuntimeHandle = RequestAtomRuntime(MainAtomParams);

		if (!MainAtomRuntimeHandle)
		{
			UE_LOG(LogCriWareAtom, Display, TEXT("Main Atom runtime could not be initialized. Please check the value for AtomModuleName and AtomMixerModuleName in [Platform]Engine.ini."));
			return false;
		}

		// make world unique
		if (World)
		{
			SetAtomRuntimeWorld(*World, MainAtomRuntimeHandle.GetRuntimeID());
		}

		// Activate the main Atom runtime
		SetActiveAtomRuntime(MainAtomRuntimeHandle.GetRuntimeID());

		// use the thread from now.
		FAtomThread::StartAtomThread();
	}

	return true;
}

uint8 FAtomRuntimeManager::GetNumAtomRuntimes() const
{
	FScopeLock ScopeLock(&RuntimeMapCriticalSection);
	return Runtimes.Num();
}

uint8 FAtomRuntimeManager::GetNumMainAtomRuntimeWorlds() const
{
	FScopeLock ScopeLock(&RuntimeMapCriticalSection);

	const FAtomRuntimeId MainDeviceID = MainAtomRuntimeHandle.GetRuntimeID();
	if (Runtimes.Contains(MainDeviceID))
	{
		return Runtimes[MainDeviceID].WorldsUsingThisRuntime.Num();
	}
	else
	{
		return 0;
	}
}

TArray<FAtomRuntime*> FAtomRuntimeManager::GetAtomRuntimes() const
{
	TArray<FAtomRuntime*> RuntimesList;

	{
		FScopeLock ScopeLock(&RuntimeMapCriticalSection);
		Algo::Transform(Runtimes, RuntimesList, [](const TPair<FAtomRuntimeId, FAtomRuntimeContainer>& Pair)
		{
			return Pair.Value.Runtime;
		});
	}

	return RuntimesList;
}

TArray<UWorld*> FAtomRuntimeManager::GetWorldsUsingAtomRuntime(const FAtomRuntimeId& InAtomID)
{
	FScopeLock ScopeLock(&RuntimeMapCriticalSection);
	if (Runtimes.Contains(InAtomID))
	{
		return Runtimes[InAtomID].WorldsUsingThisRuntime;
	}
	else
	{
		return TArray<UWorld*>();
	}
}

void FAtomRuntimeManager::UpdateAtomRuntimes(bool bGameTicking)
{
	// Before we kick off the next update make sure that we've finished the previous frame's update (this should be extremely rare)
	if (GCVarEnableAtomThreadWait)
	{
		SyncFence.Wait();
	}

	IterateOverAllRuntimes([&bGameTicking](FAtomRuntimeId, FAtomRuntime* InRuntime)
	{
		InRuntime->Update(bGameTicking);
	});

	ResourceManager->UpdateAtomResources(bGameTicking);

	if (GCVarEnableAtomThreadWait)
	{
		SyncFence.BeginFence();
	}
}

void FAtomRuntimeManager::IterateOverAllRuntimes(TUniqueFunction<void(FAtomRuntimeId, FAtomRuntime*)> ForEachRuntime)
{
	TArray<FAtomRuntimeId> RuntimeIDs;
	{
		FScopeLock ScopeLock(&RuntimeMapCriticalSection);
		Runtimes.GetKeys(RuntimeIDs);
	}

	for (const FAtomRuntimeId RuntimeID : RuntimeIDs)
	{
		FAtomRuntimeHandle RuntimeHandle = GetAtomRuntime(RuntimeID);
		if (RuntimeHandle.IsValid())
		{
			ForEachRuntime(RuntimeID, RuntimeHandle.GetAtomRuntime());
		}
	}
}

void FAtomRuntimeManager::IterateOverAllRuntimes(TUniqueFunction<void(FAtomRuntimeId, const FAtomRuntime*)> ForEachRuntime) const
{
	TArray<FAtomRuntimeId> RuntimeIDs;
	{
		FScopeLock ScopeLock(&RuntimeMapCriticalSection);
		Runtimes.GetKeys(RuntimeIDs);
	}

	for (const FAtomRuntimeId RuntimeID : RuntimeIDs)
	{
		if (const FAtomRuntime* Runtime = GetAtomRuntimeRaw(RuntimeID))
		{
			ForEachRuntime(RuntimeID, Runtime);
		}
	}
}

#if WITH_EDITOR 
void FAtomRuntimeManager::IterateOverAllRuntimesForStatistics(TUniqueFunction<void(FAtomRuntimeId, FAtomRuntime*, int32 NumberOfHandles, EAtomRuntimeScope)> ForEachRuntime)
{
	struct FRuntimeInfo
	{
		int32 NumberOfHandles;
		EAtomRuntimeScope Scope;
	};

	TSortedMap<FAtomRuntimeId, FRuntimeInfo> TempRuntimeInfos;
	{
		FScopeLock ScopeLock(&RuntimeMapCriticalSection);
		for (auto& Runtime : Runtimes)
		{
			TempRuntimeInfos.Add(Runtime.Key, { Runtime.Value.NumberOfHandlesToThisRuntime, Runtime.Value.Scope });
		}
	}
	for (auto& Pair : TempRuntimeInfos)
	{
		FAtomRuntimeHandle RuntimeHandle = GetAtomRuntime(Pair.Key);
		if (RuntimeHandle.IsValid())
		{
			ForEachRuntime(Pair.Key, RuntimeHandle.GetAtomRuntime(), Pair.Value.NumberOfHandles, Pair.Value.Scope);
		}
	}
}
#endif

FAtomResourceManager& FAtomRuntimeManager::GetAtomResourceManager() const
{
	check(ResourceManager);
	return *ResourceManager;
}

void FAtomRuntimeManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	IterateOverAllRuntimes(
		[&Collector](FAtomRuntimeId, FAtomRuntime* InRuntime)
		{
			InRuntime->AddReferencedObjects(Collector);
		}
	);
}

void FAtomRuntimeManager::StopSoundsUsingResource(IAtomSoundResource* InSound, TArray<UAtomComponent*>* StoppedComponents)
{
	IterateOverAllRuntimes(
		[&InSound, &StoppedComponents](FAtomRuntimeId, FAtomRuntime* InRuntime)
		{
			InRuntime->StopSoundsUsingResource(InSound, StoppedComponents);
		}
	);
}

void FAtomRuntimeManager::FreeResource(IAtomSoundResource* InSound)
{
	// unload any resource linked to this sound
	ResourceManager->IterateOverAllResources([SoundResource = InSound](FAtomResourceId Id, FAtomResource* Resource)
	{
		TArray<IAtomSoundResource*> SoundResources;
		SoundResources.Add(Resource->GetSoundResource());
		Resource->IterateOverSoundResourceDependencies([&SoundResources](IAtomSoundResource* Dependency)
		{
			SoundResources.Add(Dependency);
		});

		if (SoundResources.Contains(SoundResource))
		{
			Resource->Unload();
		}
	});

	// ask sound to remove data (teardown)
	InSound->RemoveAtomResource();
}

void FAtomRuntimeManager::RegisterSoundClass(UAtomSoundClass* SoundClass)
{
	IterateOverAllRuntimes([&SoundClass](FAtomRuntimeId, FAtomRuntime* InRuntime)
	{
		InRuntime->RegisterSoundClass(SoundClass);
	});
}

void FAtomRuntimeManager::UnregisterSoundClass(UAtomSoundClass* SoundClass)
{
	IterateOverAllRuntimes([&SoundClass](FAtomRuntimeId, FAtomRuntime* InRuntime)
	{
		InRuntime->UnregisterSoundClass(SoundClass);
	});
}

void FAtomRuntimeManager::InitSoundClasses()
{
	IterateOverAllRuntimes([](FAtomRuntimeId, FAtomRuntime* InRuntime)
	{
		InRuntime->InitSoundClasses();
	});
}

void FAtomRuntimeManager::AppWillEnterBackground()
{
	SCOPED_ENTER_BACKGROUND_EVENT(STAT_FAtomRuntimeManager_AppWillEnterBackground);

	// Flush all commands to the Atom thread and the Atom server thread:
	//if (GCVarFlushAtomAudioRenderCommandsOnSuspend)
	//{
		if (MainAtomRuntimeHandle.IsValid())
		{
			FAtomThread::RunCommandOnAtomThread([this]()
			{
				if (MainAtomRuntimeHandle.IsValid())
				{
					MainAtomRuntimeHandle->FlushAudioRenderingCommands(true);
				}
			}, TStatId());
		}

		FAtomCommandFence AtomCommandFence;
		AtomCommandFence.BeginFence();
		AtomCommandFence.Wait();
	//}
}

void FAtomRuntimeManager::OnWorldInitialization(UWorld* InWorld, const UWorld::InitializationValues IVS)
{
	if (InWorld && IVS.bAllowAudioPlayback && MainAtomRuntimeHandle.IsValid())
	{
		RegisterWorld(InWorld, MainAtomRuntimeHandle.GetRuntimeID());
	}
}

void FAtomRuntimeManager::OnPostWorldCleanup(UWorld* InWorld, bool bSessionEnded, bool bCleanupResources)
{
	if (FAtomRuntime* AtomRuntime = GetAtomRuntimeRawFromWorld(InWorld))
	{
		AtomRuntime->Flush(InWorld);
		AtomRuntime->SetTransientPrimaryVolume(1.0f);
	}

	//OnWorldBeginTearDown(InWorld);
}

void FAtomRuntimeManager::OnWorldBeginTearDown(UWorld* InWorld)
{
	/*for (auto& AtomRuntime : Runtimes)
	{
		auto& Container = AtomRuntime.Value;

		if (Container.WorldsUsingThisRuntime.Contains(InWorld))
		{
			check(Container.Runtime);

			Container.Runtime->Flush(InWorld);
		}
	}*/
}

void FAtomRuntimeManager::OnPreWorldFinishDestroy(UWorld* InWorld)
{
	FAtomRuntimeId RuntimeID = INDEX_NONE;

	for (auto& AtomRuntime : Runtimes)
	{
		auto& Container = AtomRuntime.Value;

		if (Container.WorldsUsingThisRuntime.Contains(InWorld))
		{
			check(Container.Runtime);

			RuntimeID = Container.Runtime->GetAtomRuntimeID();
		}
	}

	if (RuntimeID != INDEX_NONE)
	{
		UnregisterWorld(InWorld, RuntimeID);
	}
}

#if WITH_EDITOR
void FAtomRuntimeManager::OnEditorMapChange(uint32)
{
	if (UWorld* World = GEditor->GetEditorWorldContext().World())
	{
		SetAtomRuntimeWorld(*World, MainAtomRuntimeHandle.GetRuntimeID());
	}
}
#endif

#if ENABLE_ATOM_DEBUG
Atom::FAtomDebugger& FAtomRuntimeManager::GetDebugger()
{
	check(AtomDebugger.IsValid());

	return *AtomDebugger;
}

const Atom::FAtomDebugger& FAtomRuntimeManager::GetDebugger() const
{
	check(AtomDebugger.IsValid());

	return *AtomDebugger;
}

#endif // ENABLE_ATOM_DEBUG

#if ENABLE_ATOM_MONITOR
FAtomMonitor& FAtomRuntimeManager::GetAtomMonitor()
{
	check(AtomMonitor.IsValid());

	return *AtomMonitor;
}

const FAtomMonitor& FAtomRuntimeManager::GetAtomMonitor() const
{
	check(AtomMonitor.IsValid());

	return *AtomMonitor;
}
#endif // ENABLE_ATOM_MONITOR

/*
 * FAtomRuntimeHandle
 ********************************************************/

FAtomRuntimeHandle::FAtomRuntimeHandle()
	: World(nullptr)
	, Runtime(nullptr)
	, RuntimeID(INDEX_NONE)
{
/*#if INSTRUMENT_AUDIODEVICE_HANDLES
	StackWalkID = INDEX_NONE;
#endif*/
}

FAtomRuntimeHandle::FAtomRuntimeHandle(FAtomRuntime* InAtom, FAtomRuntimeId InID, UWorld* InWorld)
	: World(InWorld)
	, Runtime(InAtom)
	, RuntimeID(InID)
{
/*#if INSTRUMENT_AUDIODEVICE_HANDLES
	AddStackDumpToAudioDeviceContainer();
#endif*/
}

FAtomRuntimeHandle::FAtomRuntimeHandle(const FAtomRuntimeHandle& Other)
	: FAtomRuntimeHandle()
{
	*this = Other;
}

FAtomRuntimeHandle::FAtomRuntimeHandle(FAtomRuntimeHandle&& Other)
	: FAtomRuntimeHandle()
{
	*this = MoveTemp(Other);
}

/*#if INSTRUMENT_AUDIODEVICE_HANDLES
void FAtomRuntimeHandle::AddStackDumpToAudioDeviceContainer()
{
	static const int32 MaxPlatformWalkStringCount = 1024 * 4;

	ANSICHAR PlatformDump[MaxPlatformWalkStringCount];
	FMemory::Memzero(PlatformDump, MaxPlatformWalkStringCount * sizeof(ANSICHAR));

	FPlatformStackWalk::StackWalkAndDump(PlatformDump, MaxPlatformWalkStringCount - 1, 2);

	FString FormattedDump = TEXT("New Handle Created:\n");

	int32 DumpLength = FCStringAnsi::Strlen(PlatformDump);

	// If this hits, increase the max character length.
	ensure(DumpLength < MaxPlatformWalkStringCount - 1);

	FormattedDump.AppendChars(ANSI_TO_TCHAR(PlatformDump), DumpLength);
	FormattedDump += TEXT("\n");
	StackWalkID = FAudioDeviceManager::Get()->CreateUniqueStackWalkID();
	FAudioDeviceManager::Get()->AddStackWalkForContainer(DeviceId, StackWalkID, MoveTemp(FormattedDump));
}
#endif*/

FAtomRuntimeHandle::~FAtomRuntimeHandle()
{
	if (IsValid())
	{
		FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get();
		if (ensure(RuntimeManager))
		{
			RuntimeManager->DecrementRuntime(RuntimeID, World.Get());

/*#if INSTRUMENT_AUDIODEVICE_HANDLES
			check(StackWalkID != INDEX_NONE);
			AudioDeviceManager->RemoveStackWalkForContainer(DeviceId, StackWalkID);
#endif*/
		}
	}
}

FAtomRuntime* FAtomRuntimeHandle::GetAtomRuntime() const
{
	return Runtime;
}

TWeakObjectPtr<UWorld> FAtomRuntimeHandle::GetWorld() const
{
	return World;
}

FAtomRuntimeId FAtomRuntimeHandle::GetRuntimeID() const
{
	return RuntimeID;
}

bool FAtomRuntimeHandle::IsValid() const
{
	return Runtime != nullptr;
}

void FAtomRuntimeHandle::Reset()
{
	*this = FAtomRuntimeHandle();
}

FAtomRuntimeHandle& FAtomRuntimeHandle::operator=(const FAtomRuntimeHandle& Other)
{
	const bool bWasValid = IsValid();
	const FAtomRuntimeId OldRuntimeID = RuntimeID;
	UWorld* OldWorld = World.Get();

/*#if INSTRUMENT_AUDIODEVICE_HANDLES
	const uint32 OldStackWalkID = StackWalkID;
#endif*/

	Runtime = Other.Runtime;
	RuntimeID = Other.RuntimeID;
	World = Other.World;

	if (FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get())
	{
		if (IsValid())
		{
			RuntimeManager->IncrementRuntime(RuntimeID);

/*#if INSTRUMENT_AUDIODEVICE_HANDLES
			AddStackDumpToAudioDeviceContainer();
#endif*/
		}

		if (bWasValid)
		{
			RuntimeManager->DecrementRuntime(OldRuntimeID, OldWorld);

/*#if INSTRUMENT_AUDIODEVICE_HANDLES
			check(OldStackWalkID != INDEX_NONE);
			AudioDeviceManager->RemoveStackWalkForContainer(OldDeviceId, OldStackWalkID);
#endif*/
		}
	}

	return *this;
}

FAtomRuntimeHandle& FAtomRuntimeHandle::operator=(FAtomRuntimeHandle&& Other)
{
/*#if INSTRUMENT_AUDIODEVICE_HANDLES
	const uint32 OldStackWalkID = StackWalkID;
#endif*/

	const bool bWasValid = IsValid();
	const FAtomRuntimeId OldRuntimeID = RuntimeID;
	UWorld* OldWorld = World.Get();

	Runtime = Other.Runtime;
	RuntimeID = Other.RuntimeID;
	World = Other.World;

	FAtomRuntimeManager* RuntimeManager = FAtomRuntimeManager::Get();
	if (RuntimeManager && IsValid())
	{
/*#if INSTRUMENT_AUDIODEVICE_HANDLES
		AddStackDumpToAudioDeviceContainer();
#endif*/
	}

	if (RuntimeManager && bWasValid)
	{
/*#if INSTRUMENT_AUDIODEVICE_HANDLES
		check(OldStackWalkID != INDEX_NONE);
		AudioDeviceManager->RemoveStackWalkForContainer(OldDeviceId, OldStackWalkID);
#endif*/

		RuntimeManager->DecrementRuntime(OldRuntimeID, OldWorld);
	}

	if (RuntimeManager && Other.IsValid())
	{
/*#if INSTRUMENT_AUDIODEVICE_HANDLES
		check(Other.StackWalkID != INDEX_NONE);
		AudioDeviceManager->RemoveStackWalkForContainer(Other.DeviceId, Other.StackWalkID);
#endif*/
	}

	Other.Runtime = nullptr;
	Other.RuntimeID = INDEX_NONE;
	Other.World.Reset();

/*#if INSTRUMENT_AUDIODEVICE_HANDLES
	Other.StackWalkID = INDEX_NONE;
#endif*/

	return *this;
}

/*
 * FAtomRuntimeContainer
 ********************************************************/

FAtomRuntimeManager::FAtomRuntimeContainer::FAtomRuntimeContainer(const FAtomRuntimeParams& InParams, FAtomRuntimeId InAtomID, FAtomRuntimeManager* RuntimeManager)
	: NumberOfHandlesToThisRuntime(0)
	, Scope(InParams.Scope)
	, bIsNonRealtime(InParams.bIsNonRealtime)
	, SpecifiedModule(InParams.AtomModule)
{
	// Here we create an entirely new audio device.
	if (bIsNonRealtime)
	{
		//IAudioDeviceModule* NonRealtimeModule = FModuleManager::LoadModulePtr<IAudioDeviceModule>(TEXT("NonRealtimeAudioRenderer"));
		//check(NonRealtimeModule);
		//Device = NonRealtimeModule->CreateAudioDevice();
	}
	/*else if (SpecifiedModule != nullptr)
	{
		//Atom = SpecifiedModule->CreateAudioDevice();
	}
	else
	{
		check(RuntimeManager->AtomModule);
		Atom = RuntimeManager->AtomModule->CreateAtom();

		if (!Atom)
		{
			//Atom = new Audio::FMixerDevice(DeviceManager->AudioDeviceModule->CreateAudioMixerPlatformInterface());
		}
	}*/

	const UCriWareCoreSettings* CriWareSettings = GetDefault<UCriWareCoreSettings>();

	check(RuntimeManager->AtomRuntimeModule);

	// CreateAtom
	if (CriWareSettings->bUseUnrealSoundRenderer)
	{
		Runtime = new FAtomRuntimeUnrealAudio(RuntimeManager->AtomRuntimeModule);
	}
	else
	{
		Runtime = RuntimeManager->AtomRuntimeModule->CreateAtomRuntime();
	}

	check(Runtime);

	// Set to highest max channels initially provided by any quality setting, so that
	// setting to lower quality but potentially returning to higher quality later at
	// runtime is supported.
	
	//const int32 HighestMaxChannels = CriWarweSettings ? CriWarweSettings->Get() : 0;
	if (Runtime->Init(InAtomID/*, HighestMaxChannels*/))
	{
		//const FAtomQualitySettings& QualitySettings = Atom->GetQualityLevelSettings();
		//Atom->SetMaxChannels(QualitySettings.MaxChannels);

		//Atom->FadeIn();
	}
	else
	{
		UE_LOG(LogCriWareAtom, Warning, TEXT("FAtom::Init Failed!"));
		Runtime->Teardown();
		delete Runtime;
		Runtime = nullptr;
	}
}

FAtomRuntimeManager::FAtomRuntimeContainer::FAtomRuntimeContainer()
{
	checkNoEntry();
}

FAtomRuntimeManager::FAtomRuntimeContainer::FAtomRuntimeContainer(FAtomRuntimeContainer&& Other)
{
	Runtime = Other.Runtime;
	Other.Runtime = nullptr;

	NumberOfHandlesToThisRuntime = Other.NumberOfHandlesToThisRuntime;
	Other.NumberOfHandlesToThisRuntime = 0;

	WorldsUsingThisRuntime = MoveTemp(Other.WorldsUsingThisRuntime);

	Scope = Other.Scope;
	Other.Scope = EAtomRuntimeScope::Default;

	bIsNonRealtime = Other.bIsNonRealtime;
	Other.bIsNonRealtime = false;

	SpecifiedModule = Other.SpecifiedModule;
	Other.SpecifiedModule = nullptr;

/*#if INSTRUMENT_AUDIODEVICE_HANDLES
	HandleCreationStackWalks = MoveTemp(Other.HandleCreationStackWalks);
#endif*/
}

FAtomRuntimeManager::FAtomRuntimeContainer::~FAtomRuntimeContainer()
{
	// Shutdown the audio device.
	if (NumberOfHandlesToThisRuntime != 0)
	{
		UE_LOG(LogCriWareAtom, Display, TEXT("Shutting down Atom runtime while %d references to it are still alive. For more information, compile with INSTRUMENT_AUDIODEVICE_HANDLES."), NumberOfHandlesToThisRuntime);

/*#if INSTRUMENT_AUDIODEVICE_HANDLES
		FString ActiveAtomHandles;
		for (auto& StackWalkString : HandleCreationStackWalks)
		{
			ActiveAtomHandles += StackWalkString.Value;
			ActiveAtomHandles += TEXT("\n\n");
		}

		UE_LOG(LogAudio, Warning, TEXT("List Of Active Handles: \n%s"), *ActiveAtomHandles);
#endif*/
	}

	if (Runtime)
	{
		//Atom->FadeOut();
		Runtime->Teardown();
		delete Runtime;
		Runtime = nullptr;
	}
}

FAtomRuntimeManagerDelegates::FOnAtomRuntimeCreated FAtomRuntimeManagerDelegates::OnAtomRuntimeCreated;
FAtomRuntimeManagerDelegates::FOnAtomRuntimeDestroyed FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed;
FAtomRuntimeManagerDelegates::FOnAtomRuntimeActivated FAtomRuntimeManagerDelegates::OnAtomRuntimeWillActivate;
FAtomRuntimeManagerDelegates::FOnAtomRuntimeActivated FAtomRuntimeManagerDelegates::OnAtomRuntimeActivated;
FAtomRuntimeManagerDelegates::FOnAtomRuntimeDeactivated FAtomRuntimeManagerDelegates::OnAtomRuntimeWillDeactivate;
FAtomRuntimeManagerDelegates::FOnAtomRuntimeDeactivated FAtomRuntimeManagerDelegates::OnAtomRuntimeDeactivated;

FAtomRuntimeWorldDelegates::FOnWorldRegisteredToAtomRuntime FAtomRuntimeWorldDelegates::OnWorldRegisteredToAtomRuntime;
FAtomRuntimeWorldDelegates::FOnWorldUnregisteredWithAtomRuntime FAtomRuntimeWorldDelegates::OnWorldUnregisteredWithAtomRuntime;
