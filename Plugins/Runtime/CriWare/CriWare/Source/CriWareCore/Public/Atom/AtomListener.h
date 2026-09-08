/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomListener.h
 *
 ****************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "CriWareUtils.h"

#include "AtomVolume.h"

// Forward declarations.
class FAtomRuntime;
struct FAtomAttenuationSettings;

 /**
  * Defines the properties of the listener
  */
struct FAtomListener
{
public:

	FAtomListener(FAtomRuntime* InAtomRuntime);
	FAtomListener() = delete;

	/** Listener transform to axis vectors. */
	FVector GetUp() const { return Transform.GetUnitAxis(EAxis::Z); }
	FVector GetFront() const { return Transform.GetUnitAxis(EAxis::X); }
	FVector GetRight() const { return Transform.GetUnitAxis(EAxis::Y); }

	/**
	 * Gets the position of the listener
	 */
	FVector GetPosition(bool bAllowOverride) const;

	/**
	 * Works out the interp value between source and end
	 */
	float Interpolate(const double EndTime);
	void Update();

	/**
	 * Gets the current state of the interior settings for the listener
	 */
	//void UpdateCurrentInteriorSettings();

	/**
	 * Apply the interior settings to ambient sounds
	 */
	//void ApplyInteriorSettings(uint32 AtomVolumeID, const FInteriorSettings& Settings);

public:

	/** The runtime using this listener. */
	FAtomRuntime* AtomRuntime;

	FTransform Transform;
	FVector Velocity;

	/** An attenuation override to use for distance and attenuation calculations */
	FVector AttenuationOverride;

	/** Is our attenuation override active */
	uint32 bUseAttenuationOverride : 1;

	/** */
	//struct FAtomInteriorSettings InteriorSettings;

	/** The ID of the volume the listener resides in - deprecated? */
	//uint32 AtomVolumeID; // not used anymore

	/** The ID of the world the listener resides in */
	uint32 WorldID;

	/** Index of this listener inside the AudioDevice's listener array */
	int32 ListenerIndex;

	/** The times of interior volumes fading in and out */
	//double InteriorStartTime;
	//double InteriorEndTime;
	//double ExteriorEndTime;
	//double InteriorLPFEndTime;
	//double ExteriorLPFEndTime;
	//float InteriorVolumeInterp;
	//float InteriorLPFInterp;
	//float ExteriorVolumeInterp;
	//float ExteriorLPFInterp;

	/** Handle to Atom native 3d listener. */
	FCriAtomExListenerPtr ExListener;
};

/**
 * Game thread representation of a listener
 */
struct FAtomListenerProxy
{
	FTransform Transform;

	/** An attenuation override to use for distance and attenuation calculations */
	FVector AttenuationOverride;

	/** Is our attenuation override active */
	uint32 bUseAttenuationOverride : 1;

	/** The ID of the world the listener resides in */
	uint32 WorldID = INDEX_NONE;

	/**
	 * Gets the position of the listener proxy
	 */
	FVector GetPosition(bool bAllowOverride) const;

	FAtomListenerProxy()
		: bUseAttenuationOverride(false)
	{
	}

	FAtomListenerProxy(const FAtomListener& Listener)
		: Transform(Listener.Transform)
		, AttenuationOverride(Listener.AttenuationOverride)
		, bUseAttenuationOverride(Listener.bUseAttenuationOverride)
		, WorldID(Listener.WorldID)
	{
	}
};

/** 
 * Struct used to cache listener attenuation vector math results
 */
struct FAtomAttenuationListenerData
{
	FVector ListenerToSoundDir;
	FVector::FReal AttenuationDistance;
	FVector::FReal ListenerToSoundDistance;

	// Non-attenuation distance for calculating surround sound speaker maps for sources w/ spread
	FVector::FReal ListenerToSoundDistanceForPanning;

	FTransform ListenerTransform;
	const FTransform SoundTransform;
	const FAtomAttenuationSettings* AttenuationSettings;

	/** Computes and returns some geometry related to the listener and the given sound transform. */
	static FAtomAttenuationListenerData Create(const FAtomRuntime& AtomRuntime, int32 ListenerIndex, const FTransform& InSoundTransform, const FAtomAttenuationSettings& InAttenuationSettings);

private:
	FAtomAttenuationListenerData(const FTransform& InListenerTransform, const FTransform& InSoundTransform, const FAtomAttenuationSettings& InAttenuationSettings)
		: ListenerToSoundDir(FVector::ZeroVector)
		, AttenuationDistance(0.0f)
		, ListenerToSoundDistance(0.0f)
		, ListenerToSoundDistanceForPanning(0.0f)
		, ListenerTransform(InListenerTransform)
		, SoundTransform(InSoundTransform)
		, AttenuationSettings(&InAttenuationSettings)
	{
	}
};
