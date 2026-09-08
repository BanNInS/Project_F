/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : CriWareCore.cpp
 *
 ****************************************************************************/

#include "CriWareCore.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "IXRTrackingSystem.h"
#include "UnrealEngine.h"
#include "Misc/App.h"
#include "Misc/ConfigContext.h"


#include "CriWareCoreModule.h"
#include "CriWareCorePrivate.h"
#include "CriWareCoreSettings.h"
#include "CriWareCoreSubsystem.h"
#include "CriWare.h"
#include "CriWareDefines.h"
#include "CriWareFileIo.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomDebug.h"

#if WITH_EDITOR
#include "Editor.h"
#include "CriWareEditor.h"
#include "Settings/LevelEditorMiscSettings.h"
#endif

/* FCriWareCore class
 *****************************************************************************/

FCriWarePlatform FCriWareCore::Platform = FCriWarePlatform();

FCriWareCore::FCriWareCore()
	: bIsInitialized(false)
{
	if (FApp::ShouldUseThreadingForPerformance() && FPlatformMisc::AllowAudioThread())
	{
		FString CriWareIni;
		FConfigContext::ReadIntoGConfig().Load(*GCriWareIni, CriWareIni);

		bool bUseThreadedAtom = false;
		if (!GIsEditor)
		{
			GConfig->GetBool(TEXT("Atom"), TEXT("bUseAtomThread"), bUseThreadedAtom, CriWareIni);
		}
		FAtomThread::SetUseThreadedAtom(bUseThreadedAtom);
	}

	FCoreDelegates::OnPostEngineInit.AddLambda([]()
	{
		UCriWareCoreSubsystem::InitializeCriWare();
	});

	FCoreDelegates::OnEnginePreExit.AddLambda([]()
	{
		UCriWareCoreSubsystem::DeinitializeCriWare();

		// Shutdown Atom Manager instance
		FAtomRuntimeManager::Shutdown();
	});
}

FCriWareCore::~FCriWareCore()
{
}

void FCriWareCore::InitalizeLibraries()
{
	if (bIsInitialized)
	{
		UE_LOG(LogCriWareCore, Warning, TEXT("InitializeLibraries called again!"));
		return; 
	}

#if PLATFORM_WINDOWS && defined(CRIWARE_UE_LE)
	if (!DllHandle)
	{
		FString DLLName = TEXT("cri_ware_pcx64_le.dll");
		FString PluginPath = FString::Printf(TEXT("Plugins/Runtime/CriWare/CriWare"));
		FString CorePath = FPaths::Combine(*(FPaths::ProjectDir()), PluginPath);

		if (!FPaths::DirectoryExists(CorePath))
		{
			CorePath = FPaths::Combine(*(FPaths::EngineDir()), PluginPath);
		}

		FString RootDllPath = CorePath / TEXT("Source/CriWare/SDK/pc/libs/x64");

		FPlatformProcess::PushDllDirectory(*RootDllPath);
		DllHandle = FPlatformProcess::GetDllHandle(*(RootDllPath / DLLName));
		FPlatformProcess::PopDllDirectory(*RootDllPath);

		if (!DllHandle)
		{
			UE_LOG(LogCriWareCore, Error, TEXT("Failed to load CriWare Plugin DLL."));
			return;
		}
	}
#endif

	// error - temporary 
	// TODO: -> will use error manager
	FCriWareApi::criErr_SetCallback([](const CriChar8* errid, CriUint32 p1, CriUint32 p2, CriUint32* /* parray */)
	{
		// Display an error message
		const CriChar8* errmsg = FCriWareApi::criErr_ConvertIdToMessage(errid, p1, p2);

		switch (errmsg[0])
		{
		case 'W':
			UE_LOG(LogCriWareCore, Warning, TEXT("%s"), UTF8_TO_TCHAR(errmsg));
			break;
		default:
			UE_LOG(LogCriWareCore, Error, TEXT("%s"), UTF8_TO_TCHAR(errmsg));
			break;
		}
	});

	// ensure settings loaded
	GetMutableDefault<UCriWareCoreSettings>()->LoadConfig();

	// Platform specific module - Atom interface
	auto PlatformAtom = GetPlatform().Atom();

	if (PlatformAtom)
	{
		// Initialize the Atom manager and main Atom runtime (after a world context is setup)
		InitializeAtomRuntimeManager();

		// Register events handling.
		// Frame events
		FCoreDelegates::OnEndFrame.AddRaw(this, &FCriWareCore::HandleOnEndFrame);
		// Application events
		FCoreDelegates::ApplicationWillDeactivateDelegate.AddRaw(this, &FCriWareCore::HandleApplicationWillDeactivate);
		FCoreDelegates::ApplicationHasReactivatedDelegate.AddRaw(this, &FCriWareCore::HandleApplicationHasReactivated);

		bIsInitialized = true;
	}
	else
	{
		// Atom is not supported on this platform
	}
}

