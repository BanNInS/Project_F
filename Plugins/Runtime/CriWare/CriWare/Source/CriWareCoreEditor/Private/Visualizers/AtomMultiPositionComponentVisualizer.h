/****************************************************************************
 *
 * CRI Middleware SDK
 *
 * Copyright (c) 2021 CRI Middleware Co., Ltd.
 *
 * Library  : CRIWARE plugin for Unreal Engine
 * Module   : CriWareCore
 * File     : AtomMultiPositionComponentVisualizer.h
 *
 ****************************************************************************/

#pragma once

#include "Visualizers/AtomAttenuatedComponentVisualizer.h"

#include "Atom/AtomMultiPositionComponent.h"

class CRIWARECOREEDITOR_API FAtomMultiPositionComponentVisualizer
	: public TAtomAttenuatedComponentVisualizer<UAtomMultiPositionComponent>
{
private:

	virtual bool IsVisualizerEnabled(const FEngineShowFlags& ShowFlags) const override
	{
		return ShowFlags.AudioRadius;
	}

	//~ Begin FComponentVisualizer Interface
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override
	{
		if (IsVisualizerEnabled(View->Family->EngineShowFlags))
		{
			const UAtomMultiPositionComponent* MultiPositionComponent = Cast<UAtomMultiPositionComponent>(Component);
			if (MultiPositionComponent)
			{
				TMultiMap<EAtomAttenuationShape, FAtomAttenuationSettings::AttenuationShapeDetails> ShapeDetailsMap;
				MultiPositionComponent->CollectAttenuationShapesForVisualization(ShapeDetailsMap);

				if (MultiPositionComponent->MultiPositionSettings.bUseComponentPositionAsSource)
				{
					const FTransform& Transform = MultiPositionComponent->GetComponentTransform();
					DrawAttenuationVisualization(Transform, ShapeDetailsMap, View, PDI);
				}

				TArray<FTransform> Transforms;
				MultiPositionComponent->GetAllPositions(Transforms);

				for (auto& Transform : Transforms)
				{
					DrawAttenuationVisualization(Transform, ShapeDetailsMap, View, PDI);
				}
			}
		}
	}
	//~ End FComponentVisualizer Interface
};
