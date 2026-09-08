/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomSoundWave.cpp
 *
 ****************************************************************************/

#include "Atom/AtomSoundWave.h"

#include "CriWareCorePrivate.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomActiveSound.h"
#include "Atom/AtomWaveBankResource.h"
#include "Atom/AtomWaveBank.h"

#define LOCTEXT_NAMESPACE "AtomSoundWave"

 /*
  * UAtomSoundWave class
  *****************************************************************************/

UAtomSoundWave::UAtomSoundWave(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, WaveBank(nullptr)
	, WaveID(0)
	, bLooping(false)
	, bOverrideAttenuation(false)
	, Volume(1.0f)
	, Pitch(1.0f)
{
#if WITH_EDITORONLY_DATA
	SoundColor = FColor(70, 52, 235); // Atom::DefaultSoundWaveColor;
#endif

#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

void UAtomSoundWave::PostLoad()
{
	Super::PostLoad();

	CacheAggregateValues();
}

bool UAtomSoundWave::Rename(const TCHAR* NewName, UObject* NewOuter, ERenameFlags Flags)
{
	return Super::Rename(NewName, NewOuter, Flags);
}

void UAtomSoundWave::Init(UAtomWaveBank* InWaveBank, uint32 InWaveID)
{
	WaveBank = InWaveBank;
	WaveID = InWaveID;

	CacheAggregateValues();
}

#if WITH_EDITOR
void UAtomSoundWave::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    if (PropertyChangedEvent.Property)
    {
        for (TObjectIterator<UAtomComponent> It; It; ++It)
        {
            if (It->Sound == this && It->IsActive())
            {
                // Allow attenuation overrides not update without stopping
                if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_STRING_CHECKED(UAtomSoundWave, AttenuationOverrides))
                {
                    It->SetAttenuationOverrides(AttenuationOverrides);
                }
                else if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_STRING_CHECKED(UAtomSoundWave, bOverrideAttenuation))
                {
                    It->SetAttenuationOverrides(AttenuationOverrides);
                    It->SetOverrideAttenuation(bOverrideAttenuation);
                }
                else
                {
                    It->Stop();
                    It->Play();
                }

            }
        }
    }

    CacheAggregateValues();
}
#endif

void UAtomSoundWave::CacheAggregateValues()
{
	Duration = WaveInfo.GetDuration();
}

bool UAtomSoundWave::IsPlayable() const
{
	return true;
}

bool UAtomSoundWave::IsStreaming(const TCHAR* PlatformName /* = nullptr */) const
{ 
	return WaveBank ? WaveBank->IsStreaming(PlatformName) : false;
}

float UAtomSoundWave::GetMaxDistance() const
{
	if (auto Settings = GetAttenuationSettingsToApply())
	{
		if (Settings->bAttenuate)
		{
			return Settings->GetMaxDimension();
		}
	}

	return WORLD_MAX;
}

float UAtomSoundWave::GetDuration() const
{
	return (bLooping ? ATOM_INDEFINITELY_LOOPING_DURATION : Duration);
}

float UAtomSoundWave::GetVolumeMultiplier()
{
	return Volume;
}

float UAtomSoundWave::GetPitchMultiplier()
{
	return Pitch;
}

const FAtomAttenuationSettings* UAtomSoundWave::GetAttenuationSettingsToApply() const
{
	if (bOverrideAttenuation)
	{
		return &AttenuationOverrides;
	}

	return Super::GetAttenuationSettingsToApply();
}

IAtomSoundResource* UAtomSoundWave::GetSoundResource()
{ 
	return WaveBank;
}

void UAtomSoundWave::GetAllSoundResources(TArray<IAtomSoundResource*>& Resources)
{
	Resources.Add(WaveBank);
}

FString UAtomSoundWave::GetSoundName() const
{
	return WaveBank->GetName() + TEXT(":") + GetName();
}

#if WITH_EDITOR
bool UAtomSoundWave::GetWaveInfo(FAtomResourceHandle& InHandle, FAtomWaveInfo& OutWaveInfo) const
{
	if (InHandle.IsValid() && InHandle->GetWaveBankResource())
	{
		return InHandle->GetWaveBankResource()->GetWaveformInfo(WaveID, OutWaveInfo);
	}
	return false;
}
#endif

#undef LOCTEXT_NAMESPACE
