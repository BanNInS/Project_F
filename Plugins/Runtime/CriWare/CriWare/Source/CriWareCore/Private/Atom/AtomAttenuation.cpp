/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomAttenuation.cpp
 *
 ****************************************************************************/

#include "Atom/AtomAttenuation.h"

#include "CriWareCorePrivate.h"
#include "Atom/Atom.h"
#include "Atom/AtomRuntime.h"

#define LOCTEXT_NAMESPACE "AtomAttenuation"

namespace
{
	static const float MinAttenuationValue = 1.e-3f;
	static const float MinAttenuationValueDb = -60.0f;
} // namespace <>

/*
 * FAtomAttenuationSettings implementation
 *****************************************************************************/

FAtomAttenuationSettings::FAtomAttenuationSettings()
	: bAttenuate(true)
	, bSpatialize(true)
	, bAttenuateWithLPF(false)
	, bEnableListenerFocus(false)
	, bEnableFocusInterpolation(false)
	, bEnableOcclusion(false)
	, bUseComplexCollisionForOcclusion(false)
	//, bEnableReverbSend(true)
	, bEnablePriorityAttenuation(false)
	, bApplyNormalizationToStereoSounds(false)
	, bEnableLogFrequencyScaling(false)
	, bEnableBusSends(false)
	, bEnableSourceDataOverride(false)
	, DistanceAlgorithm(EAtomAttenuationDistanceModel::Logarithmic)
	, AttenuationShape(EAtomAttenuationShape::Sphere)
	, AbsorptionMethod(EAtomAirAbsorptionMethod::Linear)
	, SpatializationAlgorithm(EAtomSpatializationAlgorithm::Default)
	, OcclusionTraceChannel(ECC_Visibility)
	//, ReverbSendMethod(EReverbSendMethod::Linear)
	, PriorityAttenuationMethod(EAtomPriorityAttenuationMethod::Linear)
	, AttenuationShapeExtents(400.0f, 0.0f, 0.0f)
	//, ConeOffset(0.f)
	, ConeOutsideLevel(0.0f)
	, FalloffDistance(3600.0f)
	//, ConeSphereRadius(0.f)
	//, ConeSphereFalloffDistance(0.f)
#if WITH_EDITORONLY_DATA
	, OmniRadius_DEPRECATED(0.0f)
#endif
	, BinauralRadius(0.0f)
	, NonSpatializedRadiusStart(0.0f)
	, NonSpatializedRadiusEnd(0.0f)
	, NonSpatializedRadiusMode(EAtomNonSpatializedRadiusSpeakerMapMode::Direct2D)
	, StereoSpread(200.0f)
	, LPFRadiusMin(3000.f)
	, LPFRadiusMax(6000.f)
	, LPFFrequencyAtMin(ATOM_MAX_FILTER_FREQUENCY)
	, LPFFrequencyAtMax(ATOM_MAX_FILTER_FREQUENCY)
	, HPFFrequencyAtMin(ATOM_MIN_FILTER_FREQUENCY)
	, HPFFrequencyAtMax(ATOM_MIN_FILTER_FREQUENCY)
	, FocusAzimuth(30.0f)
	, NonFocusAzimuth(60.0f)
	, FocusDistanceScale(1.0f)
	, NonFocusDistanceScale(1.0f)
	, FocusPriorityScale(1.0f)
	, NonFocusPriorityScale(1.0f)
	, FocusVolumeAttenuation(1.0f)
	, NonFocusVolumeAttenuation(1.0f)
	, FocusAttackInterpSpeed(1.0f)
	, FocusReleaseInterpSpeed(1.0f)
	, OcclusionLowPassFilterFrequency(ATOM_MAX_FILTER_FREQUENCY)
	, OcclusionVolumeAttenuation(1.0f)
	, OcclusionInterpolationTime(0.1f)
	//, ReverbWetLevelMin(0.3f)
	//, ReverbWetLevelMax(0.95f)
	//, ReverbDistanceMin(UE_REAL_TO_FLOAT(AttenuationShapeExtents.X))
	//, ReverbDistanceMax(UE_REAL_TO_FLOAT(AttenuationShapeExtents.X) + FalloffDistance)
	//, ManualReverbSendLevel(0.0f)
	, PriorityAttenuationMin(1.0f)
	, PriorityAttenuationMax(1.0f)
	, PriorityAttenuationDistanceMin(UE_REAL_TO_FLOAT(AttenuationShapeExtents.X))
	, PriorityAttenuationDistanceMax(UE_REAL_TO_FLOAT(AttenuationShapeExtents.X) + FalloffDistance)
	, ManualPriorityAttenuation(1.0f)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCSTRUCT(StaticStruct());
