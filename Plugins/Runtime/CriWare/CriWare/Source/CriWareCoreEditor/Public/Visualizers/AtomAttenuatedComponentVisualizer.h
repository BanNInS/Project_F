#pragma once

#include "ComponentVisualizer.h"
#include "ShowFlags.h"
#include "SceneView.h"
#include "SceneManagement.h"

#include "Atom/AtomAttenuation.h"

template<class T>
class TAtomAttenuatedComponentVisualizer
	: public FComponentVisualizer
{
public:

	//~ Begin FComponentVisualizer Interface
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override
	{
		if (IsVisualizerEnabled(View->Family->EngineShowFlags))
		{
			const T* AttenuatedComponent = Cast<const T>(Component);
			if (AttenuatedComponent != nullptr)
			{
				const FTransform& Transform = AttenuatedComponent->GetComponentTransform();

				TMultiMap<EAtomAttenuationShape, FAtomAttenuationSettings::AttenuationShapeDetails> ShapeDetailsMap;
				AttenuatedComponent->CollectAttenuationShapesForVisualization(ShapeDetailsMap);

				DrawAttenuationVisualization(Transform, ShapeDetailsMap, View, PDI);
			}
		}
	}
	//~ End FComponentVisualizer Interface

protected:

	void DrawAttenuationVisualization(const FTransform& Transform, const TMultiMap<EAtomAttenuationShape, FAtomAttenuationSettings::AttenuationShapeDetails>& ShapeDetailsMap, const FSceneView* View, FPrimitiveDrawInterface* PDI)
	{
		const FVector Translation = Transform.GetTranslation();
		const FVector UnitXAxis = Transform.GetUnitAxis(EAxis::X);
		const FVector UnitYAxis = Transform.GetUnitAxis(EAxis::Y);
		const FVector UnitZAxis = Transform.GetUnitAxis(EAxis::Z);

		for (const TPair<EAtomAttenuationShape, FAtomAttenuationSettings::AttenuationShapeDetails>& ShapeDetailsPair : ShapeDetailsMap)
		{
			const FColor OuterRadiusColor(255, 153, 0);
			const FColor InnerRadiusColor(216, 130, 0);

			const FAtomAttenuationSettings::AttenuationShapeDetails& ShapeDetails = ShapeDetailsPair.Value;
			switch (ShapeDetailsPair.Key)
			{

			case EAtomAttenuationShape::Cone:
			{
				FTransform Origin = Transform;
				Origin.SetScale3D(FVector(1.f));
				//Origin.SetTranslation(Translation - (UnitXAxis * ShapeDetails.ConeOffset));
				Origin.SetTranslation(Translation);

				if (ShapeDetails.Falloff > 0.f || ShapeDetails.Extents.Z > 0.f)
				{
					//float ConeRadius = ShapeDetails.Extents.X + ShapeDetails.Falloff + ShapeDetails.ConeOffset;
					float ConeRadius = ShapeDetails.Extents.X + ShapeDetails.Falloff;
					float ConeAngle = ShapeDetails.Extents.Y + ShapeDetails.Extents.Z;
					DrawWireSphereCappedCone(PDI, Origin, ConeRadius, ConeAngle, 16, 4, 10, OuterRadiusColor, SDPG_World);

					//ConeRadius = ShapeDetails.Extents.X + ShapeDetails.ConeOffset;
					ConeRadius = ShapeDetails.Extents.X;
					ConeAngle = ShapeDetails.Extents.Y;
					DrawWireSphereCappedCone(PDI, Origin, ConeRadius, ConeAngle, 16, 4, 10, InnerRadiusColor, SDPG_World);
				}
				else
				{
					//const float ConeRadius = ShapeDetails.Extents.X + ShapeDetails.ConeOffset;
					const float ConeRadius = ShapeDetails.Extents.X;
					const float ConeAngle = ShapeDetails.Extents.Y;
					DrawWireSphereCappedCone(PDI, Origin, ConeRadius, ConeAngle, 16, 4, 10, OuterRadiusColor, SDPG_World);
				}

				if (!FMath::IsNearlyZero(ShapeDetails.ConeSphereRadius, KINDA_SMALL_NUMBER))
				{
					if (ShapeDetails.ConeSphereFalloff > 0.f)
					{
						DrawWireSphereAutoSides(PDI, Origin, OuterRadiusColor, ShapeDetails.ConeSphereRadius + ShapeDetails.ConeSphereFalloff, SDPG_World);
						DrawWireSphereAutoSides(PDI, Origin, InnerRadiusColor, ShapeDetails.ConeSphereRadius, SDPG_World);
					}
					else
					{
						DrawWireSphereAutoSides(PDI, Origin, OuterRadiusColor, ShapeDetails.ConeSphereRadius, SDPG_World);
					}
				}
			}
			break;

			case EAtomAttenuationShape::Sphere:

				if (ShapeDetails.Falloff > 0.f)
				{
					DrawWireSphereAutoSides(PDI, Translation, OuterRadiusColor, ShapeDetails.Extents.X + ShapeDetails.Falloff, SDPG_World);
					DrawWireSphereAutoSides(PDI, Translation, InnerRadiusColor, ShapeDetails.Extents.X, SDPG_World);
				}
				else
				{
					DrawWireSphereAutoSides(PDI, Translation, OuterRadiusColor, ShapeDetails.Extents.X, SDPG_World);
				}
				break;

			default:
				check(false);
			}
		}
	}

private:

	virtual bool IsVisualizerEnabled(const FEngineShowFlags& ShowFlags) const = 0;
};
