#pragma once
#include <iostream>
#include <chrono>

class Time {
public:
	Time();

	void Start();
	void PreUpdate();
	void Update();
	void PostUpdate();

	float deltaTime;

	static Time* GetInstance();
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_currentTime;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_lastTime;

	static Time* m_instance;
};