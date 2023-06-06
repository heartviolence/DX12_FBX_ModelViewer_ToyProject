#pragma once
#include <map>
#include <stdint.h>
#include <vector>
#include <deque>

typedef size_t OffsetType;
struct FreeBlockInfo;


using TFreeBlockByOffsetMap = std::map<OffsetType, FreeBlockInfo>;
using TFreeBlockBySizeMap = std::multimap<OffsetType, TFreeBlockByOffsetMap::iterator>;

struct FreeBlockInfo
{
	OffsetType Size;

	TFreeBlockBySizeMap::iterator OrderBySizeIt;
	FreeBlockInfo(OffsetType size) : Size(size) {}
};


class VariableSizeAllocationsManager
{
public:
	const static OffsetType InvalidOffset = static_cast<OffsetType>(-1);
public:
	VariableSizeAllocationsManager(int MaxSize);
	VariableSizeAllocationsManager(VariableSizeAllocationsManager&& rhs) noexcept;
public:

	void Free(OffsetType offset, OffsetType size);
	OffsetType Allocate(OffsetType size);
	OffsetType GetFreeSize() const
	{
		return m_freeSize;
	}
private:
	void AddNewBlock(OffsetType Offset, OffsetType size);
private:
	TFreeBlockByOffsetMap m_freeBlocksByOffset;
	TFreeBlockBySizeMap m_freeBlocksBySize;
	OffsetType m_freeSize;
};


class VariableSizeGPUAllocationsManager : public VariableSizeAllocationsManager
{
private:
	struct FreedAllocationInfo
	{
		FreedAllocationInfo(OffsetType _offset, OffsetType _size, uint64_t _frameNumber)
			: Offset(_offset), Size(_size), FrameNumber(_frameNumber) {}

		OffsetType Offset;
		OffsetType Size;
		uint64_t FrameNumber;
	};

public:
	VariableSizeGPUAllocationsManager(OffsetType MaxSize);
	~VariableSizeGPUAllocationsManager();
	VariableSizeGPUAllocationsManager(VariableSizeGPUAllocationsManager&& rhs) noexcept;

	VariableSizeGPUAllocationsManager& operator = (VariableSizeGPUAllocationsManager&& rhs) = default;
	VariableSizeGPUAllocationsManager(const VariableSizeGPUAllocationsManager&) = delete;
	VariableSizeGPUAllocationsManager& operator = (const VariableSizeGPUAllocationsManager&) = delete;

	void Free(OffsetType Offset, OffsetType Size, uint64_t FrameNumber)
	{
		m_staleAllocations.emplace_back(Offset, Size, FrameNumber);
	}

	void ReleaseCompletedFrames(uint64_t NumCompletedFrames)
	{
		while (!m_staleAllocations.empty() && m_staleAllocations.front().FrameNumber < NumCompletedFrames)
		{
			auto& OldestAllocation = m_staleAllocations.front();
			VariableSizeAllocationsManager::Free(OldestAllocation.Offset, OldestAllocation.Size);
			m_staleAllocations.pop_front();
		}
	}

private:
	std::deque< FreedAllocationInfo > m_staleAllocations;
};