void FCriWareCore::UninitializeLibraries()
{
	if (bIsInitialized) 
	{
		if (AtomRuntimeManager)
		{
			// Finalize Atom and FileSystem Libraries. 
			ReleaseAtomRuntimeManager();
		}

		// Unregister events handling.
		// Application Events
		FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveAll(this);
		FCoreDelegates::ApplicationHasReactivatedDelegate.RemoveAll(this);
		// Frames Events
		FCoreDelegates::OnEndFrame.RemoveAll(this);

		bIsInitialized = false;
	}
}

void FCriWareCore::SetupFileSystem(FCriWareAllocator& FsAllocator)
{
	if (FCriWareApi::criFs_IsInitialized() == CRI_FALSE)
	{
		// Initialize FileSystem settings
		// Allocators
		FCriWareApi::criFs_SetUserMallocFunction(static_cast<CriFsMallocFunc>(FCriWareAllocator_Native::AllocateMemory), &FsAllocator);
		FCriWareApi::criFs_SetUserFreeFunction(static_cast<CriFsFreeFunc>(FCriWareAllocator_Native::FreeMemory), &FsAllocator);
		// Settings
		FCriWareApi::criFs_ControlFileIoMode(CRIFS_FILE_IO_MODE_OPEN_EVERY_TIME);
		//FCriWareApi::criFs_SetFileAccessThreadStackSize(1024 * 1024);
		// Redirection of FileSystem I/O to I/O Dispatcher.
		FCriWareApi::criFs_SetSelectIoCallback(CriWareFileIo::SelectIo);
	}
}

FAtomRuntimeManager* FCriWareCore::GetAtomRuntimeManager() const
{
	return AtomRuntimeManager;
}

uint32 FCriWareCore::GetMainAtomRuntimeID() const
{
	return MainAtomRuntimeHandle.GetRuntimeID();
}

FAtomRuntimeHandle FCriWareCore::GetMainAtomRuntime() const
{
	if (AtomRuntimeManager)
	{
		return AtomRuntimeManager->GetMainAtomRuntimeHandle();
	}

	return FAtomRuntimeHandle();
}

FAtomRuntime* FCriWareCore::GetMainAtomRuntimeRaw() const
{
	if (AtomRuntimeManager && MainAtomRuntimeHandle.IsValid())
	{
		return AtomRuntimeManager->GetMainAtomRuntimeHandle().GetAtomRuntime();
	}

	return nullptr;
}

FAtomRuntimeHandle FCriWareCore::GetActiveAtomRuntime() const
{
	if (AtomRuntimeManager)
	{
		return AtomRuntimeManager->GetActiveAtomRuntime();
	}

	return FAtomRuntimeHandle();
}

void FCriWareCore::InitializeAtomRuntimeManager()
{
	UE_LOG(LogCriWareCore, Verbose, TEXT("Creating Atom Manager"));

	FAtomRuntimeManager::Initialize();
	AtomRuntimeManager = FAtomRuntimeManager::Get();
	if (AtomRuntimeManager)
	{
		AtomRuntimeManager->CreateMainAtomRuntime();
		MainAtomRuntimeHandle = AtomRuntimeManager->GetMainAtomRuntimeHandle();
	}
}

