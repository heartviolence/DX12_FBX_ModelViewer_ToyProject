#pragma once

#include "framework.h"
#include "Resource.h"
#include <string>
#include <WindowsX.h>
#include <initguid.h>
#include <d3d12.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3dx12/d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <array>
#include <unordered_map>
#include <DirectXCollision.h>
#include "MathHelper.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"
#include <SimpleMath.h>

struct AffineMatrix;
struct Light;
struct Lights;
struct DirectionalLight;
struct PointLight;

typedef  uint32_t IndexBufferFormat;

enum { FramesCount = 3 };

inline bool IsFloat3Equal(const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2)
{
	return (v1.x == v2.x &&
		v1.y == v2.y &&
		v1.z == v2.z);
}

class DxException
{
public:
	DxException() = delete;
	DxException(HRESULT hr, const std::wstring& functionName, const std::wstring fileName, int lineNumber);

	std::wstring ToString()const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};


inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}

class D3DUtil
{
public:
	static UINT CalcConstantBufferByteSize(UINT byteSize)
	{
		return (byteSize + 255) & ~255;
	}

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);
};

struct SubmeshGeometry
{
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	INT BaseVertexLocation = 0;

	DirectX::BoundingBox Bounds;
};

struct MeshGeometry
{
	std::wstring Name;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

	UINT VertexByteStride = 0;
	UINT VertexBufferByteSize = 0;
	DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;
	UINT IndexBufferByteSize = 0;

	std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
	{
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
		vbv.SizeInBytes = VertexBufferByteSize;
		vbv.StrideInBytes = VertexByteStride;

		return vbv;
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView() const
	{
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
		ibv.Format = IndexFormat;
		ibv.SizeInBytes = IndexBufferByteSize;

		return ibv;
	}

	void DisposeUploaders()
	{
		VertexBufferUploader = nullptr;
		IndexBufferUploader = nullptr;
	}

};

enum { MaxLightCount = 16 };

struct Light
{
	DirectX::XMFLOAT3 Direction = { 0.0f,-1.0f,0.0f };
	float Power = 1.0f;
	DirectX::XMFLOAT3 Color = { 0.5f,0.5f,0.5f };
	DirectX::XMFLOAT3 Position = { 0.0f,0.0f,0.0f };
};

struct DirectionalLight
{
	DirectX::XMFLOAT3 Direction = { 0.0f,-1.0f,0.0f };
	DirectX::XMFLOAT3 Color = { 0.5f,0.5f,0.5f };
	float Power = 1.0f;

	std::string name = "DirectionalLight";

	Light ToLight()
	{
		return Light{
			Direction,
			Power,
			Color,
			{0.0f,0.0f,0.0f}
		};
	}
};

struct PointLight
{
	float Power = 1.0f;
	DirectX::XMFLOAT3 Color = { 0.5f,0.5f,0.5f };
	DirectX::XMFLOAT3 Position = { 0.0f,0.0f,0.0f };

	std::string name = "PointLight";
	Light ToLight()
	{
		return Light
		{
			{0.0f,0.0f,0.0f},
			Power,
			Color,
			Position
		};
	}
};

struct Lights
{
	std::vector<DirectionalLight> DirectionalLights;
	std::vector<PointLight> PointLights;

	size_t Count()
	{
		return DirectionalLights.size() + PointLights.size();
	}
};

struct PBRMaterialConstants
{
	DirectX::XMFLOAT4 Albedo = { 0.8f,0.8f,0.8f,1.0f };
	float Metalic = 0.0f;
	float Roughness = 0.25f;
	UINT padding0;
	UINT padding1;
};

struct PBRMaterial
{
	std::string Name;

	DirectX::SimpleMath::Color Albedo = { 0.8f,0.8f,0.8f,1.0f };
	float Metalic = 0.0f;
	float Roughness = 0.25f;

	std::wstring AlbedoMap;

	int NumFrameDirty = FramesCount;

	bool operator==(const PBRMaterial& rhs)
	{
		return (Albedo == rhs.Albedo &&
			Metalic == rhs.Metalic &&
			Roughness == rhs.Roughness &&
			AlbedoMap == AlbedoMap);
	}

	bool operator!=(const PBRMaterial& rhs)
	{
		return !operator==(rhs);
	}

	PBRMaterialConstants ToMaterialConstants()
	{
		return PBRMaterialConstants{
		Albedo,
		Metalic,
		Roughness
		};
	}
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)												\
{																		\
	HRESULT hr__ = (x);													\
	std::wstring wfn = AnsiToWString(__FILE__);							\
	if(FAILED(hr__)){throw DxException(hr__,_TEXT(#x),wfn,__LINE__);}   \
}
#endif // !ThrowIfFailed


#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){x->Release(); x=0;}}
#endif // ReleaseCom


struct RenderItem
{
	RenderItem() = default;

	UINT MaterialCBIndex = -1;

	MeshGeometry* Geo = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};


struct Transform
{
	//Scale x,y,z
	DirectX::SimpleMath::Vector3 Scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);
	//axis x, y, z, 
	DirectX::SimpleMath::Vector3 Rotation = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	//Translate x,y,z
	DirectX::SimpleMath::Vector3 Translation = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);


	void Initialize(DirectX::SimpleMath::Matrix affineMatrix);
	

	DirectX::XMMATRIX GetFinalTransformMatrix() const;	


	DirectX::XMVECTOR GetQuaternionVector() const
	{
		using namespace DirectX;
		return XMQuaternionRotationRollPitchYaw(XMConvertToRadians(Rotation.x), XMConvertToRadians(Rotation.y), XMConvertToRadians(Rotation.z));
	}

	bool operator==(const Transform& rhs)
	{
		return ((Scale == rhs.Scale) &&
			(Rotation == rhs.Rotation) &&
			(Translation == rhs.Translation));
	}
	bool operator!=(const Transform& rhs)
	{
		return !(operator==(rhs));
	}
};

inline DirectX::XMFLOAT4X4 XMFLOAT4X4Multiply(const DirectX::XMFLOAT4X4& f1, const DirectX::XMFLOAT4X4& f2)
{
	using namespace DirectX;
	XMFLOAT4X4 result;

	XMMATRIX matrix1 = XMLoadFloat4x4(&f1);
	XMMATRIX matrix2 = XMLoadFloat4x4(&f2);
	XMMATRIX finalMatrix = XMMatrixMultiply(matrix1, matrix2);

	XMStoreFloat4x4(&result, finalMatrix);
	return result;
}


struct AffineMatrix
{
	DirectX::XMVECTOR Scale = DirectX::XMVectorSet(1, 1, 1, 0);
	DirectX::XMVECTOR Quaternion = DirectX::XMQuaternionIdentity();
	DirectX::XMVECTOR Translation = DirectX::XMVectorZero();

	DirectX::XMMATRIX GetFinalTransformMatrix() const
	{
		return DirectX::XMMatrixAffineTransformation(Scale, DirectX::XMQuaternionIdentity(), Quaternion, Translation);
	}

	void Initialize(DirectX::XMMATRIX matrix)
	{
		using namespace DirectX;
		XMMatrixDecompose(&Scale, &Quaternion, &Translation, matrix);
	}
};

enum class MeshType : int
{
	StaticMesh = 0,
	DynamicMesh,
	Count
};






