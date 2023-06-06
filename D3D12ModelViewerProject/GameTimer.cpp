#include "GameTimer.h"
#include <Windows.h>

GameTimer::GameTimer() :
	m_secondPerCount(0.0),
	m_deltaTime(-1.0),
	m_baseTime(0),
	m_pausedTime(0),
	m_prevTime(0),
	m_currTime(0),
	m_stopTime(0),
	m_stopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	m_secondPerCount = 1.0 / (double)countsPerSec;
}

float GameTimer::TotalTime() const
{
	if (m_stopped)
	{
		return static_cast<float>(((m_stopped - m_pausedTime) - m_baseTime) * m_secondPerCount);
	}
	return static_cast<float>(((m_currTime - m_pausedTime) - m_baseTime) * m_secondPerCount);
}

float GameTimer::DeltaTime() const
{
	return static_cast<float>(m_deltaTime);
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	m_baseTime = currTime;
	m_prevTime = currTime;
	m_stopped = false;
	m_stopTime = 0;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	if (m_stopped)
	{
		m_pausedTime += (startTime - m_stopped);

		m_prevTime = startTime;
		m_stopTime = 0;
		m_stopped = false;
	}
}

void GameTimer::Stop()
{
	if (m_stopped)
	{
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

	m_stopped = true;
	m_stopTime = currTime;
}

void GameTimer::Tick()
{
	if (m_stopped)
	{
		m_deltaTime = 0;
		return;
	}

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	m_currTime = currTime;

	m_deltaTime = (m_currTime - m_prevTime) * m_secondPerCount;
	m_prevTime = m_currTime;

	if (m_deltaTime < 0.0)
	{
		m_deltaTime = 0.0;
	}
}

GameTimer& GetMainTimer()
{
	static GameTimer timer;
	return timer;
}
