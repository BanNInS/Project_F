
#include "Atom/AtomConcurrency.h"

#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtomConcurrency)

DEFINE_LOG_CATEGORY(LogCriWareAtomConcurrency);

#define ATOM_TRACE_CONCURRENCY 0

static float AtomConcurrencyMinVolumeScaleCVar = 1.e-3f;
FAutoConsoleVariableRef CVarAtomConcurrencyMinVolumeScale(
	TEXT("atom.Concurrency.MinVolumeScale"),
	AtomConcurrencyMinVolumeScaleCVar,
	TEXT("Volume threshold considered silent for volume scaling (linear scale).\n"),
	ECVF_Default);


namespace AtomConcurrency
{
	void SetSoundDataTarget(const FAtomActiveSound& ActiveSound, FAtomConcurrencySoundData& SoundData, float InTargetVolume, float InLerpTime)
	{
#if UE_BUILD_SHIPPING
		SoundData.SetTarget(InTargetVolume, InLerpTime);
#else
		const float LastTargetVolume = SoundData.GetTargetVolume();

		SoundData.SetTarget(InTargetVolume, InLerpTime);
		if (!FMath::IsNearlyEqual(LastTargetVolume, InTargetVolume))
		{
			if (const UAtomSoundBase* Sound = ActiveSound.GetSound())
			{
				UE_LOG(LogCriWareAtomConcurrency, Verbose,
					TEXT("Sound '%s' concurrency generation '%i' target volume update: %.3f to %.3f."),
					*Sound->GetName(),
					SoundData.Generation,
					LastTargetVolume,
					InTargetVolume);
			}
		}
#endif // UE_BUILD_SHIPPING
	}
} // namespace

