
#include "Atom/AtomGameplayStatics.h"

#include "CriWare.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntimeManager.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomSoundBank.h"
#include "Atom/AtomSoundClass.h"
#include "Atom/AtomComponent.h"
#include "Atom/Interfaces/IAtomActorAisacParameter.h"

bool UAtomGameplayStatics::AreAnyListenersWithinRange(const UObject* WorldContextObject, const FVector& Location, float MaximumRange)
{
	// If there is no valid world from the world context object then there certainly are no listeners
	if (auto AtomRuntimeRaw = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject))
	{
		if (GCriWare)
		{
			if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
			{
				if (FAtomRuntimeHandle AtomRuntime = RuntimeManager->GetAtomRuntime(AtomRuntimeRaw->GetAtomRuntimeID()))
				{
					return AtomRuntime->LocationIsAudible(Location, MaximumRange);
				}
			}
		}
	}

	return false;
}

bool UAtomGameplayStatics::GetClosestListenerLocation(const UObject* WorldContextObject, const FVector& Location, float MaximumRange, const bool bAllowAttenuationOverride, FVector& ListenerPosition)
{
	// If there is no valid world from the world context object then there certainly are no listeners
	auto AtomRuntimeRaw = FAtomRuntimeManager::GetAtomRuntimeFromWorldContext(WorldContextObject);
	if (!AtomRuntimeRaw)
	{
		return false;
	}

	FAtomRuntimeHandle AtomRuntime;
	if (GCriWare)
	{
		if (FAtomRuntimeManager* RuntimeManager = GCriWare->GetAtomRuntimeManager())
		{

			AtomRuntime = RuntimeManager->GetAtomRuntime(AtomRuntimeRaw->GetAtomRuntimeID());
		}
	}

	if (!AtomRuntime)
	{
		return false;
	}

	float OutDistSq;
	const int32 ClosestListenerIndex = AtomRuntime->FindClosestListenerIndex(Location, OutDistSq, bAllowAttenuationOverride);
	if (ClosestListenerIndex == INDEX_NONE || ((MaximumRange * MaximumRange) < OutDistSq))
	{
		return false;
	}

	return AtomRuntime->GetListenerPosition(ClosestListenerIndex, ListenerPosition, bAllowAttenuationOverride);
}

void UAtomGameplayStatics::SetGlobalPitchModulation(const UObject* WorldContextObject, float PitchModulation, float TimeSec)
{
	if (!GEngine)
	{
		return;
	}

	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!ThisWorld || !ThisWorld->bAllowAudioPlayback || ThisWorld->IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	if (FAtomRuntimeHandle AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorld(ThisWorld))
	{
		AtomRuntime->SetGlobalPitchModulation(PitchModulation, TimeSec);
	}
}

void UAtomGameplayStatics::PlaySound2D(const UObject* WorldContextObject, UAtomSoundBase* Sound, float VolumeMultiplier /* = 1.f */, float PitchMultiplier /* = 1.f */, float StartTime /* = 0.f */, UAtomConcurrency* ConcurrencySettings /* = nullptr */, const AActor* OwningActor /* = nullptr */, bool bIsUISound /* = true */)
{
	if (!Sound || !GEngine)
	{
		return;
	}

	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!ThisWorld || !ThisWorld->bAllowAudioPlayback || ThisWorld->IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	FAtomRuntimeHandle AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorld(ThisWorld);
	if (AtomRuntime)
	{
		FAtomActiveSound NewActiveSound;
		NewActiveSound.SetSound(Sound);
		NewActiveSound.SetWorld(ThisWorld);

		NewActiveSound.SetPitch(PitchMultiplier);
		NewActiveSound.SetVolume(VolumeMultiplier);

		NewActiveSound.RequestedStartTime = FMath::Max(0.f, StartTime);

		NewActiveSound.bIsUISound = bIsUISound;
		NewActiveSound.bAllowSpatialization = false;

		if (ConcurrencySettings)
		{
			NewActiveSound.ConcurrencySet.Add(ConcurrencySettings);
		}

		NewActiveSound.Priority = Sound->Priority;
		//NewActiveSound.SubtitlePriority = Sound->GetSubtitlePriority();

		// If OwningActor isn't supplied to this function, derive an owner from the WorldContextObject
		const AActor* WorldContextOwner = Cast<const AActor>(WorldContextObject);
		if (!WorldContextOwner)
		{
			WorldContextOwner = WorldContextObject->GetTypedOuter<AActor>();
		}
		const AActor* ActiveSoundOwner = OwningActor ? OwningActor : WorldContextOwner;

		NewActiveSound.SetOwner(ActiveSoundOwner);

		TArray<FAtomAisacParameter> Params;
		UAtomActorAisacParameter::Fill(ActiveSoundOwner, Params);

		AtomRuntime->AddNewActiveSound(NewActiveSound, &Params);
	}
}