void FCriWareCore::ReleaseAtomRuntimeManager()
{
	// Release the main atom device handle.
	MainAtomRuntimeHandle.Reset();
	AtomRuntimeManager = nullptr;
}

// Engine Tick after all other object tick
void FCriWareCore::HandleOnEndFrame()
{
	if (bIsInitialized && AtomRuntimeManager)
	{
#if WITH_EDITOR
		if (GEditor)
		{
			// True if a viewport has realtime audio	// If any realtime audio is enabled in the editor
			bool bAudioIsRealtime = GetDefault<ULevelEditorMiscSettings>()->bEnableRealTimeAudio;

			auto PlayWorld = GEditor->PlayWorld;

			if (PlayWorld)
			{
				UpdateAtomListeners(PlayWorld);
			}

			AtomRuntimeManager->UpdateAtomRuntimes((!PlayWorld && bAudioIsRealtime) || (PlayWorld && !PlayWorld->IsPaused()));
		}
		else if (GEngine)
		{
			// True if a viewport has realtime audio	// If any realtime audio is enabled in the editor
			bool bAudioIsRealtime = GetDefault<ULevelEditorMiscSettings>()->bEnableRealTimeAudio;

			bool bIsAnyWorldUnpaused = false;

			UWorld* EditorWorld = nullptr;
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				// Return the first PIE world that we can find
				if (Context.WorldType == EWorldType::PIE)
				{
					UWorld* ThisWorld = Context.World();
					if (ThisWorld)
					{
						EditorWorld = ThisWorld;
						if (bAudioIsRealtime || (EditorWorld && !EditorWorld->IsPaused()))
						{
							UpdateAtomListeners(EditorWorld);
							bIsAnyWorldUnpaused = true;
						}
						break;
					}
				}
				// Or else return a valid Editor world. For "Standalone mode" the world type is Game.
				// Note that this code won't run in an actual packaged build though, as its inside an #if WITH_EDITOR block
				else if (Context.WorldType == EWorldType::Editor || Context.WorldType == EWorldType::Game)
				{
					EditorWorld = Context.World();
					if (EditorWorld && !EditorWorld->IsPaused())
					{
						UpdateAtomListeners(EditorWorld);
						bIsAnyWorldUnpaused = true;
					}
				}
			}

			AtomRuntimeManager->UpdateAtomRuntimes(bIsAnyWorldUnpaused);
		}
#else
		if (GEngine)
		{
			bool bIsAnyNonPreviewWorldUnpaused = false;

			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (!Context.World() || !Context.World()->ShouldTick())
				{
					continue;
				}

				if (Context.WorldType != EWorldType::EditorPreview && !Context.World()->IsPaused())
				{
					bIsAnyNonPreviewWorldUnpaused = true;
				}

				UpdateAtomListeners(Context.World());
			}

			AtomRuntimeManager->UpdateAtomRuntimes(bIsAnyNonPreviewWorldUnpaused);
		}
#endif

#if ENABLE_ATOM_DEBUG
		Atom::FAtomDebugger::DrawDebugStatsToViewportStats();
#endif // ENABLE_ATOM_DEBUG
	}
}

