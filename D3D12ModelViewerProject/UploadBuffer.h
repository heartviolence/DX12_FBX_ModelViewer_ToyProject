#pragma once

#include "D3DUtil.h"

template<class T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT elementCount, bool isConstantBuffer) :
		m_isConstantBuffer(isConstantBuffer)
	{
		m_elementByteSize = sizeof(T);

		if (isConstantBuffer)
		{
			m_elementByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(T));
		}

		m_elementCount = elementCount;
		m_bufferSize = m_elementByteSize * elementCount;
		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize);
		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_uploadBuffer)));

		ThrowIfFailed(m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData)));
	}
	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		if (m_uploadBuffer != nullptr)
		{
			m_uploadBuffer->Unmap(0, nullptr);
		}
		m_uploadBuffer = nullptr;
	}
	ID3D12Resource* Resource() const
	{
		return m_uploadBuffer.Get();
	}

	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&m_mappedData[elementIndex * m_elementByteSize], &data, sizeof(T));
	}
	void CopyData(int elementIndex, const T* pDataArray, size_t elementCount)
	{
		memcpy(&m_mappedData[elementIndex * m_elementByteSize], pDataArray, sizeof(T) * elementCount);
	}
	UINT GetElementByteSize() { return m_elementByteSize; }
	UINT GetBufferByteSize() { return m_bufferSize; }
	UINT GetMaxElementCount() { return m_elementCount; }
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_uploadBuffer;
	BYTE* m_mappedData = nullptr;

	UINT m_elementByteSize = 0;
	bool m_isConstantBuffer = false;
	UINT m_bufferSize = 0;
	UINT m_elementCount = 0;
};