
#include "SoundAttenuationVisualizer.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

#include "Atom/AtomSoundBase.h"

namespace Atom::Insights
{
	FSoundAttenuationVisualizer::FSoundAttenuationVisualizer(const FColor& InColor)
		: Color(InColor)
	{
		
	}
	
	void FSoundAttenuationVisualizer::Draw(float InDeltaTime, const FTransform& InTransform, const UObject& InObject, const UWorld& InWorld) const
	{
		if (LastObjectId != InObject.GetUniqueID())
		{
			ShapeDetailsMap.Reset();
			
			if (const UAtomSoundBase* SoundBase = Cast<UAtomSoundBase>(&InObject))
			{
				if (const FAtomAttenuationSettings* Settings = SoundBase->GetAttenuationSettingsToApply())
				{
					Settings->CollectAttenuationShapesForVisualization(ShapeDetailsMap);
				}
			}
			else
			{
				return;
			}
		}
		
		LastObjectId = InObject.GetUniqueID();

		for (const auto& [AttenuationShape, ShapeDetails] : ShapeDetailsMap)
		{
			switch (AttenuationShape)
			{
				case EAtomAttenuationShape::Sphere:
				{
					if (ShapeDetails.Falloff > 0.f)
					{
						DrawDebugSphere(&InWorld, InTransform.GetTranslation(), ShapeDetails.Extents.X + ShapeDetails.Falloff, 10, Color);
						DrawDebugSphere(&InWorld, InTransform.GetTranslation(), ShapeDetails.Extents.X, 10, Color);
					}
					else
					{
						DrawDebugSphere(&InWorld, InTransform.GetTranslation(), ShapeDetails.Extents.X, 10, Color);
					}
					break;
				}

				/*case EAtomAttenuationShape::Box:
				{
					if (ShapeDetails.Falloff > 0.f)
					{
						DrawDebugBox(&InWorld, InTransform.GetTranslation(), ShapeDetails.Extents + FVector(ShapeDetails.Falloff), InTransform.GetRotation(), Color);
						DrawDebugBox(&InWorld, InTransform.GetTranslation(), ShapeDetails.Extents, InTransform.GetRotation(), Color);
					}
					else
					{
						DrawDebugBox(&InWorld, InTransform.GetTranslation(), ShapeDetails.Extents, InTransform.GetRotation(), Color);
					}
					break;
				}*/

				/*case EAtomAttenuationShape::Capsule:
				{
					if (ShapeDetails.Falloff > 0.f)
					{
						DrawDebugCapsule(&InWorld, InTransform.GetTranslation(), ShapeDetails.Extents.X + ShapeDetails.Falloff, ShapeDetails.Extents.Y + ShapeDetails.Falloff, InTransform.GetRotation(), Color);
						DrawDebugCapsule(&InWorld, InTransform.GetTranslation(), ShapeDetails.Extents.X, ShapeDetails.Extents.Y, InTransform.GetRotation(), Color);
					}
					else
					{
						DrawDebugCapsule(&InWorld, InTransform.GetTranslation(), ShapeDetails.Extents.X, ShapeDetails.Extents.Y, InTransform.GetRotation(), Color);
					}
					break;
				}*/

				case EAtomAttenuationShape::Cone:
				{
					const FVector Origin = InTransform.GetTranslation() - (InTransform.GetUnitAxis(EAxis::X));

					if (ShapeDetails.Falloff > 0.f || ShapeDetails.Extents.Z > 0.f)
					{
						const float OuterAngle = FMath::DegreesToRadians(ShapeDetails.Extents.Y + ShapeDetails.Extents.Z);
						const float InnerAngle = FMath::DegreesToRadians(ShapeDetails.Extents.Y);
						DrawDebugCone(&InWorld, Origin, InTransform.GetUnitAxis(EAxis::X), ShapeDetails.Extents.X + ShapeDetails.ConeSphereFalloff, OuterAngle, OuterAngle, 10, Color);
						DrawDebugCone(&InWorld, Origin, InTransform.GetUnitAxis(EAxis::X), ShapeDetails.Extents.X, InnerAngle, InnerAngle, 10, Color);
					}
					else
					{
						const float Angle = FMath::DegreesToRadians(ShapeDetails.Extents.Y);
						DrawDebugCone(&InWorld, Origin, InTransform.GetUnitAxis(EAxis::X), ShapeDetails.Extents.X, Angle, Angle, 10, Color);
					}

					if (!FMath::IsNearlyZero(ShapeDetails.ConeSphereRadius, UE_KINDA_SMALL_NUMBER))
					{
						if (ShapeDetails.ConeSphereFalloff > 0.f)
						{

							DrawDebugSphere(&InWorld, Origin, ShapeDetails.ConeSphereRadius + ShapeDetails.ConeSphereFalloff, 10, Color);
							DrawDebugSphere(&InWorld, Origin, ShapeDetails.ConeSphereRadius, 10, Color);
						}
						else
						{
							DrawDebugSphere(&InWorld, Origin, ShapeDetails.ConeSphereRadius, 10, Color);
						}
					}

					break;
				}

				default:
				{
					break;
				}
			}
		}
	}
} // namespace
