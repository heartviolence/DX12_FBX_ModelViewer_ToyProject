#pragma once

#include "ThreadPool.h"

class ThreadManager
{
public:
	static ThreadManager& GetInstance()
	{
		static ThreadManager threadManager;
		return threadManager;
	}

	void EnqueueJob(std::function<void()> func)
	{
		m_threadPool.EnqueueJob(std::move(func));
	}
private:
	ThreadManager() :m_threadPool(4) {}
	ThreadManager(const ThreadManager&) = delete;
	ThreadManager& operator=(const ThreadManager&) = delete;
private:
	ThreadPool m_threadPool;
};