void FCriWareCore::UpdateAtomListeners(UWorld* World)
{
	FAtomRuntimeHandle AtomRuntime;

#if WITH_EDITOR
	if (GEditor)
	{
		if (PIEAtomRuntimeHandle.IsValid())
		{
			AtomRuntime = PIEAtomRuntimeHandle;
		}
	}
	else
	{
		// for the standalone mode
		AtomRuntime = MainAtomRuntimeHandle;
	}
#else
	AtomRuntime = MainAtomRuntimeHandle;
#endif

	if (!GEngine || !World || !AtomRuntime.IsValid())
	{
		return;
	}

	int32 ViewportIndex = 0;
	for (FLocalPlayerIterator Iterator(GEngine, World); Iterator; ++Iterator)
	{
		ULocalPlayer* LocalPlayer = *Iterator;
		if (LocalPlayer)
		{
			APlayerController* PlayerController = LocalPlayer->PlayerController;
			if ( PlayerController)
			{
				bool bUpdateListenerPosition = true;

				// If there is more than one world referencing the main audio device
				/*if (AtomRuntimeManager->GetNumMainAtomWorlds() > 1)
				{
					FAtomRuntimeId MainAtomID = GetMainAtomID();
					if (AtomRuntime->GetAtomID() == MainAtomID && !bHasAudioFocus)
					{
						bUpdateListenerPosition = false;
					}
				}*/

				if (bUpdateListenerPosition)
				{
					FVector Location;
					FVector ProjFront;
					FVector ProjRight;
					PlayerController->GetAudioListenerPosition(/*out*/ Location, /*out*/ ProjFront, /*out*/ ProjRight);

					FTransform ListenerTransform(FRotationMatrix::MakeFromXY(ProjFront, ProjRight));

					// Allow the HMD to adjust based on the head position of the player, as opposed to the view location
					if (GEngine->XRSystem.IsValid() && GEngine->StereoRenderingDevice.IsValid() && GEngine->StereoRenderingDevice->IsStereoEnabled())
					{
						const FVector Offset = GEngine->XRSystem->GetAudioListenerOffset();
						Location += ListenerTransform.TransformPositionNoScale(Offset);
					}

					ListenerTransform.SetTranslation(Location);
					ListenerTransform.NormalizeRotation();

					bool bCameraCut = PlayerController->PlayerCameraManager->bGameCameraCutThisFrame;

					AtomRuntime->SetListener(World, ViewportIndex, ListenerTransform, (bCameraCut ? 0.0f : World->GetDeltaSeconds()));

					// AttenuationOverridePosition => Atom focus point
					FVector OverrideAttenuation;
					if (PlayerController->GetAudioListenerAttenuationOverridePosition(OverrideAttenuation))
					{
						AtomRuntime->SetListenerAttenuationOverride(ViewportIndex, OverrideAttenuation);
					}
					else
					{
						AtomRuntime->ClearListenerAttenuationOverride(ViewportIndex);
					}

					ViewportIndex++;
				}
			}
		}
	}
}

void FCriWareCore::HandleApplicationWillDeactivate()
{
	if (FAtomRuntimeHandle Runtime = GetActiveAtomRuntime())
	{
		Runtime->SuspendContext();
	}
}

void FCriWareCore::HandleApplicationHasReactivated()
{
	if (FAtomRuntimeHandle Runtime = GetActiveAtomRuntime())
	{
		Runtime->ResumeContext();
	}
}

#if WITH_EDITOR
void FCriWareCore::OnBeginPIE(bool bIsSimulating)
{
	if (GEditor)
	{
		if (AtomRuntimeManager)
		{
			auto EditorWorld = GEditor->GetEditorWorldContext().World();
			auto PlayWorld = EditorWorld;
			if (GEditor->GetPIEWorldContext())
			{
				PlayWorld = GEditor->GetPIEWorldContext()->World();
			}

			// Flush all audio sources from the editor world
			if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(EditorWorld))
			{
				AtomRuntime->Flush(EditorWorld);
				//AtomRuntime->ResetInterpolation(); // unused 
				AtomRuntime->OnBeginPIE(bIsSimulating);
			}

			FAtomRuntimeParams AtomParams = AtomRuntimeManager->GetDefaultParamsForNewWorld();
			AtomParams.AssociatedWorld = PlayWorld;

			PIEAtomRuntimeHandle = AtomRuntimeManager->RequestAtomRuntime(AtomParams);

			if (PIEAtomRuntimeHandle.IsValid())
			{
				FAtomRuntimeId PIEAtomID = PIEAtomRuntimeHandle.GetRuntimeID();

				// Atom.SetDefaultRack(Rack for this)
				AtomRuntimeManager->SetActiveAtomRuntime(PIEAtomID); // if ActiveAtom != NewAtom Atom.Deactivate() -> FinalaizeLIb   Atom.Activate() -> initalizelibaraies with the default rackset

				if (PlayWorld)
				{
					AtomRuntimeManager->SetAtomRuntimeWorld(*PlayWorld, PIEAtomID);
				}
			}

			PIEAtomRuntimeDestroyedHandle = FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.AddLambda([this](const FAtomRuntimeId InRuntimeID)
			{
				if (InRuntimeID == PIEAtomRuntimeHandle.GetRuntimeID())
				{
					PIEAtomRuntimeHandle.Reset();
				}
			});
		}
	}
}

