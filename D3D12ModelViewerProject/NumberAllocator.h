#pragma once

#include "VariableAllocationManager.h"

class NumberAllocator
{
public:
	NumberAllocator(int maxSize = 100000) :
		m_manager(maxSize) {}
public:
	size_t Allocate()
	{
		return m_manager.Allocate(1);
	}

	void Free(size_t number)
	{
		m_manager.Free(number, 1);
	}
private:
	VariableSizeAllocationsManager m_manager;
};