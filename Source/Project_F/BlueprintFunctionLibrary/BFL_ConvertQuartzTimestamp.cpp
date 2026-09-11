// Fill out your copyright notice in the Description page of Project Settings.


#include "BFL_ConvertQuartzTimestamp.h"

float UBFL_ConvertQuartzTimestamp::ConvertQuartzTimestampToContinuousBeat(int InputNumBars, int InputBeat, float InputBeatFraction)
{
	float TotalBeats = (InputNumBars * 4) - (4 - InputBeat) + InputBeatFraction;
	return TotalBeats;
};

