/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomListener.cpp
 *
 ****************************************************************************/

#include "Atom/AtomListener.h"

#include "CriWareCorePrivate.h"
#include "CriWareApi.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"
#include "Atom/AtomAttenuation.h"

FAtomListener::FAtomListener(FAtomRuntime* InAtomRuntime)
	: AtomRuntime(InAtomRuntime)
	, Transform(FTransform::Identity)
	, Velocity(ForceInit)
	, AttenuationOverride(ForceInit)
	, bUseAttenuationOverride(false)
	//, AtomVolumeID(0)
	, ListenerIndex(0)
	//, InteriorStartTime(0.0)
	//, InteriorEndTime(0.0)
	//, ExteriorEndTime(0.0)
	//, InteriorLPFEndTime(0.0)
	//, ExteriorLPFEndTime(0.0)
	//, InteriorVolumeInterp(0.0f)
	//, InteriorLPFInterp(0.0f)
	//, ExteriorVolumeInterp(0.0f)
	//, ExteriorLPFInterp(0.0f)
{
	CriAtomEx3dListenerConfig Config;
	criAtomEx3dListener_SetDefaultConfig(&Config);

	ExListener = MakeCriHandle(FCriWareApi::criAtomEx3dListener_Create(&Config, nullptr, 0));
}

FVector FAtomListener::GetPosition(bool bAllowOverride) const
{
	if (bAllowOverride && bUseAttenuationOverride)
	{
		return AttenuationOverride;
	}

	return Transform.GetTranslation();
}

float FAtomListener::Interpolate(const double EndTime)
{
	/*if (FApp::GetCurrentTime() < InteriorStartTime)
	{
		return 0.0f;
	}

	if (FApp::GetCurrentTime() >= EndTime)
	{
		return 1.0f;
	}

	float InterpValue = (float)((FApp::GetCurrentTime() - InteriorStartTime) / (EndTime - InteriorStartTime));
	return FMath::Clamp(InterpValue, 0.0f, 1.0f);*/

	return 0.0f;
}

void FAtomListener::Update()
{
	if (ExListener.IsValid())
	{
		const float DistanceFactor = 0.01f;

		auto Pos = ToCriAtomExVector(Transform.GetTranslation() * DistanceFactor);
		auto Front = ToCriAtomExVector(GetFront());
		auto Up = ToCriAtomExVector(GetUp());
		auto Vel = ToCriAtomExVector(Velocity * DistanceFactor);

		//UE_LOG(LogTemp, Warning, TEXT("Listener SetPos(%f, %f, %f)"), Pos.x, Pos.y, Pos.z);
		//UE_LOG(LogTemp, Warning, TEXT("Listener SetOri(%f, %f, %f | %f, %f, %f)"), Front.x, Front.y, Front.z, Up.x, Up.y, Up.z);
		//UE_LOG(LogTemp, Warning, TEXT("Listener SetVel(%f, %f, %f)"), Vel.x, Vel.y, Vel.z);

		FCriWareApi::criAtomEx3dListener_SetPosition(ExListener, &Pos);
		FCriWareApi::criAtomEx3dListener_SetOrientation(ExListener, &Front, &Up);
		FCriWareApi::criAtomEx3dListener_SetVelocity(ExListener, &Vel);

		// focus point
		if (bUseAttenuationOverride)
		{
			auto FocusPos = ToCriAtomExVector(AttenuationOverride * DistanceFactor);
			FCriWareApi::criAtomEx3dListener_SetFocusPoint(ExListener, &FocusPos);
			FCriWareApi::criAtomEx3dListener_SetDistanceFocusLevel(ExListener, 1.0f);
		}
		else
		{
			FCriWareApi::criAtomEx3dListener_SetDistanceFocusLevel(ExListener, 0.0f);
		}

		// Advanced Utility functions for test (can be reproduced by user with simple UE4 3d functions)
		// lerp between listener and focus point. [L0.0 -> 1.0FP]
		//FCriWareApi::criAtomEx3dListener_SetDistanceFocusLevel(ExListener, float Level);
		// slerp on angle made by Listener between Source and focus point.  
		//FCriWareApi::criAtomEx3dListener_SetDirectionFocusLevel(ExListener, float Level);

		// listerner - doppler fx
		//FCriWareApi::criAtomEx3dListener_SetDopplerMultiplier(ExListener, float Mul);

		FCriWareApi::criAtomEx3dListener_Update(ExListener);
	}
}

FVector FAtomListenerProxy::GetPosition(bool bAllowOverride) const
{
	if (bAllowOverride && bUseAttenuationOverride)
	{
		return AttenuationOverride;
	}

	return Transform.GetTranslation();
}

FAtomAttenuationListenerData FAtomAttenuationListenerData::Create(const FAtomRuntime& AtomRuntime, int32 ListenerIndex, const FTransform& InSoundTransform, const FAtomAttenuationSettings& InAttenuationSettings)
{
	FTransform ListenerTransform;
	AtomRuntime.GetListenerTransform(ListenerIndex, ListenerTransform);

	FAtomAttenuationListenerData ListenerData(ListenerTransform, InSoundTransform, InAttenuationSettings);

	const FVector SoundTranslation = InSoundTransform.GetTranslation();
	FVector ListenerToSound = SoundTranslation - ListenerTransform.GetTranslation();
	ListenerToSound.ToDirectionAndLength(ListenerData.ListenerToSoundDir, ListenerData.ListenerToSoundDistance);

	// Store the actual distance for surround-panning sources with spread
	ListenerData.ListenerToSoundDistanceForPanning = ListenerData.ListenerToSoundDistance;

	// Calculating override listener-to-sound distance and transform must
	// be applied after distance used for panning value is calculated.
	FVector ListenerPosition;
	const bool bAllowAttenuationOverride = true;
	if (AtomRuntime.GetListenerPosition(ListenerIndex, ListenerPosition, bAllowAttenuationOverride))
	{
		ListenerData.ListenerToSoundDistance = (SoundTranslation - ListenerPosition).Size();
		ListenerData.ListenerTransform.SetTranslation(ListenerPosition);
	}

	const FAtomAttenuationSettings& AttenuationSettings = *ListenerData.AttenuationSettings;
	if ((AttenuationSettings.bAttenuate && AttenuationSettings.AttenuationShape == EAtomAttenuationShape::Sphere) || AttenuationSettings.bAttenuateWithLPF)
	{
		ListenerData.AttenuationDistance = FMath::Max<float>(ListenerData.ListenerToSoundDistance - AttenuationSettings.AttenuationShapeExtents.X, 0.0f);
	}

	return MoveTemp(ListenerData);
}