#endif
}

bool FAtomAttenuationSettings::operator==(const FAtomAttenuationSettings& Other) const
{
	return (   bAttenuate				== Other.bAttenuate
			&& bSpatialize				== Other.bSpatialize
			//&& dBAttenuationAtMax		== Other.dBAttenuationAtMax
			//&& FalloffMode			== Other.FalloffMode
			&& NonSpatializedRadiusStart == Other.NonSpatializedRadiusStart
			&& NonSpatializedRadiusEnd   == Other.NonSpatializedRadiusEnd
			&& NonSpatializedRadiusMode  == Other.NonSpatializedRadiusMode
			&& bApplyNormalizationToStereoSounds == Other.bApplyNormalizationToStereoSounds
			&& StereoSpread				== Other.StereoSpread
			&& DistanceAlgorithm		== Other.DistanceAlgorithm
			&& AttenuationShape			== Other.AttenuationShape
			&& bAttenuateWithLPF		== Other.bAttenuateWithLPF
			&& LPFRadiusMax				== Other.LPFRadiusMin
			&& LPFRadiusMin				== Other.LPFRadiusMax
			&& FalloffDistance			== Other.FalloffDistance
			&& AttenuationShapeExtents	== Other.AttenuationShapeExtents
			&& SpatializationAlgorithm	== Other.SpatializationAlgorithm
			//&& PluginSettings.SpatializationPluginSettingsArray == Other.PluginSettings.SpatializationPluginSettingsArray
			&& LPFFrequencyAtMax		== Other.LPFFrequencyAtMax
			&& LPFFrequencyAtMin		== Other.LPFFrequencyAtMin
			&& HPFFrequencyAtMax		== Other.HPFFrequencyAtMax
			&& HPFFrequencyAtMin		== Other.HPFFrequencyAtMin
			&& bEnableLogFrequencyScaling == Other.bEnableLogFrequencyScaling
			&& bEnableBusSends			== Other.bEnableBusSends
			&& bEnableListenerFocus		== Other.bEnableListenerFocus
			&& FocusAzimuth			== Other.FocusAzimuth
			&& NonFocusAzimuth		== Other.NonFocusAzimuth
			&& FocusDistanceScale		== Other.FocusDistanceScale
			&& FocusPriorityScale		== Other.FocusPriorityScale
			&& NonFocusPriorityScale	== Other.NonFocusPriorityScale
			&& FocusVolumeAttenuation	== Other.FocusVolumeAttenuation
			&& NonFocusVolumeAttenuation == Other.NonFocusVolumeAttenuation
			&& OcclusionTraceChannel	== Other.OcclusionTraceChannel
			&& OcclusionLowPassFilterFrequency == Other.OcclusionLowPassFilterFrequency
			&& OcclusionVolumeAttenuation == Other.OcclusionVolumeAttenuation
			&& OcclusionInterpolationTime == Other.OcclusionInterpolationTime
			//&& PluginSettings.OcclusionPluginSettingsArray == Other.PluginSettings.OcclusionPluginSettingsArray
			//&& bEnableReverbSend		== Other.bEnableReverbSend
			//&& PluginSettings.ReverbPluginSettingsArray == Other.PluginSettings.ReverbPluginSettingsArray
			&& PluginSettings.SourceDataOverridePluginSettingsArray == Other.PluginSettings.SourceDataOverridePluginSettingsArray
			&& PluginSettings.RuntimePluginSettingsArray == Other.PluginSettings.RuntimePluginSettingsArray
			//&& ReverbWetLevelMin		== Other.ReverbWetLevelMin
			//&& ReverbWetLevelMax		== Other.ReverbWetLevelMax
			//&& ReverbDistanceMin		== Other.ReverbDistanceMin
			//&& ReverbDistanceMax		== Other.ReverbDistanceMax
		);
}

#if WITH_EDITORONLY_DATA
void FAtomAttenuationSettings::PostSerialize(const FArchive& Ar)
{
}
#endif

