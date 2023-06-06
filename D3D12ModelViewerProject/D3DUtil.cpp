#include "D3DUtil.h"
#include <comdef.h>
#include <fstream>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{

}
std::wstring DxException::ToString() const
{
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + _TEXT("Failed in ") + Filename + _TEXT(";line ") + std::to_wstring(LineNumber) + _TEXT(";error: ") + msg;
}

ComPtr<ID3DBlob> D3DUtil::CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target)
{
	UINT compileFlag = 0;
#if defined(DEBUG) || defined(_DEBUG)
	compileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;

	hr = D3DCompileFromFile(
		filename.c_str(),
		defines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(),
		compileFlag,
		0,
		&byteCode,
		&errors);

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	return byteCode;
}

Microsoft::WRL::ComPtr<ID3D12Resource> D3DUtil::CreateDefaultBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 byteSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource> defaultBuffer;

	auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	auto UploadheapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	ThrowIfFailed(device->CreateCommittedResource(
		&UploadheapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	D3D12_SUBRESOURCE_DATA subResourceData = { };
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	UpdateSubresources<1>(
		cmdList,
		defaultBuffer.Get(),
		uploadBuffer.Get(),
		0,
		0,
		1,
		&subResourceData);

	auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		defaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_GENERIC_READ);

	cmdList->ResourceBarrier(1, &resourceBarrier);

	return defaultBuffer;
}

void Transform::Initialize(DirectX::SimpleMath::Matrix affineMatrix)
{
	using namespace DirectX;
	using namespace DirectX::SimpleMath;

	SimpleMath::Quaternion quat;
	affineMatrix.Decompose(Scale, quat, Translation);

	Vector3 euler = quat.ToEuler();

	euler.x = XMConvertToDegrees(euler.x);
	euler.y = XMConvertToDegrees(euler.y);
	euler.z = XMConvertToDegrees(euler.z);

	Rotation = euler;
}

DirectX::XMMATRIX Transform::GetFinalTransformMatrix() const
{
	using namespace DirectX;

	XMVECTOR scaling = XMLoadFloat3(&Scale);
	XMVECTOR rotationQuat = GetQuaternionVector();
	XMVECTOR translation = XMLoadFloat3(&Translation);
	XMVECTOR zeroVector = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	return XMMatrixAffineTransformation(scaling, zeroVector, rotationQuat, translation);
}