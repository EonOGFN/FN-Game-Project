#include "Factories/UEFModelFactory.h"
#include "StaticMeshAttributes.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/MorphTarget.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "MeshUtilities.h"

UEFModelFactory::UEFModelFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Formats.Add(TEXT("uemodel; UEMODEL Mesh File"));
	SupportedClass = UObject::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UEFModelFactory::FactoryCreateFile(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, const FString& Filename, const TCHAR* Params, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UEFModelReader Data = UEFModelReader(Filename);
	if (!Data.Read() || Data.LODs.Num() == 0)
		return nullptr;

	if (Data.Skeleton.Bones.Num() > 0)
	{
		USkeletalMesh* SkeletalMesh = CreateSkeletalMesh(Data.LODs, Data.Skeleton, Parent, Name, Flags);

		SkeletalMesh->PostEditChange();
		FAssetRegistryModule::AssetCreated(SkeletalMesh);
		return SkeletalMesh;
	}
	else
	{
		UStaticMesh* StaticMesh = CreateStaticMesh(Data.LODs, Parent, Name, Flags);

		StaticMesh->PostEditChange();
		FAssetRegistryModule::AssetCreated(StaticMesh);
		return StaticMesh;
	}
}

void UEFModelFactory::PopulateMeshDescription(FMeshDescription& MeshDesc, FLODData& Data)
{
	MeshDesc.ReserveNewVertices(Data.Vertices.Num());
	MeshDesc.ReserveNewVertexInstances(Data.Vertices.Num());
	MeshDesc.ReserveNewPolygons(Data.Indices.Num() / 3);
	MeshDesc.ReserveNewPolygonGroups(Data.Materials.Num());

	//Vertices
	for (auto i = 0; i < Data.Vertices.Num(); ++i)
		MeshDesc.CreateVertex();

	//Indices
	for (const auto& Index : Data.Indices)
		MeshDesc.CreateVertexInstance(FVertexID(Index));
}

void UEFModelFactory::SetMeshAttributes(FMeshDescription& MeshDesc, FLODData& Data)
{
	FStaticMeshAttributes Attributes(MeshDesc);
	Attributes.Register();

	const auto VertexPositions = Attributes.GetVertexPositions();
	const auto VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
	const auto VertexInstanceTangents = Attributes.GetVertexInstanceTangents();
	const auto VertexInstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
	const auto VertexInstanceColors = Attributes.GetVertexInstanceColors();
	const auto VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
	VertexInstanceUVs.SetNumIndices(Data.TextureCoordinates.Num());

	//Vertices
	for (auto i = 0; i < Data.Vertices.Num(); ++i)
		VertexPositions.Set(FVertexID(i), Data.Vertices[i]);

	//Indices
	for (auto i = 0; i < Data.Indices.Num(); i++) {
		int Index = Data.Indices[i];

		//Normals
		if (Data.Normals.Num() > 0) {
			VertexInstanceBinormalSigns.Set(FVertexInstanceID(i), Data.Normals[Index].X);
			VertexInstanceNormals.Set(FVertexInstanceID(i), FVector(Data.Normals[Index].Y, Data.Normals[Index].Z, Data.Normals[Index].W));
		}
		//Tangents
		if (Data.Tangents.Num() > 0)
			VertexInstanceTangents.Set(FVertexInstanceID(i), Data.Tangents[Index]);

		//Vertex Colors
		if (Data.VertexColors.Num() > 0)
			VertexInstanceColors.Set(FVertexInstanceID(i), FVector4(Data.VertexColors[0].Data[Index]));

		//Texture Coordinates
		for (auto u = 0; u < Data.TextureCoordinates.Num(); u++)
			VertexInstanceUVs.Set(FVertexInstanceID(i), u, Data.TextureCoordinates[u][Index]);
	}
}

void UEFModelFactory::CreatePolygonGroups(FMeshDescription& MeshDesc, FLODData& Data)
{
	FStaticMeshAttributes Attributes(MeshDesc);
	Attributes.Register();

	for (const auto& [MatIndex, MatName, FirstIndex, NumFaces] : Data.Materials) {
		FPolygonGroupID PolygonGroup = MeshDesc.CreatePolygonGroup();
		for (auto i = FirstIndex; i < FirstIndex + (NumFaces * 3); i += 3)
			MeshDesc.CreatePolygon(PolygonGroup, { FVertexInstanceID(i), FVertexInstanceID(i + 1), FVertexInstanceID(i + 2) });

		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroup] = MatName.c_str();
	}
}