void FCriWareCore::OnPrePIEEnded(bool bIsSimulating)
{
	if (AtomRuntimeManager)
	{
		// Shutdown all Atom runtime if we've requested end playmap now to avoid issues with GC running
		TArray<FAtomRuntime*> AtomRuntimes = AtomRuntimeManager->GetAtomRuntimes();
		for (FAtomRuntime* AtomRuntime : AtomRuntimes)
		{
			AtomRuntime->Flush(nullptr);
		}
	}
}

void FCriWareCore::OnEndPIE(bool bIsSimulating)
{
	if (GEditor)
	{
		if (AtomRuntimeManager)
		{
			auto EditorWorld = GEditor->GetEditorWorldContext().World();
			auto PlayWorld = EditorWorld;
			if (GEditor->GetPIEWorldContext())
			{
				PlayWorld = GEditor->GetPIEWorldContext()->World();
			}

			// Flush all audio sources from the pie world
			if (FAtomRuntime* AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(PlayWorld))
			{
				AtomRuntime->Flush(PlayWorld);
				//AtomRuntime->ResetInterpolation(); //unused
				AtomRuntime->OnEndPIE(bIsSimulating);
				AtomRuntime->SetTransientPrimaryVolume(1.0f);
				// Reset solo audio
				/*if (PlayInEditorSessionInfo.IsSet())
				{
					ULevelEditorPlaySettings* EditorPlaySettings = PlayInEditorSessionInfo->OriginalRequestParams.EditorPlaySettings;
					if (EditorPlaySettings && EditorPlaySettings->SoloAudioInFirstPIEClient && GEngine)
					{
						if (FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(PlayWorld))
						{
							if (WorldContext->bIsPrimaryPIEInstance)
							{
								if (FAudioDeviceManager* DeviceManager = AudioDevice->GetAudioDeviceManager())
								{
									DeviceManager->SetSoloDevice(INDEX_NONE);
								}
							}
						}
					}
				}*/
			}

			// switch to Editor world
			FAtomRuntimeId MainAtomID = GetMainAtomRuntimeID();
			
			AtomRuntimeManager->SetActiveAtomRuntime(MainAtomID);

			if (EditorWorld)
			{
				AtomRuntimeManager->SetAtomRuntimeWorld(*EditorWorld, MainAtomID);
			}

			if (PIEAtomRuntimeDestroyedHandle.IsValid())
			{
				FAtomRuntimeManagerDelegates::OnAtomRuntimeDestroyed.Remove(PIEAtomRuntimeDestroyedHandle);
				PIEAtomRuntimeDestroyedHandle.Reset();
			}

			PIEAtomRuntimeHandle.Reset();
		}
	}
}
#endif // WITH_EDITOR

/*
 * Globals
 *****************************************************************************/

// tasks pipe that arranges atom tasks execution one after another so no synchronisation between them is required
CRIWARECORE_API UE::Tasks::FPipe GAtomPipe{ TEXT("AtomPipe") };
// True if async atom processing is enabled and started
CRIWARECORE_API std::atomic<bool> GIsAtomThreadRunning{ false };

CRIWARECORE_API std::atomic<bool> GIsAtomThreadSuspended{ false };


CRIWARECORE_API bool IsAtomThreadRunning()
{
	return GIsAtomThreadRunning.load(std::memory_order_acquire) && !GIsAtomThreadSuspended.load(std::memory_order_acquire);
}

CRIWARECORE_API bool IsInAtomThread()
{
	return GIsAtomThreadRunning.load(std::memory_order_acquire) && !GIsAtomThreadSuspended.load(std::memory_order_acquire) ? GAtomPipe.IsInContext() : IsInGameThread();
}
