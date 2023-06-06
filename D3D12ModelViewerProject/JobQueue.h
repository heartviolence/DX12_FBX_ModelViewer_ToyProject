#pragma once

#include <thread>
#include <functional>
#include <mutex>
#include <deque>

class JobQueue
{
private:
	using VoidFunction = std::function<void()>;
public:

	void AddJobQueue(const std::vector<VoidFunction>& addList)
	{
		std::lock_guard<std::mutex> lock(m_updateQueueMutex);
		m_updateQueue.insert(m_updateQueue.end(), addList.begin(), addList.end());
	}

	void AddJobQueue(const VoidFunction& addItem)
	{
		std::lock_guard<std::mutex> lock(m_updateQueueMutex);
		m_updateQueue.push_back(addItem);
	}

	void ProcessJobQueue()
	{
		std::lock_guard<std::mutex> lock(m_updateQueueMutex);
		while (m_updateQueue.empty() == false)
		{
			auto element = m_updateQueue.front();
			m_updateQueue.pop_front();
			element();
		}
	}
private:
	std::mutex m_updateQueueMutex;
	std::deque<VoidFunction> m_updateQueue;
};