TArray<FStaticMaterial> UEFModelFactory::CreateStaticMaterials(TArray<FMaterialChunk> MaterialInfos)
{
	TArray<FStaticMaterial> Materials;
	for (const auto& MaterialInfo : MaterialInfos)
	{
		FStaticMaterial Material;
		Material.MaterialSlotName = MaterialInfo.Name.c_str();
		Material.ImportedMaterialSlotName = MaterialInfo.Name.c_str();
		Material.MaterialInterface = nullptr;
		Materials.Add(Material);
	}
	return Materials;
}

TArray<FSkeletalMaterial> UEFModelFactory::CreateSkeletalMaterials(TArray<FMaterialChunk> MaterialInfos)
{
	TArray<FSkeletalMaterial> Materials;
	for (const auto& MaterialInfo : MaterialInfos)
	{
		FSkeletalMaterial Material;
		Material.MaterialSlotName = MaterialInfo.Name.c_str();
		Material.ImportedMaterialSlotName = MaterialInfo.Name.c_str();
		Material.MaterialInterface = nullptr;
		Materials.Add(Material);
	}
	return Materials;
}

void UEFModelFactory::ProcessLOD(
	FMeshDescription& MeshDesc,
	FLODData& LODData)
{
	PopulateMeshDescription(MeshDesc, LODData);
	SetMeshAttributes(MeshDesc, LODData);
	CreatePolygonGroups(MeshDesc, LODData);
}

UStaticMesh* UEFModelFactory::CreateStaticMesh(TArray<FLODData>& LODData, UObject* Parent, FName Name, EObjectFlags Flags) {
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Parent->GetPackage(), Name, Flags);
	
	//Mesh Descriptions
	TArray<FMeshDescription> MeshDescriptions;
	MeshDescriptions.Reserve(LODData.Num());
	//Pointer array for passing to BuildFromMeshDescriptions
	TArray<const FMeshDescription*> MeshDescriptionPtrs;
	MeshDescriptionPtrs.Reserve(LODData.Num());
	
	for (auto i = 0; i < LODData.Num(); ++i) {
		MeshDescriptions.Emplace(); //Modify the array outside of the loop
		MeshDescriptionPtrs.Add(&MeshDescriptions[i]); //Get the pointer and add it to the pointer array
		ProcessLOD(MeshDescriptions[i], LODData[i]);
	}

	StaticMesh->PostEditChange();
	UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
	StaticMesh->BuildFromMeshDescriptions(MeshDescriptionPtrs, BuildParams);
	StaticMesh->GetStaticMaterials() = CreateStaticMaterials(LODData[0].Materials);
	
	//Mesh LOD Settings
	for (auto i = 0; i < StaticMesh->GetSourceModels().Num(); ++i)
	{
		FStaticMeshSourceModel& SourceModel = StaticMesh->GetSourceModel(i);
		SourceModel.BuildSettings.bRecomputeNormals = false;
		SourceModel.BuildSettings.bRecomputeTangents = false;
		SourceModel.BuildSettings.bRemoveDegenerates = false;
		SourceModel.BuildSettings.bGenerateLightmapUVs = false;
	}
	StaticMesh->Modify();
	
	return StaticMesh;
}

