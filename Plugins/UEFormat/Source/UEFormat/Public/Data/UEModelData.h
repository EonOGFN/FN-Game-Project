#pragma once

#include "CoreMinimal.h"
#include "Math/Color.h"
#include "Math/Quat.h"
#include <string>

struct FVertexColorChunk
{
	std::string Name;
	int32 Count = 0;
	TArray<FColor> Data;
};

struct FWeightChunk
{
	short WeightBoneIndex = 0;
	int32 WeightVertexIndex = 0;
	float WeightAmount = 0.f;
};

struct FBoneChunk
{
	std::string BoneName;
	int32 BoneParentIndex = 0;
	FVector BonePos = FVector::ZeroVector;
	FQuat BoneRot = FQuat::Identity;
	FVector BoneScale = FVector(1.f, 1.f, 1.f);
};

struct FSocketChunk
{
	std::string SocketName;
	std::string SocketParentName;
	FVector SocketPos = FVector::ZeroVector;
	FQuat SocketRot = FQuat::Identity;
	FVector SocketScale = FVector(1.f, 1.f, 1.f);
};

struct FMaterialChunk
{
	std::string Name;
	std::string Path;
	int32 FirstIndex = 0;
	int32 NumFaces = 0;
};

struct FMorphTargetDataChunk
{
	FVector MorphPosition = FVector::ZeroVector;
	FVector MorphNormals = FVector::ZeroVector;
	int32 MorphVertexIndex = 0;
};

struct FMorphTargetChunk
{
	std::string MorphName;
	TArray<FMorphTargetDataChunk> MorphDeltas;
};

struct FVirtualBoneChunk
{
	std::string SourceBoneName;
	std::string TargetBoneName;
	std::string VirtualBoneName;
};

struct FLODData
{
	TArray<FVector> Vertices;
	TArray<int32> Indices;
	TArray<FVector4> Normals;
	TArray<FVector> Tangents;
	TArray<FVertexColorChunk> VertexColors;
	TArray<TArray<FVector2D>> TextureCoordinates;
	TArray<FMaterialChunk> Materials;
	TArray<FWeightChunk> Weights;
	TArray<FMorphTargetChunk> Morphs;
};

struct FSkeletonData
{
	std::string Path;
	TArray<FBoneChunk> Bones;
	TArray<FSocketChunk> Sockets;
	TArray<FVirtualBoneChunk> VirtualBones;
};
