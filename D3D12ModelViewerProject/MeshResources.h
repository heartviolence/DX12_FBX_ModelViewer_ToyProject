#pragma once
#include <vector>
#include <map> 
#include "FbxUtil.h"
#include "DescriptorHeapManager.h"
#include "SceneObject.h"
#include "NumberAllocator.h"
#include "AnimationCalculator.h"

using IndexTableType = std::vector<IndexBufferFormat>;

struct MeshResources;
struct MeshResourcesInfo;
class MeshObject;

struct MeshResourcesInfo
{
	std::vector<PBRMaterial> Materials;
	std::vector<Vertex> VertexTable;
	std::map<std::string, IndexTableType> SubMeshes;
	Skeleton Skeleton;
};

struct MeshResources
{
public:
	MeshResources(MeshResourcesInfo& modelResourceInfo);

	std::shared_ptr<MeshObject> CreateMeshObject(std::string name);
public:
	std::shared_ptr<MeshGeometry> Geometry;
	std::vector<PBRMaterial> Materials;
	Skeleton Skeleton;
};