UAtomComponent* UAtomGameplayStatics::SpawnSound2D(const UObject* WorldContextObject, UAtomSoundBase* Sound, float VolumeMultiplier /* = 1.f */, float PitchMultiplier /* = 1.f */, float StartTime /* = 0.f */, UAtomConcurrency* ConcurrencySettings /* = nullptr */, bool bPersistAcrossLevelTransition /* = false */, bool bAutoDestroy /* = true */)
{
	UAtomComponent* AtomComponent = CreateSound2D(WorldContextObject, Sound, VolumeMultiplier, PitchMultiplier, StartTime, ConcurrencySettings, bPersistAcrossLevelTransition, bAutoDestroy);
	if (AtomComponent)
	{
		AtomComponent->Play(StartTime);
	}
	return AtomComponent;
}

UAtomComponent* UAtomGameplayStatics::CreateSound2D(const UObject* WorldContextObject, UAtomSoundBase* Sound, float VolumeMultiplier /* = 1.f */, float PitchMultiplier /* = 1.f */, float StartTime /* = 0.f */, UAtomConcurrency* ConcurrencySettings /* = nullptr */, bool bPersistAcrossLevelTransition /* = false */, bool bAutoDestroy /* = true */)
{
	if (!Sound || !GEngine)
	{
		return nullptr;
	}

	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!ThisWorld || !ThisWorld->bAllowAudioPlayback || ThisWorld->IsNetMode(NM_DedicatedServer))
	{
		return nullptr;
	}

	// Derive an owner from the WorldContextObject
	AActor* WorldContextOwner = Cast<AActor>(const_cast<UObject*>(WorldContextObject));
	if (!WorldContextOwner)
	{
		WorldContextOwner = WorldContextObject->GetTypedOuter<AActor>();;
	}

	FAtomRuntime::FCreateComponentParams Params = bPersistAcrossLevelTransition
		? FAtomRuntime::FCreateComponentParams(FAtomRuntimeManager::GetAtomRuntimeRawFromWorld(ThisWorld))
		: FAtomRuntime::FCreateComponentParams(ThisWorld, WorldContextOwner);

	if (ConcurrencySettings)
	{
		Params.ConcurrencySet.Add(ConcurrencySettings);
	}

	UAtomComponent* AtomComponent = FAtomRuntime::CreateComponent(Sound, Params);
	if (AtomComponent)
	{
		AtomComponent->SetVolumeMultiplier(VolumeMultiplier);
		AtomComponent->SetPitchMultiplier(PitchMultiplier);
		AtomComponent->bAllowSpatialization = false;
		AtomComponent->bIsUISound = true;
		AtomComponent->bAutoDestroy = bAutoDestroy;
		AtomComponent->bIgnoreForFlushing = bPersistAcrossLevelTransition;
		//AtomComponent->SubtitlePriority = Sound->GetSubtitlePriority();
		AtomComponent->bStopWhenOwnerDestroyed = false;
	}
	return AtomComponent;
}

void UAtomGameplayStatics::PlaySoundAtLocation(const UObject* WorldContextObject, UAtomSoundBase* Sound, FVector Location, FRotator Rotation, float VolumeMultiplier /* = 1.f */, float PitchMultiplier /* = 1.f */, float StartTime /* = 0.f */, class UAtomAttenuation* AttenuationSettings /* = nullptr */, UAtomConcurrency* ConcurrencySettings /* = nullptr */, const AActor* OwningActor /* = nullptr */, UAtomInitialActiveSoundParams* InitialParams /* = nullptr */)
{
	if (!Sound || !GEngine)
	{
		return;
	}

	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!ThisWorld || !ThisWorld->bAllowAudioPlayback || ThisWorld->IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	if (FAtomRuntimeHandle AtomRuntime = FAtomRuntimeManager::GetAtomRuntimeFromWorld(ThisWorld))
	{
		TArray<FAtomAisacParameter> Params;
		if (InitialParams)
		{
			Params.Append(MoveTemp(InitialParams->AtomParams));
		}

		// If OwningActor isn't supplied to this function, derive an owner from the WorldContextObject
		const AActor* WorldContextOwner = Cast<const AActor>(WorldContextObject);
		if (!WorldContextOwner)
		{
			WorldContextOwner = WorldContextObject->GetTypedOuter<AActor>();
		}
		const AActor* ActiveSoundOwner = OwningActor ? OwningActor : WorldContextOwner;

		UAtomActorAisacParameter::Fill(ActiveSoundOwner, Params);

		AtomRuntime->PlaySoundAtLocation(Sound, ThisWorld, VolumeMultiplier, PitchMultiplier, StartTime, Location, Rotation, AttenuationSettings, ConcurrencySettings, nullptr /*&Params*/, ActiveSoundOwner);
	}
}

