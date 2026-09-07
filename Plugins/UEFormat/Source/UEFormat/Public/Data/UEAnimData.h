#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimTypes.h"
#include "Math/Quat.h"
#include <string>

struct FFloatKey
{
	int32 Frame = 0;
	float FloatValue = 0.f;
};

struct FVectorKey
{
	int32 Frame = 0;
	FVector VectorValue = FVector::ZeroVector;
};

struct FQuatKey
{
	int32 Frame = 0;
	FQuat QuatValue = FQuat::Identity;
};

struct FCurve
{
	std::string CurveName;
	TArray<FFloatKey> CurveKeys;
};

struct FTrack
{
	std::string TrackName;
	TArray<FVectorKey> TrackPosKeys;
	TArray<FQuatKey> TrackRotKeys;
	TArray<FVectorKey> TrackScaleKeys;
};

struct FAnimData
{
	int32 NumFrames = 0;
	float FramesPerSecond = 0.f;
	std::string RefPosePath;
	EAdditiveAnimationType AdditiveAnimType = AAT_None;
	EAdditiveBasePoseType RefPoseType = ABPT_None;
	int32 RefFrameIndex = 0;
	TArray<FTrack> Tracks;
	TArray<FCurve> Curves;
};
