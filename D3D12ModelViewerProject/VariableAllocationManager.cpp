#include "VariableAllocationManager.h"

void VariableSizeAllocationsManager::AddNewBlock(OffsetType Offset, OffsetType size)
{
	auto newBlockIter = m_freeBlocksByOffset.emplace(Offset, size);
	auto OrderIt = m_freeBlocksBySize.emplace(size, newBlockIter.first);
	newBlockIter.first->second.OrderBySizeIt = OrderIt;
}

OffsetType VariableSizeAllocationsManager::Allocate(OffsetType size)
{
	if (m_freeSize < size)
	{
		return InvalidOffset;
	}

	auto smallestBlockIterIter = m_freeBlocksBySize.lower_bound(size);
	if (smallestBlockIterIter == m_freeBlocksBySize.end())
	{
		return InvalidOffset;
	}

	auto smallestBlockIter = smallestBlockIterIter->second;
	auto offset = smallestBlockIter->first;
	auto newOffset = offset + size;
	auto newSize = smallestBlockIter->second.Size - size;

	m_freeBlocksBySize.erase(smallestBlockIterIter);
	m_freeBlocksByOffset.erase(smallestBlockIter);

	if (newSize > 0)
	{
		AddNewBlock(newOffset, newSize);
	}

	m_freeSize -= size;
	return offset;
}

VariableSizeAllocationsManager::VariableSizeAllocationsManager(int MaxSize) : m_freeSize(MaxSize)
{
	AddNewBlock(0, MaxSize);
}

VariableSizeAllocationsManager::VariableSizeAllocationsManager(VariableSizeAllocationsManager&& rhs) noexcept
{
	m_freeBlocksByOffset = std::move(rhs.m_freeBlocksByOffset);
	m_freeBlocksBySize = std::move(rhs.m_freeBlocksBySize);
	m_freeSize = rhs.m_freeSize;
}

void VariableSizeAllocationsManager::Free(OffsetType offset, OffsetType size)
{
	//4���� ��츦 ó���ؾ���
	//�Ҵ簡�� �޸𸮰� ���ʿ� �����Ѱ�� ����
	//�����ʿ� �����Ѱ�� ����
	//���ʿ� �����Ѱ�� ����
	//���� ������ ���ο� �Ҵ簡�� �޸� �߰�
	auto nextBlockIter = m_freeBlocksByOffset.upper_bound(offset); //���� �����޸�
	auto prevBlockIter = nextBlockIter;  //���� �����޸�
	if (prevBlockIter != m_freeBlocksByOffset.begin())
	{
		--prevBlockIter;
	}
	else
	{
		prevBlockIter = m_freeBlocksByOffset.end();
	}

	OffsetType newSize;
	OffsetType newOffset;
	//��������
	if (prevBlockIter != m_freeBlocksByOffset.end() && offset == prevBlockIter->first + prevBlockIter->second.Size)
	{
		newSize = prevBlockIter->second.Size + size;
		newOffset = prevBlockIter->first;
		//���� ����
		if (nextBlockIter != m_freeBlocksByOffset.end() && offset + size == nextBlockIter->first)
		{
			newSize += nextBlockIter->second.Size;
			m_freeBlocksBySize.erase(prevBlockIter->second.OrderBySizeIt);
			m_freeBlocksBySize.erase(nextBlockIter->second.OrderBySizeIt);
			++nextBlockIter;
			m_freeBlocksByOffset.erase(prevBlockIter, nextBlockIter);
		}
		//���� ������������
		else
		{
			m_freeBlocksBySize.erase(prevBlockIter->second.OrderBySizeIt);
			m_freeBlocksByOffset.erase(prevBlockIter);
		}
	}
	//���� ������������ + ��������
	else if (nextBlockIter != m_freeBlocksByOffset.end() && offset + size == nextBlockIter->first)
	{
		newSize = size + nextBlockIter->second.Size;
		newOffset = offset;
		m_freeBlocksBySize.erase(nextBlockIter->second.OrderBySizeIt);
		m_freeBlocksByOffset.erase(nextBlockIter);
	}
	else //��������
	{
		newSize = size;
		newOffset = offset;
	}

	AddNewBlock(newOffset, newSize);

	m_freeSize += size;
}

VariableSizeGPUAllocationsManager::VariableSizeGPUAllocationsManager(OffsetType MaxSize) : VariableSizeAllocationsManager(MaxSize)
{
}

VariableSizeGPUAllocationsManager::~VariableSizeGPUAllocationsManager()
{
}

VariableSizeGPUAllocationsManager::VariableSizeGPUAllocationsManager(VariableSizeGPUAllocationsManager&& rhs) noexcept : VariableSizeAllocationsManager(std::move(rhs))
{
	m_staleAllocations = std::move(rhs.m_staleAllocations);
}