UAtomComponent* UAtomGameplayStatics::SpawnSoundAtLocation(const UObject* WorldContextObject, UAtomSoundBase* Sound, FVector Location, FRotator Rotation /* = FRotator::ZeroRotator */, float VolumeMultiplier /* = 1.f */, float PitchMultiplier /* = 1.f */, float StartTime /* = 0.f */, class UAtomAttenuation* AttenuationSettings /* = nullptr */, UAtomConcurrency* ConcurrencySettings /* = nullptr */, bool bAutoDestroy /* = true */)
{
	if (!Sound || !GEngine)
	{
		return nullptr;
	}

	UWorld* ThisWorld = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!ThisWorld || !ThisWorld->bAllowAudioPlayback || ThisWorld->IsNetMode(NM_DedicatedServer))
	{
		return nullptr;
	}

	const bool bIsInGameWorld = ThisWorld->IsGameWorld();

	// Derive an owner from the WorldContextObject
	AActor* WorldContextOwner = Cast<AActor>(const_cast<UObject*>(WorldContextObject));
	if (!WorldContextOwner)
	{
		WorldContextOwner = WorldContextObject->GetTypedOuter<AActor>();;
	}

	FAtomRuntime::FCreateComponentParams Params(ThisWorld, WorldContextOwner);
	Params.SetLocation(Location);
	Params.AttenuationSettings = AttenuationSettings;

	if (ConcurrencySettings)
	{
		Params.ConcurrencySet.Add(ConcurrencySettings);
	}

	UAtomComponent* AtomComponent = FAtomRuntime::CreateComponent(Sound, Params);

	if (AtomComponent)
	{
		AtomComponent->SetWorldLocationAndRotation(Location, Rotation);
		AtomComponent->SetVolumeMultiplier(VolumeMultiplier);
		AtomComponent->SetPitchMultiplier(PitchMultiplier);
		AtomComponent->bAllowSpatialization = Params.ShouldUseAttenuation();
		AtomComponent->bIsUISound = !bIsInGameWorld;
		AtomComponent->bAutoDestroy = bAutoDestroy;
		//AtomComponent->SubtitlePriority = Sound->GetSubtitlePriority();
		AtomComponent->bStopWhenOwnerDestroyed = false;
		AtomComponent->Play(StartTime);
	}

	return AtomComponent;
}