float FAtomAttenuationSettings::GetMinDimension() const
{
	float MinDimension = 0.0f;

	switch (AttenuationShape)
	{
	case EAtomAttenuationShape::Sphere:
	case EAtomAttenuationShape::Cone:

		MinDimension += AttenuationShapeExtents.X;
		break;

	default:
		
		check(false);
	}

	return FMath::Clamp(MinDimension, 0.0f, static_cast<float>(FAtomRuntime::GetMaxWorldDistance()));
}

float FAtomAttenuationSettings::GetMaxDimension() const
{
	float MaxDimension = GetMinDimension() + GetMaxFalloffDistance();

	return FMath::Clamp(MaxDimension, 0.0f, static_cast<float>(FAtomRuntime::GetMaxWorldDistance()));
}

float FAtomAttenuationSettings::GetMaxFalloffDistance() const
{
	static const float WorldMax = static_cast<float>(FAtomRuntime::GetMaxWorldDistance());
	if (FalloffDistance > WorldMax)
	{
		return WorldMax;
	}

	return FalloffDistance;
}

float FAtomAttenuationSettings::GetConeAnglesAndOutsideLevel(float& InnerAngle, float& OuterAngle) const
{
	// Cone - X is Cone Radius, Y is Cone Angle, Z is Cone Falloff Angle
	//const float ConeRadius = AttenuationShapeExtents.X; // inner radius -> inner cone height (min distance)
	const float ConeAngle = AttenuationShapeExtents.Y; // inner angle
	const float ConeFalloffAngle = AttenuationShapeExtents.Z; // outer falloff angle   

	InnerAngle = ConeAngle;
	OuterAngle = ConeAngle + ConeFalloffAngle;

	//ConeOffset; // unusable by atom 
	//ConeSphereRadius; // unusable by atom 
	//ConeSphereFalloffDistance; // unusable by atom

	return ConeOutsideLevel;
}

void FAtomAttenuationSettings::CollectAttenuationShapesForVisualization(TMultiMap<EAtomAttenuationShape, AttenuationShapeDetails>& ShapeDetailsMap) const
{
	if (bAttenuate)
	{
		AttenuationShapeDetails ShapeDetails;
		ShapeDetails.Extents = AttenuationShapeExtents;
		ShapeDetails.Falloff = FalloffDistance;
		//ShapeDetails.ConeOffset = ConeOffset;
		//ShapeDetails.ConeSphereRadius = ConeSphereRadius;
		ShapeDetails.ConeSphereRadius = GetMinDimension();
		//ShapeDetails.ConeSphereFalloff = ConeSphereFalloffDistance;
		ShapeDetails.ConeSphereFalloff = GetMaxFalloffDistance();

		ShapeDetailsMap.Add(AttenuationShape, MoveTemp(ShapeDetails));
	}
}

float FAtomAttenuationSettings::GetFocusPriorityScale(const struct FAtomGlobalFocusSettings& FocusSettings, float FocusFactor) const
{
	float Focus = FocusSettings.FocusPriorityScale * FocusPriorityScale;
	float NonFocus = FocusSettings.NonFocusPriorityScale * NonFocusPriorityScale;
	float Result = FMath::Lerp(Focus, NonFocus, FocusFactor);
	return FMath::Max(0.0f, Result);
}

float FAtomAttenuationSettings::GetFocusAttenuation(const struct FAtomGlobalFocusSettings& FocusSettings, float FocusFactor) const
{
	float Focus = FocusSettings.FocusVolumeScale * FocusVolumeAttenuation;
	float NonFocus = FocusSettings.NonFocusVolumeScale * NonFocusVolumeAttenuation;
	float Result = FMath::Lerp(Focus, NonFocus, FocusFactor);
	return FMath::Max(0.0f, Result);
}

float FAtomAttenuationSettings::GetFocusDistanceScale(const struct FAtomGlobalFocusSettings& FocusSettings, float FocusFactor) const
{
	float Focus = FocusSettings.FocusDistanceScale * FocusDistanceScale;
	float NonFocus = FocusSettings.NonFocusDistanceScale * NonFocusDistanceScale;
	float Result = FMath::Lerp(Focus, NonFocus, FocusFactor);
	return FMath::Max(0.0f, Result);
}

/* 
 * UAtomAttenuation implementation 
 *****************************************************************************/

UAtomAttenuation::UAtomAttenuation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	// Localization of unreal properties metadata with LOCTEXT markups and reflection
	CRI_LOCCLASS(GetClass());
#endif
}

#undef LOCTEXT_NAMESPACE