UAtomConcurrency::UAtomConcurrency(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAtomConcurrency::PostInitProperties()
{
	Super::PostInitProperties();

#if !UE_BUILD_SHIPPING
	Concurrency.SetDebugName(GetFName());
#endif
}

float FAtomConcurrencySettings::GetVolumeScale() const
{
	return FMath::Clamp(VolumeScale, 0.0f, 1.0f);;
}

bool FAtomConcurrencySettings::IsEvictionSupported() const
{
	switch (ResolutionRule)
	{
		case EAtomMaxConcurrentResolutionRule::StopLowestPriority:
		case EAtomMaxConcurrentResolutionRule::StopLowestPriorityThenPreventNew:
		case EAtomMaxConcurrentResolutionRule::StopQuietest:
		{
			return false;
		}

		case EAtomMaxConcurrentResolutionRule::PreventNew:
		case EAtomMaxConcurrentResolutionRule::StopOldest:
		case EAtomMaxConcurrentResolutionRule::StopFarthestThenPreventNew:
		case EAtomMaxConcurrentResolutionRule::StopFarthestThenOldest:
		default:
		{
			static_assert(static_cast<int32>(EAtomMaxConcurrentResolutionRule::Count) == 7,
				"Possible Missing EAtomMaxConcurrentResolutionRule switch case coverage");
			return true;
		}
	}
}


FAtomConcurrencyHandle::FAtomConcurrencyHandle(const FAtomConcurrencySettings& InSettings)
	: Settings(InSettings)
	, ObjectID(0)
	, bIsOverride(true)
{
}

FAtomConcurrencyHandle::FAtomConcurrencyHandle(const UAtomConcurrency& Concurrency)
	: Settings(Concurrency.Concurrency)
	, ObjectID(Concurrency.GetUniqueID())
	, bIsOverride(false)
{
}

EAtomConcurrencyMode FAtomConcurrencyHandle::GetMode(const FAtomActiveSound& ActiveSound) const
{
	if (Settings.bLimitToOwner && ActiveSound.GetOwnerID() != 0)
	{
		return bIsOverride && ActiveSound.GetSound() != nullptr
			? EAtomConcurrencyMode::OwnerPerSound
			: EAtomConcurrencyMode::Owner;
	}

	return ObjectID == 0 ? EAtomConcurrencyMode::Sound : EAtomConcurrencyMode::Group;
}

void FAtomConcurrencySoundData::Update(float InElapsed)
{
	Elapsed += InElapsed;
}

float FAtomConcurrencySoundData::GetLerpTime() const
{
	return LerpTime;
}

float FAtomConcurrencySoundData::GetVolume(bool bInDecibels) const
{
	const float Alpha = FMath::Clamp(Elapsed / FMath::Max(LerpTime, UE_SMALL_NUMBER), 0.0f, 1.0f);
	const float VolumeDb = FMath::Lerp(DbStartVolume, DbTargetVolume, Alpha);

	if (bInDecibels)
	{
		return VolumeDb;
	}
	
	float VolumeLin = Audio::ConvertToLinear(VolumeDb);
	if (VolumeLin < AtomConcurrencyMinVolumeScaleCVar || FMath::IsNearlyEqual(VolumeLin, AtomConcurrencyMinVolumeScaleCVar))
	{
		return 0.0f;
	}
	return VolumeLin;
}

float FAtomConcurrencySoundData::GetTargetVolume(bool bInDecibels) const
{
	if (bInDecibels)
	{
		return DbTargetVolume;
	}

	const float VolumeLin = Audio::ConvertToLinear(DbTargetVolume);
	if (VolumeLin < AtomConcurrencyMinVolumeScaleCVar || FMath::IsNearlyEqual(VolumeLin, AtomConcurrencyMinVolumeScaleCVar))
	{
		return 0.0f;
	}

	return VolumeLin;
}

void FAtomConcurrencySoundData::SetTarget(float InTargetVolume, float InLerpTime)
{
	DbStartVolume = GetVolume(true /* bInDecibels */);
	DbTargetVolume = Audio::ConvertToDecibels(InTargetVolume, AtomConcurrencyMinVolumeScaleCVar);
	LerpTime = FMath::Max(InLerpTime, 0.0f);
	Elapsed = 0.0f;
}

FAtomConcurrencyGroup::FAtomConcurrencyGroup(FAtomConcurrencyGroupId InGroupID, const FAtomConcurrencyHandle& ConcurrencyHandle)
	: GroupID(InGroupID)
	, ObjectID(ConcurrencyHandle.ObjectID)
	, Settings(ConcurrencyHandle.Settings)
	, LastTimePlayed(FPlatformTime::ToSeconds64(FPlatformTime::Cycles64()))
{
}

FAtomConcurrencyGroupId FAtomConcurrencyGroup::GenerateNewID()
{
	static FAtomConcurrencyGroupId ConcurrencyGroupIDs = 0;
	return ++ConcurrencyGroupIDs;
}

void FAtomConcurrencyGroup::AddActiveSound(FAtomActiveSound& ActiveSound)
{
	check(GroupID != 0);

	if (ActiveSound.ConcurrencyGroupData.Contains(GroupID))
	{
		UE_LOG(LogCriWareAtomConcurrency, Fatal, TEXT("Attempting to add active sound '%s' to concurrency group multiple times."), *ActiveSound.GetOwnerName());
		return;
	}

	FAtomConcurrencySoundData SoundData;

	if (Settings.VolumeScaleMode == EAtomConcurrencyVolumeScaleMode::Default)
	{
		SoundData.Generation = ActiveSounds.Num();
		AtomConcurrency::SetSoundDataTarget(ActiveSound, SoundData, 1.0f, 0.0f);
	}

	ActiveSounds.Add(&ActiveSound);
	ActiveSound.ConcurrencyGroupData.Add(GroupID, MoveTemp(SoundData));

	UpdateGeneration(&ActiveSound);
}

void FAtomConcurrencyGroup::RemoveActiveSound(FAtomActiveSound& ActiveSound)
{
	// Remove from array
	const int32 NumRemoved = ActiveSounds.RemoveSwap(&ActiveSound);
	if (NumRemoved == 0)
	{
		return;
	}
	check(NumRemoved == 1);

	FAtomConcurrencySoundData& SoundData = ActiveSound.ConcurrencyGroupData.FindChecked(GroupID);

	// Rebase generations due to removal of a member
	for (FAtomActiveSound* OtherSound : ActiveSounds)
	{
		check(OtherSound);
		FAtomConcurrencySoundData& OtherSoundData = OtherSound->ConcurrencyGroupData.FindChecked(GroupID);
		if (OtherSoundData.Generation > SoundData.Generation)
		{
			check(OtherSoundData.Generation > 0);
			check(OtherSoundData.Generation <= ActiveSounds.Num());
			OtherSoundData.Generation--;
		}

		if (Settings.bVolumeScaleCanRelease && Settings.VolumeScaleMode == EAtomConcurrencyVolumeScaleMode::Default)
		{
			const float GenerationDelta = static_cast<float>(ActiveSounds.Num() - OtherSoundData.Generation) - 1.0f;
			const float NewTargetVolume = FMath::Clamp(FMath::Pow(Settings.GetVolumeScale(), GenerationDelta), 0.0f, 1.0f);
			AtomConcurrency::SetSoundDataTarget(*OtherSound, OtherSoundData, NewTargetVolume, Settings.VolumeScaleReleaseTime);
		}
	}
}

void FAtomConcurrencyGroup::UpdateGeneration(FAtomActiveSound* NewActiveSound)
{
	// Default uses value set at time of adding sound to group, so no sort required
	if (Settings.VolumeScaleMode == EAtomConcurrencyVolumeScaleMode::Default)
	{
		return;
	}

	struct FCompareActiveSounds
	{
		EAtomConcurrencyVolumeScaleMode ScaleMode;

		FCompareActiveSounds(EAtomConcurrencyVolumeScaleMode InScaleMode)
			: ScaleMode(InScaleMode)
		{
		}

		FORCEINLINE bool operator()(const FAtomActiveSound& A, const FAtomActiveSound& B) const
		{
			switch(ScaleMode)
			{
				case EAtomConcurrencyVolumeScaleMode::Distance:
				{
					float DistSqA = 0.0f;
					float DistSqB = 0.0f;
					A.GetAtomRuntime()->GetDistanceSquaredToNearestListener(A.LastLocation, DistSqA);
					B.GetAtomRuntime()->GetDistanceSquaredToNearestListener(B.LastLocation, DistSqB);

					// If sounds share the same distance, newer sounds will be sorted first to avoid volume ping-ponging 
					if (FMath::IsNearlyEqual(DistSqA, DistSqB, UE_KINDA_SMALL_NUMBER))
					{
						return A.GetPlayOrder() < B.GetPlayOrder();
					}

					return DistSqA > DistSqB;
				}

				case EAtomConcurrencyVolumeScaleMode::Priority:
				{
					// Ensures sounds set to always play are sorted above those that aren't, but are sorted appropriately between one another
					auto GetPriority = [](const FAtomActiveSound& InActiveSound)
					{
						if (InActiveSound.GetAlwaysPlay())
						{
							return InActiveSound.GetHighestPriority(true) * InActiveSound.Priority + ATOM_MAX_SOUND_PRIORITY + 1.0f;
						}
						else
						{
							return InActiveSound.GetHighestPriority() * InActiveSound.Priority;
						}
					};
					const float APriority = GetPriority(A);
					const float BPriority = GetPriority(B);

					// If sounds share the same priority, newer sounds will be sorted last to avoid volume ping-ponging 
					if (FMath::IsNearlyEqual(APriority, BPriority, UE_KINDA_SMALL_NUMBER))
					{
						return A.GetPlayOrder() < B.GetPlayOrder();
					}

					return APriority < BPriority;
				}
			}

			return true;
		}
	};

	ActiveSounds.Sort(FCompareActiveSounds(Settings.VolumeScaleMode));

	for (int32 i = 0; i < ActiveSounds.Num(); ++i)
	{
		FAtomActiveSound* ActiveSound = ActiveSounds[i];
		
		if (FAtomConcurrencySoundData* SoundData = ActiveSound->ConcurrencyGroupData.Find(GetGroupID()))
		{
			SoundData->Generation = i;
			const float GenerationDelta = static_cast<float>(ActiveSounds.Num() - SoundData->Generation) - 1.0f;
			const float NewTargetVolume = FMath::Clamp(FMath::Pow(Settings.GetVolumeScale(), GenerationDelta), 0.0f, 1.0f);

			// If new sound added, immediately lerp to the generation volume.
			const float AttackTime = NewActiveSound == ActiveSound ? 0.0f : Settings.VolumeScaleAttackTime;

			if (!FMath::IsNearlyEqual(AttackTime, SoundData->GetLerpTime()) || !FMath::IsNearlyEqual(SoundData->GetTargetVolume(), NewTargetVolume, AtomConcurrencyMinVolumeScaleCVar))
			{
				AtomConcurrency::SetSoundDataTarget(*ActiveSound, *SoundData, NewTargetVolume, AttackTime);
			}
		}
	}
}

void FAtomConcurrencyGroup::CullSoundsDueToMaxConcurrency()
{
	// Nothing to do if our active sound count is less than or equal to our max active sounds
	// or if eviction is supported
	if (ActiveSounds.Num() <= Settings.MaxCount)
	{
		return;
	}

	// Comparator for sorting group's ActiveSounds according to their dynamic concurrency nature.
	// Quieter/lower priority sounds will be at the front of the array.
	struct FCompareActiveSounds
	{
		EAtomMaxConcurrentResolutionRule ResolutionRule;

		FCompareActiveSounds(EAtomMaxConcurrentResolutionRule InResolutionRule)
			: ResolutionRule(InResolutionRule)
		{
		}

		FORCEINLINE bool operator()(const FAtomActiveSound& A, const FAtomActiveSound& B) const
		{
			switch (ResolutionRule)
			{
				case EAtomMaxConcurrentResolutionRule::StopQuietest:
				{
					// If sounds share the same volume, newer sounds will be sorted first to avoid loop realization ping-ponging 
					if (FMath::IsNearlyEqual(A.VolumeConcurrency, B.VolumeConcurrency, UE_KINDA_SMALL_NUMBER))
					{
						return A.GetPlayOrder() > B.GetPlayOrder();
					}
					return A.VolumeConcurrency < B.VolumeConcurrency;
				}
				break;

				case EAtomMaxConcurrentResolutionRule::StopLowestPriority:
				case EAtomMaxConcurrentResolutionRule::StopLowestPriorityThenPreventNew:
				{
					// Ensures sounds set to always play are sorted above those that aren't, but are sorted appropriately between one another
					auto GetPriority = [](const FAtomActiveSound& InActiveSound)
					{
						if (InActiveSound.GetAlwaysPlay())
						{
							return InActiveSound.GetHighestPriority(true) * InActiveSound.Priority + ATOM_MAX_SOUND_PRIORITY + 1.0f;
						}
						else
						{
							return InActiveSound.GetHighestPriority() * InActiveSound.Priority;
						}
					};
					const float APriority = GetPriority(A);
					const float BPriority = GetPriority(B);

					if (!FMath::IsNearlyEqual(APriority, BPriority, UE_KINDA_SMALL_NUMBER))
					{
						return APriority < BPriority;
					}

					// Newer sounds pushed forward in sort to make them more likely to be culled if PreventNew
					return ResolutionRule == EAtomMaxConcurrentResolutionRule::StopLowestPriorityThenPreventNew
						? A.GetPlayOrder() > B.GetPlayOrder()
						: A.GetPlayOrder() < B.GetPlayOrder();
				}
				break;

				default:
				case EAtomMaxConcurrentResolutionRule::PreventNew:
				case EAtomMaxConcurrentResolutionRule::StopOldest:
				case EAtomMaxConcurrentResolutionRule::StopFarthestThenPreventNew:
				case EAtomMaxConcurrentResolutionRule::StopFarthestThenOldest:
				{
					static_assert(static_cast<int32>(EAtomMaxConcurrentResolutionRule::Count) == 7,
						"Possible Missing EAtomMaxConcurrentResolutionRule switch case coverage");
					return true;
				}
				break;
			}
		}
	};

	ActiveSounds.Sort(FCompareActiveSounds(Settings.ResolutionRule));

	// Need to make a new list when stopping the sounds since the process of stopping an active sound
	// will remove the sound from this concurrency group's ActiveSounds array.
	const int32 NumSoundsToStop = ActiveSounds.Num() - Settings.MaxCount;
	check(NumSoundsToStop > 0);
	for (int32 i = 0; i < ActiveSounds.Num(); ++i)
	{
		FAtomActiveSound* ActiveSound = ActiveSounds[i];
		check(ActiveSound);

		const bool bShouldStop = i < NumSoundsToStop;
		ActiveSound->bShouldStopDueToMaxConcurrency = bShouldStop;
	}
}

bool FAtomConcurrencyGroup::CanPlaySoundNow(float InCurrentTime) const
{
	if (Settings.RetriggerTime > KINDA_SMALL_NUMBER)
	{
		const float DeltaTime = InCurrentTime - LastTimePlayed;
		if (DeltaTime < Settings.RetriggerTime)
		{
			UE_LOG(LogCriWareAtomConcurrency, VeryVerbose, TEXT("Rejected Sound for Group ID (%d) with DeltaTime: %.2f"), GroupID, DeltaTime);
			return false;
		}
	}
	return true;
}

FAtomConcurrencyManager::FAtomConcurrencyManager(class FAtomRuntime* InAtomRuntime)
	: AtomRuntime(InAtomRuntime)
{
}

FAtomConcurrencyManager::~FAtomConcurrencyManager()
{
}

void FAtomConcurrencyManager::CreateNewGroupsFromHandles(
	const FAtomActiveSound& NewActiveSound,
	const TArray<FAtomConcurrencyHandle>& ConcurrencyHandles,
	TArray<FAtomConcurrencyGroup*>& OutGroupsToApply
)
{
	for (const FAtomConcurrencyHandle& ConcurrencyHandle : ConcurrencyHandles)
	{
		switch (ConcurrencyHandle.GetMode(NewActiveSound))
		{
			case EAtomConcurrencyMode::Group:
			{
				if (!ConcurrencyMap.Contains(ConcurrencyHandle.ObjectID))
				{
					FAtomConcurrencyGroup& ConcurrencyGroup = CreateNewConcurrencyGroup(ConcurrencyHandle);
					ConcurrencyMap.Add(ConcurrencyHandle.ObjectID, ConcurrencyGroup.GetGroupID());
					OutGroupsToApply.Add(&ConcurrencyGroup);
				}
			}
			break;

			case EAtomConcurrencyMode::Owner:
			{
				const FAtomSoundOwnerObjectId OwnerObjectID = NewActiveSound.GetOwnerID();
				if (FAtomOwnerConcurrencyMapEntry* ConcurrencyEntry = OwnerConcurrencyMap.Find(OwnerObjectID))
				{
					if (!ConcurrencyEntry->ConcurrencyObjectToConcurrencyGroup.Contains(ConcurrencyHandle.ObjectID))
					{
						FAtomConcurrencyGroup& ConcurrencyGroup = CreateNewConcurrencyGroup(ConcurrencyHandle);
						ConcurrencyEntry->ConcurrencyObjectToConcurrencyGroup.Add(ConcurrencyHandle.ObjectID, ConcurrencyGroup.GetGroupID());
						OutGroupsToApply.Add(&ConcurrencyGroup);
					}
				}
				else
				{
					FAtomConcurrencyGroup& ConcurrencyGroup = CreateNewConcurrencyGroup(ConcurrencyHandle);
					OwnerConcurrencyMap.Emplace(OwnerObjectID, FAtomOwnerConcurrencyMapEntry(ConcurrencyHandle.ObjectID, ConcurrencyGroup.GetGroupID()));
					OutGroupsToApply.Add(&ConcurrencyGroup);
				}
			}
			break;

			case EAtomConcurrencyMode::OwnerPerSound:
			{
				UAtomSoundBase* Sound = NewActiveSound.GetSound();
				check(Sound);

				const FAtomSoundObjectId SoundObjectID = static_cast<FAtomSoundObjectId>(Sound->GetUniqueID());
				const FAtomSoundOwnerObjectId OwnerObjectID = NewActiveSound.GetOwnerID();

				if (FAtomSoundInstanceEntry* InstanceEntry = OwnerPerSoundConcurrencyMap.Find(OwnerObjectID))
				{
					if (!InstanceEntry->SoundInstanceToConcurrencyGroup.Contains(SoundObjectID))
					{
						FAtomConcurrencyGroup& ConcurrencyGroup = CreateNewConcurrencyGroup(ConcurrencyHandle);
						InstanceEntry->SoundInstanceToConcurrencyGroup.Add(SoundObjectID, ConcurrencyGroup.GetGroupID());
						OutGroupsToApply.Add(&ConcurrencyGroup);
					}
				}
				else
				{
					FAtomConcurrencyGroup& ConcurrencyGroup = CreateNewConcurrencyGroup(ConcurrencyHandle);
					OwnerPerSoundConcurrencyMap.Emplace(OwnerObjectID, FAtomSoundInstanceEntry(SoundObjectID, ConcurrencyGroup.GetGroupID()));
					OutGroupsToApply.Add(&ConcurrencyGroup);
				}
			}
			break;

			case EAtomConcurrencyMode::Sound:
			{
				const FAtomSoundObjectId SoundObjectID = NewActiveSound.GetSound()->GetUniqueID();
				if (!SoundObjectToConcurrencyGroup.Contains(SoundObjectID))
				{
					FAtomConcurrencyGroup& ConcurrencyGroup = CreateNewConcurrencyGroup(ConcurrencyHandle);
					SoundObjectToConcurrencyGroup.Add(SoundObjectID, ConcurrencyGroup.GetGroupID());
					OutGroupsToApply.Add(&ConcurrencyGroup);
				}
			}
			break;
		}
	}
}

FAtomActiveSound* FAtomConcurrencyManager::CreateNewActiveSound(const FAtomActiveSound& NewActiveSound, bool bIsRetriggering)
{
	check(NewActiveSound.GetSound());
	check(IsInAtomThread());

	// If there are no concurrency settings associated then there is no limit on this sound
	TArray<FAtomConcurrencyHandle> ConcurrencyHandles;
	NewActiveSound.GetConcurrencyHandles(ConcurrencyHandles);

	// If there was no concurrency or the setting was zero, then always play this sound.
	if (!ConcurrencyHandles.Num())
	{
		FAtomActiveSound* ActiveSound = new FAtomActiveSound(NewActiveSound);
		ActiveSound->PlaybackTimeNonVirtualized = 0.0f;
		ActiveSound->SetAtomRuntime(AtomRuntime);
		return ActiveSound;
	}

#if !UE_BUILD_SHIPPING
	for (const FAtomConcurrencyHandle& ConcurrencyHandle : ConcurrencyHandles)
	{
		check(ConcurrencyHandle.Settings.MaxCount > 0);
	}
#endif

	return EvaluateConcurrency(const_cast<FAtomActiveSound&>(NewActiveSound), ConcurrencyHandles, bIsRetriggering, false);
}

bool FAtomConcurrencyManager::AddActiveSound(FAtomActiveSound& ActiveSound)
{
	check(ActiveSound.GetSound());
	check(IsInAtomThread());

	// If there are no concurrency settings associated then there is no limit on this sound
	TArray<FAtomConcurrencyHandle> ConcurrencyHandles;
	ActiveSound.GetConcurrencyHandles(ConcurrencyHandles);

	// If there was no concurrency or the setting was zero, then always play this sound.
	if (!ConcurrencyHandles.Num())
	{
		return true;
	}

#if !UE_BUILD_SHIPPING
	for (const FAtomConcurrencyHandle& ConcurrencyHandle : ConcurrencyHandles)
	{
		check(ConcurrencyHandle.Settings.MaxCount > 0);
	}
#endif

	return EvaluateConcurrency(ActiveSound, ConcurrencyHandles, true, true) != nullptr;
}

bool FAtomConcurrencyManager::EvaluateActiveSound(const FAtomActiveSound& ActiveSound, bool bIsRetriggering)
{
	check(ActiveSound.GetSound());
	check(IsInAtomThread());

	// If there are no concurrency settings associated then there is no limit on this sound
	TArray<FAtomConcurrencyHandle> ConcurrencyHandles;
	ActiveSound.GetConcurrencyHandles(ConcurrencyHandles);

	// If there was no concurrency or the setting was zero, then always play this sound.
	if (!ConcurrencyHandles.Num())
	{
		return true;
	}

	TArray<FAtomActiveSound*> SoundsToEvict;
	TArray<FAtomConcurrencyGroup*> GroupsToApply;
	bool bEvictNew = false;
	
	return EvaluateConcurrencyInternal(ActiveSound, ConcurrencyHandles, GroupsToApply, SoundsToEvict, bIsRetriggering, false, bEvictNew) && !bEvictNew;
}

FAtomConcurrencyGroup& FAtomConcurrencyManager::CreateNewConcurrencyGroup(const FAtomConcurrencyHandle& ConcurrencyHandle)
{
	//Create & add new concurrency group to the map
	FAtomConcurrencyGroupId GroupID = FAtomConcurrencyGroup::GenerateNewID();
	ConcurrencyGroups.Emplace(GroupID, new FAtomConcurrencyGroup(GroupID, ConcurrencyHandle));

	return *ConcurrencyGroups.FindRef(GroupID);
}

FAtomConcurrencyGroup* FAtomConcurrencyManager::CanPlaySound(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroupId GroupID, TArray<FAtomActiveSound*>& OutSoundsToEvict, bool bIsRetriggering, bool bIsRealizing, bool& bOutEvictNew)
{
	check(GroupID != 0);
	FAtomConcurrencyGroup* ConcurrencyGroup = ConcurrencyGroups.FindRef(GroupID);
	if (!ConcurrencyGroup)
	{
		UE_LOG(LogCriWareAtomConcurrency, Warning, TEXT("Attempting to add active sound '%s' (owner '%s') to invalid concurrency group."),
			NewActiveSound.GetSound() ? *NewActiveSound.GetSound()->GetFullName() : TEXT("Unset"),
			*NewActiveSound.GetOwnerName());
		return nullptr;
	}

	// Check for rate limiting behavior
 	const float CurrentTime = FPlatformTime::ToSeconds64(FPlatformTime::Cycles64());
	if (!bIsRealizing && !ConcurrencyGroup->CanPlaySoundNow(CurrentTime))
	{
#if ATOM_TRACE_CONCURRENCY
		UE_LOG(LogCriWareAtomConcurrency, Log, TEXT("CanPlaySound %s -> No: Wait Retrigger Time (loop added, evicted)"), *NewActiveSound.GetSound()->GetFName().ToString());
#endif
		bOutEvictNew = true;
		return ConcurrencyGroup;
	}

	bool bCanPlayWhenSilent = NewActiveSound.CanVirtualizeWhenSilent();
	bool bCanEvict = NewActiveSound.GetSound() ? NewActiveSound.GetSound()->GetVirtualizationMode() != EAtomVirtualizationMode::StopWhenSilent : false;

	// Some settings don't support immediate eviction, they cull once we instantiate the sound.  This
	// is because it is not possible to evaluate sound volumes/priorities/etc *before* they play.
	if (!ConcurrencyGroup->GetSettings().IsEvictionSupported() && !bCanEvict)
	{
		// If we have successfully played a sound, then we update the last time a sound played here
		ConcurrencyGroup->SetLastTimePlayed(CurrentTime);

		return ConcurrencyGroup;
	}

	if (ConcurrencyGroup->IsFull())
	{
		// If no room for new sound, early out
		if (FAtomActiveSound* SoundToEvict = GetEvictableSound(NewActiveSound, *ConcurrencyGroup, bIsRetriggering))
		{
			OutSoundsToEvict.AddUnique(SoundToEvict);
		}
		else
		{
			if (bCanPlayWhenSilent)
			{
#if ATOM_TRACE_CONCURRENCY
				UE_LOG(LogCriWareAtomConcurrency, Log, TEXT("CanPlaySound %s -> No: Group Full (loop added, evicted)"), *NewActiveSound.GetSound()->GetFName().ToString());
#endif
				bOutEvictNew = true;
				return ConcurrencyGroup;
			}
			else
			{
				// If we have successfully played a sound, then we update the last time a sound played here
				ConcurrencyGroup->SetLastTimePlayed(CurrentTime);

				return nullptr;
			}
		}
	}

	// If we have successfully played a sound, then we update the last time a sound played here
	ConcurrencyGroup->SetLastTimePlayed(CurrentTime);

#if ATOM_TRACE_CONCURRENCY
	UE_LOG(LogCriWareAtomConcurrency, Log, TEXT("CanPlaySound %s -> Yes "), *NewActiveSound.GetSound()->GetFName().ToString());
#endif

	return ConcurrencyGroup;
}

FAtomActiveSound* FAtomConcurrencyManager::GetEvictableSound(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroup& ConcurrencyGroup, bool bIsRetriggering)
{
	check(ConcurrencyGroup.IsFull());

	const EAtomMaxConcurrentResolutionRule Rule = ConcurrencyGroup.GetSettings().ResolutionRule;
	switch (Rule)
	{
		case EAtomMaxConcurrentResolutionRule::PreventNew:
		{
			return nullptr;
		}
		break;

		case EAtomMaxConcurrentResolutionRule::StopOldest:
		{
			return GetEvictableSoundStopOldest(NewActiveSound, ConcurrencyGroup, bIsRetriggering);

		}
		break;

		case EAtomMaxConcurrentResolutionRule::StopFarthestThenPreventNew:
		case EAtomMaxConcurrentResolutionRule::StopFarthestThenOldest:
		{
			return GetEvictableSoundStopFarthest(NewActiveSound, ConcurrencyGroup, bIsRetriggering);
		}
		break;

		// Eviction not supported by following rules due to requiring the sound to be initialized in order to calculate.
		// Therefore, it is culled later and not evicted.
		case EAtomMaxConcurrentResolutionRule::StopLowestPriority:
		case EAtomMaxConcurrentResolutionRule::StopLowestPriorityThenPreventNew:
		{
			return GetEvictableSoundStopLowestPriority(NewActiveSound, ConcurrencyGroup, bIsRetriggering);
		}
		break;
		case EAtomMaxConcurrentResolutionRule::StopQuietest:
		{
			return GetEvictableSoundStopQuietest(NewActiveSound, ConcurrencyGroup, bIsRetriggering);
		}
		break;

		default:
		{
			static_assert(static_cast<int32>(EAtomMaxConcurrentResolutionRule::Count) == 7,
				"Possible Missing EAtomMaxConcurrentResolutionRule switch case coverage");
		}
		break;
	}

	return nullptr;
}

FAtomActiveSound* FAtomConcurrencyManager::GetEvictableSoundStopOldest(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroup& ConcurrencyGroup, bool bIsRetriggering) const
{
	FAtomActiveSound* EvictableSound = nullptr;
	const TArray<FAtomActiveSound*>& ActiveSounds = ConcurrencyGroup.GetActiveSounds();
	for (FAtomActiveSound* ActiveSound : ActiveSounds)
	{
		if (EvictableSound == nullptr || ActiveSound->PlaybackTime > EvictableSound->PlaybackTime)
		{
			EvictableSound = ActiveSound;
		}
	}

	// Don't evict if attempting to re-trigger an older sound than that which is currently playing
	if (bIsRetriggering && EvictableSound && NewActiveSound.PlaybackTime > EvictableSound->PlaybackTime)
	{
		return nullptr;
	}

	return EvictableSound;
}

FAtomActiveSound* FAtomConcurrencyManager::GetEvictableSoundStopFarthest(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroup& ConcurrencyGroup, bool bIsRetriggering) const
{
	const EAtomMaxConcurrentResolutionRule Rule = ConcurrencyGroup.GetSettings().ResolutionRule;

	check(AtomRuntime);
	float DistanceToStopSoundSq;
	const bool bAllowAttenuationOverrides = true;
	int32 ClosestListenerIndex = AtomRuntime->FindClosestListenerIndex(NewActiveSound.Transform.GetTranslation(), DistanceToStopSoundSq, bAllowAttenuationOverrides);

	FAtomActiveSound* EvictableSound = nullptr;
	const TArray<FAtomActiveSound*>& ActiveSounds = ConcurrencyGroup.GetActiveSounds();
	for (FAtomActiveSound* ActiveSound : ActiveSounds)
	{
		check(ActiveSound);

		float DistanceToActiveSoundSq;
		ClosestListenerIndex = AtomRuntime->FindClosestListenerIndex(ActiveSound->Transform.GetTranslation(), DistanceToActiveSoundSq, bAllowAttenuationOverrides);

		// Stop Farthest
		if (DistanceToActiveSoundSq > DistanceToStopSoundSq)
		{
			DistanceToStopSoundSq = DistanceToActiveSoundSq;
			EvictableSound = ActiveSound;
			continue;
		}

		// Stop Farthest than Oldest
		if (Rule == EAtomMaxConcurrentResolutionRule::StopFarthestThenOldest && DistanceToActiveSoundSq == DistanceToStopSoundSq)
		{
			const bool bIsOlderThanChosen = EvictableSound == nullptr || ActiveSound->PlaybackTime > EvictableSound->PlaybackTime;
			if (bIsOlderThanChosen)
			{
				// Don't evict if attempting to re-trigger an older sound than that which is currently playing
				if (!bIsRetriggering || ActiveSound->PlaybackTime > NewActiveSound.PlaybackTime)
				{
					DistanceToStopSoundSq = DistanceToActiveSoundSq;
					EvictableSound = ActiveSound;
				}
			}
		}
	}

	return EvictableSound;
}

FAtomActiveSound* FAtomConcurrencyManager::GetEvictableSoundStopQuietest(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroup& ConcurrencyGroup, bool bIsRetriggering) const
{
	auto IsQuietest = [](const FAtomActiveSound& A, const FAtomActiveSound& Other)
	{
		// If sounds share the same volume, newer sounds used first to avoid loop realization ping-ponging 
		if (FMath::IsNearlyEqual(A.VolumeConcurrency, Other.VolumeConcurrency, UE_KINDA_SMALL_NUMBER))
		{
			return A.GetPlayOrder() > Other.GetPlayOrder();
		}
		return A.VolumeConcurrency < Other.VolumeConcurrency;
	};

	FAtomActiveSound* EvictableSound = nullptr;
	const TArray<FAtomActiveSound*>& ActiveSounds = ConcurrencyGroup.GetActiveSounds();
	for (FAtomActiveSound* ActiveSound : ActiveSounds)
	{
		if (EvictableSound == nullptr || IsQuietest(*ActiveSound, *EvictableSound))
		{
			EvictableSound = ActiveSound;
		}
	}

	// Don't evict if attempting to re-trigger an older sound than that which is currently playing
	if (bIsRetriggering && EvictableSound && IsQuietest(NewActiveSound, *EvictableSound))
	{
		return nullptr;
	}

	return EvictableSound;
}

FAtomActiveSound* FAtomConcurrencyManager::GetEvictableSoundStopLowestPriority(const FAtomActiveSound& NewActiveSound, const FAtomConcurrencyGroup& ConcurrencyGroup, bool bIsRetriggering) const
{
	const EAtomMaxConcurrentResolutionRule Rule = ConcurrencyGroup.GetSettings().ResolutionRule;

	auto IsLowestPriority = [Rule](const FAtomActiveSound& A, const FAtomActiveSound& Other)
	{
		// Ensures sounds set to always play are sorted above those that aren't, but are sorted appropriately between one another
		auto GetPriority = [](const FAtomActiveSound& InActiveSound)
		{
			if (InActiveSound.GetAlwaysPlay())
			{
				return InActiveSound.GetHighestPriority(true) * InActiveSound.Priority + ATOM_MAX_SOUND_PRIORITY + 1.0f;
			}
			else
			{
				return InActiveSound.GetHighestPriority() * InActiveSound.Priority;
			}
		};
		const float APriority = GetPriority(A);
		const float BPriority = GetPriority(Other);

		if (!FMath::IsNearlyEqual(APriority, BPriority, UE_KINDA_SMALL_NUMBER))
		{
			return APriority < BPriority;
		}

		// Newer sounds pushed forward in sort to make them more likely to be culled if PreventNew
		return Rule == EAtomMaxConcurrentResolutionRule::StopLowestPriorityThenPreventNew
			? A.GetPlayOrder() > Other.GetPlayOrder()
			: A.GetPlayOrder() < Other.GetPlayOrder();
	};

	FAtomActiveSound* EvictableSound = nullptr;
	const TArray<FAtomActiveSound*>& ActiveSounds = ConcurrencyGroup.GetActiveSounds();
	for (FAtomActiveSound* ActiveSound : ActiveSounds)
	{
		if (EvictableSound == nullptr || IsLowestPriority(*ActiveSound, *EvictableSound))
		{
			EvictableSound = ActiveSound;
		}
	}

	// Don't evict if attempting to re-trigger a lower priority sound than that which is currently playing
	if (bIsRetriggering && EvictableSound && IsLowestPriority(NewActiveSound, *EvictableSound))
	{
		return nullptr;
	}

	return EvictableSound;
}

FAtomActiveSound* FAtomConcurrencyManager::EvaluateConcurrency(FAtomActiveSound& NewActiveSound, TArray<FAtomConcurrencyHandle>& ConcurrencyHandles, bool bIsRetriggering, bool bIsRealizing)
{
	check(NewActiveSound.GetSound());

	TArray<FAtomActiveSound*> SoundsToEvict;
	TArray<FAtomConcurrencyGroup*> GroupsToApply;
	bool bEvictNewActiveSound = false;

	if (EvaluateConcurrencyInternal(NewActiveSound, ConcurrencyHandles, GroupsToApply, SoundsToEvict, bIsRetriggering, bIsRealizing, bEvictNewActiveSound))
	{
		CreateNewGroupsFromHandles(NewActiveSound, ConcurrencyHandles, GroupsToApply);
		return CreateAndEvictActiveSounds(NewActiveSound, GroupsToApply, SoundsToEvict, bIsRealizing, bEvictNewActiveSound);
	}

	return nullptr;
}

bool FAtomConcurrencyManager::EvaluateConcurrencyInternal(const FAtomActiveSound& NewActiveSound, TArray<FAtomConcurrencyHandle>& ConcurrencyHandles, TArray<FAtomConcurrencyGroup*>& GroupsToApply, TArray<FAtomActiveSound*>& SoundsToEvict, bool bIsRetriggering, bool bIsRealizing, bool& bOutEvictNew)
{
	for (const FAtomConcurrencyHandle& ConcurrencyHandle : ConcurrencyHandles)
	{
		switch (ConcurrencyHandle.GetMode(NewActiveSound))
		{
		case EAtomConcurrencyMode::Group:
		{
			if (FAtomConcurrencyGroupId* ConcurrencyGroupID = ConcurrencyMap.Find(ConcurrencyHandle.ObjectID))
			{
				FAtomConcurrencyGroup* ConcurrencyGroup = CanPlaySound(NewActiveSound, *ConcurrencyGroupID, SoundsToEvict, bIsRetriggering, bIsRealizing, bOutEvictNew);
				if (!ConcurrencyGroup)
				{
					return false;
				}
				GroupsToApply.Add(ConcurrencyGroup);
			}
		}
		break;

		case EAtomConcurrencyMode::Owner:
		{
			if (FAtomOwnerConcurrencyMapEntry* ConcurrencyEntry = OwnerConcurrencyMap.Find(NewActiveSound.GetOwnerID()))
			{
				if (FAtomConcurrencyGroupId* ConcurrencyGroupID = ConcurrencyEntry->ConcurrencyObjectToConcurrencyGroup.Find(ConcurrencyHandle.ObjectID))
				{
					FAtomConcurrencyGroup* ConcurrencyGroup = CanPlaySound(NewActiveSound, *ConcurrencyGroupID, SoundsToEvict, bIsRetriggering, bIsRealizing, bOutEvictNew);
					if (!ConcurrencyGroup)
					{
						return false;
					}
					GroupsToApply.Add(ConcurrencyGroup);
				}
			}
		}
		break;

		case EAtomConcurrencyMode::OwnerPerSound:
		{
			const uint32 OwnerObjectID = NewActiveSound.GetOwnerID();
			if (FAtomSoundInstanceEntry* InstanceEntry = OwnerPerSoundConcurrencyMap.Find(OwnerObjectID))
			{
				UAtomSoundBase* Sound = NewActiveSound.GetSound();
				check(Sound);
				if (FAtomConcurrencyGroupId* ConcurrencyGroupID = InstanceEntry->SoundInstanceToConcurrencyGroup.Find(Sound->GetUniqueID()))
				{
					FAtomConcurrencyGroup* ConcurrencyGroup = CanPlaySound(NewActiveSound, *ConcurrencyGroupID, SoundsToEvict, bIsRetriggering, bIsRealizing, bOutEvictNew);
					if (!ConcurrencyGroup)
					{
						return false;
					}
					GroupsToApply.Add(ConcurrencyGroup);
				}
			}
		}
		break;

		case EAtomConcurrencyMode::Sound:
		{
			const FAtomSoundObjectId SoundObjectID = NewActiveSound.GetSound()->GetUniqueID();
			if (FAtomConcurrencyGroupId* ConcurrencyGroupID = SoundObjectToConcurrencyGroup.Find(SoundObjectID))
			{
				FAtomConcurrencyGroup* ConcurrencyGroup = CanPlaySound(NewActiveSound, *ConcurrencyGroupID, SoundsToEvict, bIsRetriggering, bIsRealizing, bOutEvictNew);
				if (!ConcurrencyGroup)
				{
					return false;
				}
				GroupsToApply.Add(ConcurrencyGroup);
			}
		}
		break;
		}
	}

	return true;
}

FAtomActiveSound* FAtomConcurrencyManager::CreateAndEvictActiveSounds(FAtomActiveSound& NewActiveSound, const TArray<FAtomConcurrencyGroup*>& GroupsToApply, const TArray<FAtomActiveSound*>& SoundsToEvict, bool bIsRealizing, bool bEvictNewActiveSound)
{
	FAtomActiveSound* ActiveSound = nullptr;
	if (!bIsRealizing)
	{
		// First make a new active sound
		ActiveSound = new FAtomActiveSound(NewActiveSound);
		ActiveSound->SetAtomRuntime(AtomRuntime);
	}
	else
	{
		// use the realized ActiveSound
		ActiveSound = &NewActiveSound;
	}

	check(AtomRuntime == ActiveSound->GetAtomRuntime());

#if ATOM_TRACE_CONCURRENCY
	UE_LOG(LogCriWareAtomConcurrency, Log, TEXT("CreateAndEvictActive %s realizing %d evicted %d"), *NewActiveSound.GetSound()->GetFName().ToString(), bIsRealizing, bEvictNewActiveSound);
	for (auto& EvictSound : SoundsToEvict)
	{
		UE_LOG(LogCriWareAtomConcurrency, Log, TEXT("-- evict %s"), *EvictSound->GetSound()->GetFName().ToString());
	}
#endif

	// If evicted do not add to concurency yet just create the active sound
	if (!bEvictNewActiveSound)
	{
		bool bTrackConcurrencyVolume = false;
		float FadeInDuration = -1.0f;
		for (FAtomConcurrencyGroup* ConcurrencyGroup : GroupsToApply)
		{
			check(ConcurrencyGroup);

			const FAtomConcurrencySettings& Settings = ConcurrencyGroup->GetSettings();

			FadeInDuration = FadeInDuration < 0.0f
				? Settings.VoiceStealAttackTime
				: FMath::Min(Settings.VoiceStealAttackTime, FadeInDuration);

			if (Settings.VolumeScaleMode == EAtomConcurrencyVolumeScaleMode::Default)
			{
				const float Volume = Settings.GetVolumeScale();
				if (!FMath::IsNearlyEqual(Volume, 1.0f))
				{
					check(Volume >= 0.0f);
					const int32 NextGeneration = ConcurrencyGroup->GetNextGeneration();

					// If we're ducking older sounds in the concurrency group, then loop through each sound in the concurrency group
					// and update their duck amount based on each sound's generation and the next generation count. The older the sound, the more ducking.
					const TArray<FAtomActiveSound*>& ActiveSounds = ConcurrencyGroup->GetActiveSounds();
					for (FAtomActiveSound* OtherSound : ActiveSounds)
					{
						check(OtherSound);

						FAtomConcurrencySoundData& OtherSoundData = OtherSound->ConcurrencyGroupData.FindChecked(ConcurrencyGroup->GetGroupID());
						const float ActiveSoundGeneration = static_cast<float>(OtherSoundData.Generation);
						const float GenerationDelta = NextGeneration - ActiveSoundGeneration;
						const float NewTargetVolume = FMath::Pow(Volume, GenerationDelta);

						// Don't allow volume to recover if release time is negative
						if (Settings.bVolumeScaleCanRelease && OtherSoundData.GetTargetVolume() < NewTargetVolume)
						{
							continue;
						}

						AtomConcurrency::SetSoundDataTarget(*OtherSound, OtherSoundData, NewTargetVolume, Settings.VolumeScaleAttackTime);
					}
				}
			}

			// Determine if we need to track concurrency volume on this active sound
			if (ConcurrencyGroup->GetSettings().ResolutionRule == EAtomMaxConcurrentResolutionRule::StopQuietest)
			{
				bTrackConcurrencyVolume = true;
			}

			// And add it to to the concurrency group.
			ConcurrencyGroup->AddActiveSound(*ActiveSound);
		}

		if (!bTrackConcurrencyVolume)
		{
			ActiveSound->VolumeConcurrency = -1.0f;
		}

		// recover volume
		if (ActiveSound->bHasVirtualized)
		{
			if (FadeInDuration > KINDA_SMALL_NUMBER)
			{
				float TargetVolume = ActiveSound->VolumeMultiplier * ActiveSound->GetSound()->GetVolumeMultiplier();
				ActiveSound->ComponentVolumeFader.SetVolume(0.0f);
				ActiveSound->ComponentVolumeFader.StartFade(TargetVolume, FadeInDuration, EAtomFaderCurve::Logarithmic);
				ActiveSound->ComponentVolumeFader.SetActiveDuration(FadeInDuration);
				ActiveSound->bIsRealizingWithFadeIn = true;
			}
			else
			{
				// recover fader if faded out by concurrency
				float FadeDuration = 0.0f;
				if (ActiveSound->GetConcurrencyFadeDuration(FadeDuration))
				{
					ActiveSound->ComponentVolumeFader.SetVolume(1.0f);
				}
			}
		}
	}
	else
	{
		bool bCanUseVirtualVoice = false;
		if (UAtomSoundBase* Sound = ActiveSound->GetSound())
		{
			const EAtomVirtualizationMode VirtualizationMode = Sound->GetVirtualizationMode();
			bCanUseVirtualVoice =
				(VirtualizationMode == EAtomVirtualizationMode::PlayWhenSilent) ||
				(VirtualizationMode == EAtomVirtualizationMode::Retrigger);
		}

		if (bCanUseVirtualVoice)
		{
			// Ask to ADX to handle virtualization
			ActiveSound->bVirtualizedDueToMaxConcurrency = true;
			ActiveSound->bIsPendingVirtualization = true;
		}
		else
		{
			ActiveSound->bShouldStopDueToMaxConcurrency = true;

			// simply fade out or mute by fader.
			float FadeOutDuration = 0.0f;
			bool bRequiresConcurrencyFade = FMath::IsNearlyZero(ActiveSound->PlaybackTime, 0.1f) && ActiveSound->GetConcurrencyFadeDuration(FadeOutDuration);
			if (bRequiresConcurrencyFade)
			{
				ActiveSound->FadeOut = FAtomActiveSound::EFadeOut::Concurrency;
				ActiveSound->ComponentVolumeFader.StartFade(0.0f, FadeOutDuration, EAtomFaderCurve::Logarithmic);
				ActiveSound->ComponentVolumeFader.SetActiveDuration(FadeOutDuration);
			}
		}
	}

	// Stop any sounds now if needed
	for (FAtomActiveSound* SoundToEvict : SoundsToEvict)
	{
		check(SoundToEvict);
		check(AtomRuntime == SoundToEvict->GetAtomRuntime());

		// Remove the active sound from the concurrency manager immediately so it doesn't count towards
		// subsequent concurrency resolution checks (i.e. if sounds are triggered multiple times in this frame)
		RemoveActiveSound(*SoundToEvict);

		if (SoundToEvict->FadeOut == FAtomActiveSound::EFadeOut::Concurrency)
		{
			SoundToEvict->bVirtualizedDueToMaxConcurrency = true;
			SoundToEvict->bIsPendingVirtualization = true;
			continue;
		}

		if (AtomRuntime->IsPendingStop(SoundToEvict))
		{
			continue;
		}

		if (SoundToEvict->GetSound())
		{
			UE_LOG(LogCriWareAtomConcurrency, VeryVerbose, TEXT("Evicting Sound %s due to concurrency"), *(SoundToEvict->GetSound()->GetName()));
		}

		StopDueToVoiceStealing(*SoundToEvict);
	}

	return ActiveSound;
}

void FAtomConcurrencyManager::ResetConcurrency()
{
	check(IsInAtomThread());

	for (auto& [ConcurrencyGroupID, ConcurrencyGroup] : ConcurrencyGroups)
	{
		for (auto& ActiveSound : ConcurrencyGroup->GetActiveSounds())
		{
			if (ActiveSound)
			{
				ActiveSound->ConcurrencyGroupData.Reset();
			}
		}
	}

	OwnerPerSoundConcurrencyMap.Reset();
	OwnerConcurrencyMap.Reset();
	SoundObjectToConcurrencyGroup.Reset();
	ConcurrencyMap.Reset();
	ConcurrencyGroups.Reset();
}

void FAtomConcurrencyManager::RemoveActiveSound(FAtomActiveSound& ActiveSound)
{
	check(IsInAtomThread());

	// Remove this sound from it's concurrency list
	for (const TPair<FAtomConcurrencyGroupId, FAtomConcurrencySoundData>& SoundDataPair : ActiveSound.ConcurrencyGroupData)
	{
		const FAtomConcurrencyGroupId ConcurrencyGroupID = SoundDataPair.Key;
		FAtomConcurrencyGroup* ConcurrencyGroup = ConcurrencyGroups.FindRef(ConcurrencyGroupID);
		if (!ConcurrencyGroup)
		{
			UE_LOG(LogCriWareAtomConcurrency, Error, TEXT("Attempting to remove stopped sound '%s' from inactive concurrency group."),
				ActiveSound.GetSound() ? *ActiveSound.GetSound()->GetName(): TEXT("Unset"));
			continue;
		}

		check(!ConcurrencyGroup->IsEmpty());
		ConcurrencyGroup->RemoveActiveSound(ActiveSound);

		// Don't delete the concurrency group state if there is a retrigger time set. This is so that 
		// state can persist past the last sound playing in the group
		if (ConcurrencyGroup->IsEmpty() && ConcurrencyGroup->GetSettings().RetriggerTime <= 0.0f)
		{
			// Get the object ID prior to removing from groups collection to avoid reading
			// from the object after its destroyed.
			const FAtomConcurrencyObjectId ConcurrencyObjectID = ConcurrencyGroup->GetObjectID();

			// Remove the object from the map.
			ConcurrencyGroups.Remove(ConcurrencyGroupID);

			// Remove from global group map if present.
			ConcurrencyMap.Remove(ConcurrencyObjectID);

			// Remove from sound object map if present.
			if (UAtomSoundBase* Sound = ActiveSound.GetSound())
			{
				const FAtomSoundOwnerObjectId ObjectID = Sound->GetUniqueID();
				SoundObjectToConcurrencyGroup.Remove(ObjectID);
			}

			// Remove from owner map if present.
			const uint32 OwnerID = ActiveSound.GetOwnerID();
			if (FAtomOwnerConcurrencyMapEntry* OwnerEntry = OwnerConcurrencyMap.Find(OwnerID))
			{
				if (OwnerEntry->ConcurrencyObjectToConcurrencyGroup.Remove(ConcurrencyObjectID))
				{
					if (OwnerEntry->ConcurrencyObjectToConcurrencyGroup.Num() == 0)
					{
						OwnerConcurrencyMap.Remove(OwnerID);
					}
				}
			}

			// Remove from owner per sound map if present.
			if (FAtomSoundInstanceEntry* InstanceEntry = OwnerPerSoundConcurrencyMap.Find(OwnerID))
			{
				if (UAtomSoundBase* Sound = ActiveSound.GetSound())
				{
					if (InstanceEntry->SoundInstanceToConcurrencyGroup.Remove(Sound->GetUniqueID()))
					{
						if (InstanceEntry->SoundInstanceToConcurrencyGroup.Num() == 0)
						{
							OwnerPerSoundConcurrencyMap.Remove(OwnerID);
						}
					}
				}
			}

			delete ConcurrencyGroup;
		}
	}

	ActiveSound.ConcurrencyGroupData.Reset();
}

void FAtomConcurrencyManager::StopDueToVoiceStealing(FAtomActiveSound& ActiveSound)
{
	FAtomRuntime* Runtime = ActiveSound.GetAtomRuntime();
	check(Runtime);

	float FadeOutDuration = 0.0f;
	bool bRequiresConcurrencyFade = ActiveSound.GetConcurrencyFadeDuration(FadeOutDuration);
	bool bCanUseVirtualVoice = false;
	bool bStoppedByMixerOnFade = false;

	if (UAtomSoundBase* Sound = ActiveSound.GetSound())
	{
		const EAtomVirtualizationMode VirtualizationMode = Sound->GetVirtualizationMode();
		bCanUseVirtualVoice = 
			(VirtualizationMode == EAtomVirtualizationMode::PlayWhenSilent) ||
			(VirtualizationMode == EAtomVirtualizationMode::Retrigger);
		bStoppedByMixerOnFade = VirtualizationMode == EAtomVirtualizationMode::StopWhenSilent;
	}

	if (bCanUseVirtualVoice)
	{
		// ask to Atom Sdk to handle virtualization
		ActiveSound.bVirtualizedDueToMaxConcurrency = true;
		ActiveSound.bIsPendingVirtualization = true;

		RemoveActiveSound(ActiveSound);
	}
	else
	{
		if (bRequiresConcurrencyFade)
		{
			if (bStoppedByMixerOnFade)
			{
				Runtime->UnlinkActiveSoundFromComponent(ActiveSound);
			}
		}
		else
		{
			Runtime->AddSoundToStop(&ActiveSound);
		}

		const bool bDoRangeCheck = false;
		FAtomVirtualLoop VirtualLoop;
		if (FAtomVirtualLoop::Virtualize(ActiveSound, bDoRangeCheck, VirtualLoop))
		{
			ActiveSound.ClearAtomComponent();

			if (UAtomSoundBase* Sound = ActiveSound.GetSound())
			{
				UE_LOG(LogCriWareAtomConcurrency, Verbose, TEXT("Playing ActiveSound %s Virtualizing: Sound's voice stollen due to concurrency group maximum met."), *Sound->GetName());
			}
			Runtime->AddVirtualLoop(VirtualLoop);
		}
	}

	// Apply concurrency fade after potentially virtualizing to avoid transferring undesired new concurrency fade state
	if (bRequiresConcurrencyFade)
	{
		ActiveSound.FadeOut = FAtomActiveSound::EFadeOut::Concurrency;
		ActiveSound.ComponentVolumeFader.StartFade(0.0f, FadeOutDuration, EAtomFaderCurve::Logarithmic);
		ActiveSound.ComponentVolumeFader.SetActiveDuration(FadeOutDuration);
	}
}

void FAtomConcurrencyManager::UpdateVolumeScaleGenerations()
{
	check(IsInAtomThread());

	for (auto& ConcurrencyGroupEntry : ConcurrencyGroups)
	{
		ConcurrencyGroupEntry.Value->UpdateGeneration();
	}
}

void FAtomConcurrencyManager::UpdateSoundsToCull()
{
	check(IsInAtomThread());

	for (auto& ConcurrenyGroupEntry : ConcurrencyGroups)
	{
		ConcurrenyGroupEntry.Value->CullSoundsDueToMaxConcurrency();
	}
}

#if !UE_BUILD_SHIPPING
	void FAtomConcurrencyManager::IterateOverConcurrencyGroups(TFunctionRef<void(FAtomConcurrencyGroup*)> Func) const
	{
		check(IsInAtomThread());

		for (auto& ConcurrenyGroupEntry : ConcurrencyGroups)
		{
			Func(ConcurrenyGroupEntry.Value);
		}
	}
#endif