USkeletalMesh* UEFModelFactory::CreateSkeletalMesh(TArray<FLODData>& LODData, FSkeletonData& SkeletonData, UObject* Parent, FName Name, EObjectFlags Flags)
{
	USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>(Parent->GetPackage(), Name, Flags);

	//Skeleton
	FReferenceSkeleton RefSkeleton;
	USkeleton* Skeleton = CreateSkeleton(Name.ToString(), Parent, Flags, SkeletonData, RefSkeleton);
	
	SkeletalMesh->SetRefSkeleton(RefSkeleton);
	SkeletalMesh->GetMaterials() = CreateSkeletalMaterials(LODData[0].Materials);
	FSkeletalMeshModel* ImportedModel = SkeletalMesh->GetImportedModel();
	ImportedModel->LODModels.Empty();
	IMeshUtilities& MeshUtilities = FModuleManager::LoadModuleChecked<IMeshUtilities>(TEXT("MeshUtilities"));

	FBox Bounds(ForceInit);
	for (const FVector& Point : LODData[0].Vertices)
		Bounds += Point;

	for (int32 LODIndex = 0; LODIndex < LODData.Num(); ++LODIndex)
	{
		FLODData& Data = LODData[LODIndex];
		FSkeletalMeshImportData ImportData;
		ImportData.Points = Data.Vertices;
		ImportData.PointToRawMap.Reserve(Data.Vertices.Num());
		for (int32 PointIndex = 0; PointIndex < Data.Vertices.Num(); ++PointIndex)
			ImportData.PointToRawMap.Add(PointIndex);
		ImportData.NumTexCoords = FMath::Max(1, Data.TextureCoordinates.Num());
		ImportData.bHasNormals = Data.Normals.Num() == Data.Vertices.Num();
		ImportData.bHasTangents = Data.Tangents.Num() == Data.Vertices.Num();
		ImportData.bHasVertexColors = Data.VertexColors.Num() > 0;

		for (const FMaterialChunk& Material : Data.Materials)
		{
			SkeletalMeshImportData::FMaterial& ImportedMaterial = ImportData.Materials.AddDefaulted_GetRef();
			ImportedMaterial.MaterialImportName = Material.Name.c_str();
		}

		ImportData.Wedges.Reserve(Data.Indices.Num());
		for (int32 WedgeIndex = 0; WedgeIndex < Data.Indices.Num(); ++WedgeIndex)
		{
			const int32 VertexIndex = Data.Indices[WedgeIndex];
			SkeletalMeshImportData::FVertex& Wedge = ImportData.Wedges.AddDefaulted_GetRef();
			Wedge.VertexIndex = VertexIndex;
			Wedge.Color = ImportData.bHasVertexColors ? Data.VertexColors[0].Data[VertexIndex] : FColor::White;
			for (int32 UVIndex = 0; UVIndex < Data.TextureCoordinates.Num() && UVIndex < MAX_TEXCOORDS; ++UVIndex)
				Wedge.UVs[UVIndex] = Data.TextureCoordinates[UVIndex][VertexIndex];
		}

		for (int32 MaterialIndex = 0; MaterialIndex < Data.Materials.Num(); ++MaterialIndex)
		{
			const FMaterialChunk& Material = Data.Materials[MaterialIndex];
			for (int32 Index = Material.FirstIndex; Index < Material.FirstIndex + Material.NumFaces * 3; Index += 3)
			{
				SkeletalMeshImportData::FTriangle& Face = ImportData.Faces.AddDefaulted_GetRef();
				Face.MatIndex = MaterialIndex;
				Face.AuxMatIndex = 0;
				Face.SmoothingGroups = 1;
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					Face.WedgeIndex[Corner] = Index + Corner;
					const int32 VertexIndex = Data.Indices[Index + Corner];
					const FVector Normal = ImportData.bHasNormals ? FVector(Data.Normals[VertexIndex].Y, Data.Normals[VertexIndex].Z, Data.Normals[VertexIndex].W) : FVector::UpVector;
					const FVector Tangent = ImportData.bHasTangents ? Data.Tangents[VertexIndex] : FVector::ForwardVector;
					Face.TangentX[Corner] = Tangent;
					Face.TangentZ[Corner] = Normal;
					Face.TangentY[Corner] = (Normal ^ Tangent).GetSafeNormal() * (ImportData.bHasNormals ? Data.Normals[VertexIndex].X : 1.f);
				}
			}
		}

		for (const FWeightChunk& Weight : Data.Weights)
		{
			SkeletalMeshImportData::FRawBoneInfluence& Influence = ImportData.Influences.AddDefaulted_GetRef();
			Influence.VertexIndex = Weight.WeightVertexIndex;
			Influence.BoneIndex = Weight.WeightBoneIndex;
			Influence.Weight = Weight.WeightAmount;
		}

		TArray<FVector> Points;
		TArray<SkeletalMeshImportData::FMeshWedge> Wedges;
		TArray<SkeletalMeshImportData::FMeshFace> Faces;
		TArray<SkeletalMeshImportData::FVertInfluence> Influences;
		TArray<int32> PointToRawMap;
		ImportData.CopyLODImportData(Points, Wedges, Faces, Influences, PointToRawMap);
		ImportedModel->LODModels.Add(new FSkeletalMeshLODModel());
		FSkeletalMeshLODModel& LODModel = ImportedModel->LODModels[LODIndex];
		LODModel.NumTexCoords = ImportData.NumTexCoords;
		IMeshUtilities::MeshBuildOptions BuildOptions;
		BuildOptions.bComputeNormals = !ImportData.bHasNormals;
		BuildOptions.bComputeTangents = !ImportData.bHasTangents;
		MeshUtilities.BuildSkeletalMesh(LODModel, SkeletalMesh->GetPathName(), RefSkeleton, Influences, Wedges, Faces, Points, PointToRawMap, BuildOptions);
		SkeletalMesh->SaveLODImportedData(LODIndex, ImportData);
		SkeletalMesh->AddLODInfo();

		for (const FMorphTargetChunk& MorphData : Data.Morphs)
		{
			UMorphTarget* MorphTarget = SkeletalMesh->FindMorphTarget(MorphData.MorphName.c_str());
			if (!MorphTarget)
				MorphTarget = NewObject<UMorphTarget>(SkeletalMesh, MorphData.MorphName.c_str());
			TArray<FMorphTargetDelta> Deltas;
			for (const FMorphTargetDataChunk& SourceDelta : MorphData.MorphDeltas)
			{
				FMorphTargetDelta& Delta = Deltas.AddDefaulted_GetRef();
				Delta.SourceIdx = SourceDelta.MorphVertexIndex;
				Delta.PositionDelta = FVector(SourceDelta.MorphPosition.X, -SourceDelta.MorphPosition.Y, SourceDelta.MorphPosition.Z);
				Delta.TangentZDelta = SourceDelta.MorphNormals;
			}
			MorphTarget->PopulateDeltas(Deltas, LODIndex, LODModel.Sections, true, false);
			SkeletalMesh->RegisterMorphTarget(MorphTarget, false);
		}
	}
	SkeletalMesh->SetImportedBounds(FBoxSphereBounds(Bounds));
	SkeletalMesh->CalculateInvRefMatrices();

	SkeletalMesh->SetSkeleton(Skeleton);
	Skeleton->MergeAllBonesToBoneTree(SkeletalMesh);
	Skeleton->SetPreviewMesh(SkeletalMesh);

	Skeleton->PostEditChange();
	FAssetRegistryModule::AssetCreated(Skeleton);
	
    return SkeletalMesh;
}

