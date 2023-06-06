#include "MeshResources.h"
#include "D3DResourceManager.h"
#include "FileUtil.h"
#include "GameTimer.h"
#include "MeshObject.h"

using namespace std;
using namespace DirectX;

MeshResources::MeshResources(MeshResourcesInfo& modelResourceInfo)
	: Materials(modelResourceInfo.Materials),
	Skeleton(std::move(modelResourceInfo.Skeleton))
{
	std::map<std::string, IndexTableType>& subMeshes = modelResourceInfo.SubMeshes;
	std::vector<Vertex>& vertexTable = modelResourceInfo.VertexTable;

	//buildGeometry
	auto geo = std::make_shared<MeshGeometry>();
	std::vector<IndexBufferFormat> indexTable;
	size_t indexTableSize = 0;
	for (auto& element : subMeshes)
	{
		indexTableSize += element.second.size();
	}
	indexTable.reserve(indexTableSize);

	int startIndexLocation = 0;
	for (auto& element : subMeshes)
	{
		string materialName = element.first;
		IndexTableType subMesh = element.second;

		SubmeshGeometry subGeo;
		subGeo.BaseVertexLocation = 0;
		subGeo.IndexCount = subMesh.size();
		subGeo.StartIndexLocation = startIndexLocation;

		indexTable.insert(indexTable.end(), subMesh.begin(), subMesh.end());
		startIndexLocation += subMesh.size();

		geo->DrawArgs[materialName] = subGeo;
	}

	const UINT vbByteSize = (UINT)vertexTable.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indexTable.size() * sizeof(IndexBufferFormat);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	D3DResourceManager& resourceManager = D3DResourceManager::GetInstance();

	geo->VertexBufferGPU = resourceManager.CreateDefaultBuffer(vertexTable.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = resourceManager.CreateDefaultBuffer(indexTable.data(), ibByteSize, geo->IndexBufferUploader);

	Geometry = std::move(geo);
}

std::shared_ptr<MeshObject> MeshResources::CreateMeshObject(std::string name)
{
	return MeshObject::CreateMeshObject(name, *this);
}