UAtomComponent* UAtomGameplayStatics::SpawnSoundAttached(UAtomSoundBase* Sound, USceneComponent* AttachToComponent, FName AttachPointName /* = NAME_None */, FVector Location /* = FVector(ForceInit) */, FRotator Rotation /* = FRotator::ZeroRotator */, EAttachLocation::Type LocationType /* = EAttachLocation::KeepRelativeOffset */, bool bStopWhenAttachedToDestroyed /* = false */, float VolumeMultiplier /* = 1.f */, float PitchMultiplier /* = 1.f */, float StartTime /* = 0.f */, UAtomAttenuation* AttenuationSettings /* = nullptr */, UAtomConcurrency* ConcurrencySettings /* = nullptr */, bool bAutoDestroy /* = true */)
{
	if (!Sound)
	{
		return nullptr;
	}

	if (!AttachToComponent)
	{
		UE_LOG(LogScript, Warning, TEXT("UGameplayStatics::SpawnSoundAttached: NULL AttachComponent specified! Trying to spawn sound [%s],"), *Sound->GetName());
		return nullptr;
	}

	UWorld* const ThisWorld = AttachToComponent->GetWorld();
	if (ThisWorld && ThisWorld->IsNetMode(NM_DedicatedServer))
	{
		// FAudioDevice::CreateComponent will fail to create the AudioComponent in a real dedicated server, but we need to check netmode here for Editor support.
		return nullptr;
	}

	// Location used to check whether to create a component if out of range
	FVector TestLocation = Location;
	if (LocationType == EAttachLocation::KeepRelativeOffset)
	{
		if (AttachPointName != NAME_None)
		{
			TestLocation = AttachToComponent->GetSocketTransform(AttachPointName).TransformPosition(Location);
		}
		else
		{
			TestLocation = AttachToComponent->GetComponentTransform().TransformPosition(Location);
		}
	}
	else if (LocationType == EAttachLocation::SnapToTarget || LocationType == EAttachLocation::SnapToTargetIncludingScale)
	{
		// If AttachPointName is NAME_None, will return just the component position
		TestLocation = AttachToComponent->GetSocketLocation(AttachPointName);
	}

	FAtomRuntime::FCreateComponentParams Params(ThisWorld, AttachToComponent->GetOwner());
	Params.SetLocation(TestLocation);
	Params.bStopWhenOwnerDestroyed = bStopWhenAttachedToDestroyed;
	Params.AttenuationSettings = AttenuationSettings;

	if (ConcurrencySettings)
	{
		Params.ConcurrencySet.Add(ConcurrencySettings);
	}

	UAtomComponent* AtomComponent = FAtomRuntime::CreateComponent(Sound, Params);
	if (AtomComponent)
	{
		if (UWorld* ComponentWorld = AtomComponent->GetWorld())
		{
			const bool bIsInGameWorld = ComponentWorld->IsGameWorld();

			if (LocationType == EAttachLocation::SnapToTarget || LocationType == EAttachLocation::SnapToTargetIncludingScale)
			{
				AtomComponent->AttachToComponent(AttachToComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachPointName);
			}
			else
			{
				AtomComponent->AttachToComponent(AttachToComponent, FAttachmentTransformRules::KeepRelativeTransform, AttachPointName);
				if (LocationType == EAttachLocation::KeepWorldPosition)
				{
					AtomComponent->SetWorldLocationAndRotation(Location, Rotation);
				}
				else
				{
					AtomComponent->SetRelativeLocationAndRotation(Location, Rotation);
				}
			}

			AtomComponent->SetVolumeMultiplier(VolumeMultiplier);
			AtomComponent->SetPitchMultiplier(PitchMultiplier);
			AtomComponent->bAllowSpatialization = Params.ShouldUseAttenuation();
			AtomComponent->bIsUISound = !bIsInGameWorld;
			AtomComponent->bAutoDestroy = bAutoDestroy;
			//AtomComponent->SubtitlePriority = Sound->GetSubtitlePriority();
			AtomComponent->Play(StartTime);
		}
	}

	return AtomComponent;
}

void UAtomGameplayStatics::PrimeSound(UAtomSoundBase* InSound)
{
	if (!InSound || !GCriWare)
	{
		return;
	}

	if (IAtomSoundResource* Source = InSound->GetSoundResource())
	{
		FAtomLoadParams LoadParams;
		LoadParams.Source = Source;
		LoadParams.Scope = EAtomResourceScope::Shared;
		LoadParams.bIsStreamed = true;
		LoadParams.bAsyncLoad = true;
		LoadParams.OnLoadCompleted = nullptr;
	
		FAtomResourceHandle AtomResource = FAtomRuntimeManager::Get()->GetAtomResourceManager().RequestSoundResource(LoadParams);
		if (auto SoundBank = Cast<UAtomSoundBank>(Source))
		{
			SoundBank->AtomResource = AtomResource;
		}
	}
}

void UAtomGameplayStatics::PrimeAllSoundsInSoundClass(class UAtomSoundClass* InSoundClass)
{
	for (TObjectIterator<UAtomSoundBase> Itr; Itr; ++Itr)
	{
		const UAtomSoundClass* SoundClass = Itr->GetSoundClass();
		if (SoundClass && (SoundClass->GetName() == InSoundClass->GetName()))
		{
			PrimeSound(*Itr);
		}
	}
}

/*void UAtomGameplayStatics::UnRetainAllSoundsInSoundClass(class UAtomSoundClass* InSoundClassy)
{
	for (TObjectIterator<UAtomSoundBase> Itr; Itr; ++Itr)
	{
		const UAtomSoundClass* SoundClass = Itr->GetSoundClass();
		if (SoundClass && (SoundClass->GetName() == InSoundClass->GetName()))
		{
			//Itr->;
		}
	}
}*/
