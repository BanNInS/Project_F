
#include "Atom/Gameplay/AtomSoundHandleSubsystem.h"

#include "Atom/AtomActiveSound.h"
#include "Atom/AtomRuntime.h"
#include "Atom/Interfaces/IAtomSoundHandleOwner.h"

bool UAtomSoundHandleSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer) && !IsRunningDedicatedServer();
}

void UAtomSoundHandleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
}

void UAtomSoundHandleSubsystem::Deinitialize()
{
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	ActiveHandles.Reset();
	Super::Deinitialize();
}

void UAtomSoundHandleSubsystem::NotifyActiveSoundDeleting(const FAtomActiveSound& ActiveSound)
{
	NotifySoundDeleting(ActiveSound);
}

void UAtomSoundHandleSubsystem::NotifyVirtualizedSoundDeleting(const FAtomActiveSound& ActiveSound)
{
	NotifySoundDeleting(ActiveSound);
}

void UAtomSoundHandleSubsystem::NotifySoundDeleting(const FAtomActiveSound& ActiveSound)
{
	check(IsInAtomThread());
	if (ActiveHandles.Num() > 0)
	{
		const Atom::FSoundHandleID HandleID = ActiveSound.GetInstanceID();
		ActiveHandles.Remove(HandleID);
		if(IAtomSoundHandleOwner* Owner = Owners.FindAndRemoveChecked(HandleID))
		{	
			Owner->OnSoundHandleRemoved();
		}
	}
}

Atom::FSoundHandleID UAtomSoundHandleSubsystem::CreateSoundHandle(UAtomSoundBase* Sound, IAtomSoundHandleOwner* Owner)
{
	if(Sound && Owner)
	{
		FSoundHandle NewHandle;
		NewHandle.ActiveSound.bLocationDefined = false;
		Atom::FSoundHandleID HandleID = NewHandle.ActiveSound.GetInstanceID();
		NewHandle.ActiveSound.SetSound(Sound);
		ActiveHandles.Emplace(HandleID, NewHandle);
		Owners.Emplace(HandleID, Owner);
		return HandleID;
	}
	return INDEX_NONE;
}

void UAtomSoundHandleSubsystem::SetTransform(const Atom::FSoundHandleID ID, const FTransform& Transform)
{
	// TODO: We should add this as an incoming change to handle on the audio thread in the update
	if(FSoundHandle* Handle = ActiveHandles.Find(ID))
	{
		Handle->ActiveSound.bLocationDefined = true;
		Handle->ActiveSound.Transform = Transform;
	}	
}

Atom::EResult UAtomSoundHandleSubsystem::Play(const Atom::FSoundHandleID ID)
{
	if(FSoundHandle* Handle = ActiveHandles.Find(ID))
	{
		FAtomRuntime* AtomRuntime = GetAtomRuntimeHandle().GetAtomRuntime();
		if (Handle->ActiveSound.bLocationDefined)
		{
			float MaxDistance = 0.0f;
			float FocusFactor = 1.0f;
			const FVector Location = Handle->ActiveSound.Transform.GetLocation();

			const TObjectPtr<UAtomSoundBase> Sound = Handle->ActiveSound.GetSound();
			const FAtomAttenuationSettings* AttenuationSettings = Sound->GetAttenuationSettingsToApply();
			AtomRuntime->GetMaxDistanceAndFocusFactor(Sound, GetWorld(), Location, AttenuationSettings, MaxDistance, FocusFactor);
			
			Handle->ActiveSound.bHasAttenuationSettings = (AttenuationSettings != nullptr);
			if (Handle->ActiveSound.bHasAttenuationSettings)
			{
				Handle->ActiveSound.AttenuationSettings = *AttenuationSettings;
				Handle->ActiveSound.FocusData.PriorityScale = AttenuationSettings->GetFocusPriorityScale(GetAtomRuntimeHandle().GetAtomRuntime()->GetGlobalFocusSettings(), FocusFactor);
			}

			Handle->ActiveSound.MaxDistance = MaxDistance;
		}
		
		AtomRuntime->AddNewActiveSound(Handle->ActiveSound);		
		return Atom::EResult::Success;
	}
	
	return Atom::EResult::Failure;
}

void UAtomSoundHandleSubsystem::Stop(const Atom::FSoundHandleID ID)
{	
	if(FSoundHandle* Handle = ActiveHandles.Find(ID))
	{
		FAtomRuntime* AtomRuntime = GetAtomRuntimeHandle().GetAtomRuntime();
		AtomRuntime->AddSoundToStop(&Handle->ActiveSound);
	}
}