USkeleton* UEFModelFactory::CreateSkeleton(FString Name, UObject* Parent, EObjectFlags Flags, FSkeletonData& Data, FReferenceSkeleton& RefSkeleton)
{
	FString SkeletonName = Name + "_Skeleton";
	auto SkeletonPackage = CreatePackage(*FPaths::Combine(FPaths::GetPath(Parent->GetPathName()), SkeletonName));
	USkeleton* Skeleton = NewObject<USkeleton>(SkeletonPackage, FName(*SkeletonName), Flags);
	
	FReferenceSkeletonModifier RefSkeletonModifier(RefSkeleton, Skeleton);
	RefSkeleton.Empty();

	for (const auto& Bone : Data.Bones)
	{
		FTransform Transform;
		Transform.SetLocation(FVector(Bone.BonePos));
		Transform.SetRotation(FQuat(Bone.BoneRot));
		Transform.SetScale3D(FVector(Bone.BoneScale));

		FMeshBoneInfo BoneInfo(Bone.BoneName.c_str(), Bone.BoneName.c_str(), Bone.BoneParentIndex);
		RefSkeletonModifier.Add(BoneInfo, FTransform(Transform));
	}

	for (const auto& Socket : Data.Sockets)
	{
		USkeletalMeshSocket* NewSocket = NewObject<USkeletalMeshSocket>(Skeleton);
		NewSocket->SocketName = Socket.SocketName.c_str();
		NewSocket->BoneName = Socket.SocketParentName.c_str();
		NewSocket->RelativeLocation = FVector(Socket.SocketPos);
		NewSocket->RelativeRotation = FQuat(Socket.SocketRot).Rotator();
		NewSocket->RelativeScale = FVector(Socket.SocketScale);
		Skeleton->Sockets.Add(NewSocket);
	}

	for (const auto& VirtualBone : Data.VirtualBones)
	{
		FName CreatedVirtualBoneName;
		if (Skeleton->AddNewVirtualBone(VirtualBone.SourceBoneName.c_str(), VirtualBone.TargetBoneName.c_str(), CreatedVirtualBoneName))
			Skeleton->RenameVirtualBone(CreatedVirtualBoneName, VirtualBone.VirtualBoneName.c_str());
	}
	
	return Skeleton;
}
